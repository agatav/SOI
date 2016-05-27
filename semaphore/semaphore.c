#include <unistd.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/wait.h>

#include <stdlib.h>
#include <string.h>

#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>

#include <time.h>
#include <stdbool.h>


#define MAX 9
#define true 1
#define false 0

typedef struct SemConditions
{

	bool waiting_for_write_A ;
	bool waiting_for_write_B ;
	bool waiting_for_read_A ;
	bool waiting_for_read_B ;
	
}Condition; 

 Condition * BindCondition()
{
	static int shmid = 0;

	if(shmid == 0)
		shmid = shmget(IPC_PRIVATE, sizeof(Condition) , SHM_W | SHM_R);

	if(shmid < 0)
	{
		perror("booleans shared memory id");
		abort();
	}

	void * data = shmat(shmid, NULL, 0);

	Condition * lockedBools = (Condition*)data;

	return lockedBools;
}

Condition * InitCondition()
{
	Condition * booleans = BindCondition();

	booleans->waiting_for_write_A = false;
	booleans->waiting_for_write_B = false;
	booleans->waiting_for_read_A = false;
	booleans->waiting_for_read_B = false;

	return booleans;
}

typedef struct FIFOQUEUE
{
	short int start, end; //must init with 0
	int size; //must init with 0
	int *prd;

}Queue;

void printQueue(Queue * q)
{
	printf("%d\n", q->size);
}

void insertElement(Queue * q, char prd)
{
	q->prd[q->end] = prd;
	++(q->end);
	q->end %= MAX;
	q->size++;
}

char removeElement(Queue * q)
{
	char to_return = q->prd[q->start];
	++(q->start);
	q->start %= MAX;
	q->size--;
	
	return to_return;
}


Queue * BindQueue()
{

	static int shmid = 0;

	if(shmid == 0)
		shmid = shmget(IPC_PRIVATE, sizeof(Queue) + MAX * sizeof(int) , SHM_W | SHM_R);

	if(shmid <= 0) /* -1 ?*/
	{
		perror("shmid queue");
		abort();
	}

	int * data = shmat(shmid, NULL, 0);

	Queue * queue = (Queue*)data;

	queue->prd = (int*)(data + MAX * sizeof(int));

	return queue;

}

Queue * InitQueue()
{
	Queue * queue = BindQueue();
	queue->start = 0;
	queue->end = 0;
	queue->size = 0;
	
	return queue;
}

typedef struct ProjectSemaphores
{

	sem_t write_A;
	sem_t write_B;
	sem_t read_A;
	sem_t read_B;
	sem_t MUTEX;

}Semaphores;


Semaphores * BindSemaphores()
{
	static int shmid = 0;

	if(shmid == 0)
		shmid = shmget(IPC_PRIVATE, sizeof(struct ProjectSemaphores), SHM_W | SHM_R);

	if(shmid <= 0)
	{
		perror("shmid semaphores");
		abort();
	}
	void * data = shmat(shmid, NULL, 0);
	Semaphores * semCollection = (Semaphores*)data;

	return semCollection;

}



Semaphores * InitSemaphores()
{

	Semaphores * semCollection = BindSemaphores();

	if(sem_init(&semCollection->write_A, 1, 0))
		perror("sem_init fail");
	if(sem_init(&semCollection->write_B, 1, 0))
		perror("sem_init fail");
	if(sem_init(&semCollection->read_A, 1, 0))
		perror("sem_init fail");
	if(sem_init(&semCollection->read_B, 1, 0))
		perror("sem_init fail");
	if(sem_init(&semCollection->MUTEX, 1, 1))
		perror("sem_init fail");

	return semCollection;
}


void wait_sem(sem_t * s)
{
	if(sem_wait(s))
		perror("wait_sem on semaphore");
}

void signal_sem(sem_t * s)
{
	if(sem_post(s))
		perror("up on semaphore");
}

void CreateSubProc(void (* function)())
{
	if(fork() == 0)
	{
		function();
		exit(0);
	}
}


void Producer_A()
{
	Queue * queue = BindQueue();
	Semaphores * semaphores = BindSemaphores();
	Condition * booleans = BindCondition();


while(1)
	{
		wait_sem(&semaphores->MUTEX);

		if (queue->size == MAX) 

		{
			booleans->waiting_for_write_A = true;
			signal_sem(&semaphores->MUTEX);
			wait_sem(&semaphores->write_A);
		}

		booleans->waiting_for_write_A = false;

		printf("A produkuje\n");
		char prd = 'A';

		insertElement(queue, prd);
		printQueue(queue);

		if (booleans->waiting_for_read_A && queue->size > 3)
			signal_sem(&semaphores->read_A);
		else if (booleans->waiting_for_read_B && queue->size > 4)
			signal_sem(&semaphores->read_B);
		else if (booleans->waiting_for_write_B && queue->size <= MAX - 3)
			signal_sem(&semaphores->write_B);
		else 
			 signal_sem(&semaphores->MUTEX);
		
	}
		
}

void Producer_B()
{
	Queue * queue = BindQueue();
	Semaphores * semaphores = BindSemaphores();
	Condition * booleans = BindCondition();

	while(1)
	{
		int i;
		
		wait_sem(&semaphores->MUTEX);
		
		if (queue->size > MAX-3) 

		{
			booleans->waiting_for_write_B = true;
			signal_sem(&semaphores->MUTEX);
			wait_sem(&semaphores->write_B);
		}

		booleans->waiting_for_write_B = false;

		printf("B produkuje\n");
		char prd = 'B';

		for (i=0; i<3; i++) {
		insertElement(queue, prd);
		printQueue(queue);
	}

		if (booleans->waiting_for_read_A && queue->size > 3)
			signal_sem(&semaphores->read_A);
		else if (booleans->waiting_for_read_B && queue->size > 4)
			signal_sem(&semaphores->read_B);
		else if (booleans->waiting_for_write_A && queue->size < MAX)
			signal_sem(&semaphores->write_A);
		else 
			 signal_sem(&semaphores->MUTEX);
	}

}

void Consumer_A()
{
	Queue * queue = BindQueue();
	Semaphores * semaphores = BindSemaphores(); 
	Condition * booleans = BindCondition();

	while(1)
	{

		wait_sem(&semaphores->MUTEX);

		if (queue->size <= 3) 

		{
			booleans->waiting_for_read_A = true;
			signal_sem(&semaphores->MUTEX);
			wait_sem(&semaphores->read_A);
			
		}
		
		booleans->waiting_for_read_A = false;

		printf("A konsumuje\n");
		removeElement(queue);
		printQueue(queue);

		if (booleans->waiting_for_write_B && queue->size <= MAX - 3)
			signal_sem(&semaphores->write_B);
		else if (booleans->waiting_for_read_B && queue->size > 4)
			signal_sem(&semaphores->read_B);
		else if (booleans->waiting_for_write_A && queue->size < MAX)
			signal_sem(&semaphores->write_A);
		else 
			 signal_sem(&semaphores->MUTEX);
	}

}

void Consumer_B()
{
	Queue * queue = BindQueue();
	Semaphores * semaphores = BindSemaphores();
	Condition * booleans = BindCondition();

	int i;

	while(1)
	{
		
		wait_sem(&semaphores->MUTEX);

		if (queue->size <= 4) 

		{
			booleans->waiting_for_read_B = true;
			signal_sem(&semaphores->MUTEX);
			wait_sem(&semaphores->read_B);
		}
		
		booleans->waiting_for_read_B = false;

		printf("B konsumuje\n");

		for (i=0; i<2; i++) {
			removeElement(queue);
			printQueue(queue);
	}

		if (booleans->waiting_for_write_B && queue->size <= MAX-3)
			signal_sem(&semaphores->write_B);
		else if (booleans->waiting_for_read_A && queue->size > 3)
			signal_sem(&semaphores->read_A);
		else if (booleans->waiting_for_write_A && queue->size < MAX)
			signal_sem(&semaphores->write_A);
		else 
			 signal_sem(&semaphores->MUTEX);
	}

}

int main(int argc, char **argv)
{
	InitQueue();
	InitSemaphores();
	InitCondition();


	CreateSubProc(&Producer_A);
	CreateSubProc(&Producer_B);
	CreateSubProc(&Consumer_A);
	CreateSubProc(&Consumer_B); 

	while (1) {
        sleep(1);
    }

	return 0;

}
