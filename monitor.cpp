#include "monitor.h"
#include <iostream>
#include <cstdlib>
#include <deque>
#include <pthread.h>
#include <time.h>

#define SIZE 9
#define K 50000

using namespace std;

//implementacja kolejki FIFO wraz z obsluga przewidywanych bledow.
class Buffer : public Monitor {
  private:
	  deque<char> buffer;
    char product;
    Condition sem_bigger_than_3;
    Condition sem_bigger_than_4;
    Condition sem_let_A;
    Condition sem_let_B; //pozwalamy pisać

    bool forbid_A ;
    bool forbid_B ;
    bool B_asks ;

    int back() {
      return buffer.back();
    }

    int size() {
       return buffer.size();
    }

	  void ToBegin(char product) {
      if (buffer.size() < SIZE) {
        buffer.push_front(product);
      }
      else {
        cerr << "Blad przy operacji push_front\n";
        exit(2);
      }
      return;
	  }

	  void DeleteTail() {
      if (buffer.size() > 0) {
        buffer.pop_back();
        if(buffer.size()==2) {
          cerr << "Niespelniony warunek > 3\n";
          exit(4);
        }
      }
      else {
        cerr << "Blad przy operacji pop_back\n";
        exit(3);
      }
      return;
	  }

  public:
  Buffer();
  void produce_A();
  void produce_B();
  void consume_A();
  void consume_B();

};

  Buffer::Buffer(){
     bool forbid_A  = false ;
    bool forbid_B = false ;
    bool B_asks = false;
  }

	void Buffer::produce_A() {
		enter();
		if (B_asks == true || buffer.size() == SIZE) {
			cout << "A nie produkuje" << endl;
      forbid_A = true;
			wait(sem_let_A);
			forbid_A = false;
		}
		product = 'A';
		cout << "Producent A\n" << product << endl;
		ToBegin(product);
    cout << "Dodano element: " << product << endl;
		cout << "Bufor: " << buffer.size() << endl;
   
        if (buffer.size() > 4) {
            signal(sem_bigger_than_4);
        }
       if (buffer.size() > 3) {  
      signal(sem_bigger_than_3);
    }
		if (forbid_B == true && buffer.size() <= SIZE - 3) {
      signal(sem_let_B);
    }
   	leave();
  }

  void Buffer::produce_B() {
    enter();
    B_asks = true;
    if (buffer.size() > SIZE - 3) {
	    cout << "B nie produkuje" << endl;
      forbid_B = true;
      wait(sem_let_B);
      forbid_B = false;
    }
    product = 'B';
    for (int i=0; i<3; i++)
      {
    cout << "B produkuje " << product << endl;
    ToBegin(product);
    cout << "Dodano element: " << product << endl;
    cout << "Bufor: " << buffer.size() << endl;
      }

    if (buffer.size() > 4) {
            signal(sem_bigger_than_4);
        }
       if (buffer.size() > 3) {  
      signal(sem_bigger_than_3);
    }
  
      B_asks = false;
    if (forbid_A == true && buffer.size() < SIZE) {
      signal(sem_let_A);
    }
    leave();
  }

	void Buffer::consume_A() {
		enter();
    if (buffer.size() <= 3) {
      wait(sem_bigger_than_3);
    }
    cout << "Konsument A " << endl;
    cout << "Usuwam z bufora " << (char)buffer.back() << endl;
    DeleteTail();
    cout << "Bufor: " << buffer.size() << endl;      
    if (forbid_B == true && buffer.size() <= SIZE - 3) {
    	cout << "Obudz B" << endl;
    	signal(sem_let_B);
    }
    else if (forbid_A == true && buffer.size() < SIZE) {
    	cout << "Obudz A" << endl;
    	signal(sem_let_A);
    }
		leave();
	}

	void Buffer::consume_B() {
    enter();
    if(buffer.size() <= 4) {
      wait(sem_bigger_than_4);
    }
    cout << "Konsument B\n";

    for (int i=0; i<2; i++) {
    cout << "Usuwam z bufora " << (char)buffer.back() << endl;
    DeleteTail();
    cout << "Bufor: " << buffer.size() << endl;
    }
 
 if (buffer.size() > 4) {
            signal(sem_bigger_than_4);
        }
       if (buffer.size() > 3) {  
      signal(sem_bigger_than_3);
    }

    if (forbid_B == true && buffer.size() <= SIZE - 3) {
    cout << "Obudz B" << endl;
    	signal(sem_let_B);
    }
    else if (forbid_A == true && buffer.size() < SIZE) {
    	cout << "Obudz A" << endl;
    signal(sem_let_A);
    }
    leave();
  }

////////////////////////////////////
////////////////////////////////////
Buffer buffer;

/* void * ProducerA(void*) {
  while (1) {
    buffer.produce_A();
    
  }
}

void * ProducerB(void*) {
  while( 1) {
    buffer.produce_B();
  }
}

void * ConsumerA(void*) {
    while( 1 ) {
    buffer.consume_A();
  }
}

void * ConsumerB(void*) {
   while( 1 ) {
    buffer.consume_B();
    }
} */

 void * ProducerA(void*) {
  int i = 0;
  while ( i!= K ) {
    i++;
    buffer.produce_A();
    
  }
}

void * ProducerB(void*) {
  int i = 0;
  while( i!= K) {
    i++;
    buffer.produce_B();
  }
}

void * ConsumerA(void*) {
  int i = 0;
  while( i!= K ) {
    i++;
    buffer.consume_A();
  }
}

void * ConsumerB(void*) {
  int i = 0;
  while( i!= K ) {
   i++;
    buffer.consume_B();
    }
} 

////////////////////////////////////
////////////////////////////////////
int main(void) {
  srand(time(NULL));
   
  // zarodek do losowania sleepa
  srand(time(NULL));
  
  // pthread_t trzyma ID wątku po utworzeniu go za pomocą pthread_create()
	pthread_t t_id[4];

	//utworz watek ProducerA
	pthread_create(&(t_id[0]), NULL, ProducerA, (void*)0 );
	pthread_create(&(t_id[1]), NULL, ProducerB, (void*)1 );

	//utworz watki konsumentow
	pthread_create(&(t_id[2]), NULL, ConsumerA, (void*)2 );
	pthread_create(&(t_id[3]), NULL, ConsumerB, (void*)3 );

	//czekaj na zakonczenie watkow
	int i;
	for ( i = 0; i < 4; i++)
    pthread_join(t_id[i],(void **)NULL);
	return 0;
}
// dodanie warunków sprawdzania do konsumentów -> to samo co w producencie
// w producencie A, przy B_asks sprawdzić, czy może wstawić 3 elementy
// producencie 169, 170 na monitorach nie będzie działało, i A może prześcignąć 