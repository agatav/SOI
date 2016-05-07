#include "monitor.h"
#include <iostream>
#include <cstdlib>
#include <deque>
#include <pthread.h>
#include <time.h>

#define SIZE 9

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
    bool forbid_B  = false;
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
		buffer.ToBegin(product);
    cout << "Dodano element: " << product << endl;
		cout << "Bufor: " << buffer.size() << endl;
    if (buffer.size() > 3) {
        if (buffer.size() > 4) {
           cout << "Bufor > 4 : " << buffer.size() << endl;
            signal(sem_bigger_than_4);
        }
      cout << "Bufor > 3 : " << buffer.size() << endl;
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
    buffer.ToBegin(product);
    cout << "Dodano element: " << product << endl;
    cout << "Bufor: " << buffer.size() << endl;
      }

    if (buffer.size() > 3) {
        if (buffer.size() > 4) {
           cout << "Bufor > 4 : " << buffer.size() << endl;
            signal(sem_bigger_than_4);
        }
      cout << "Bufor > 3 : " << buffer.size() << endl;
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
    buffer.DeleteTail();
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
    buffer.DeleteTail();
    cout << "Bufor: " << buffer.size() << endl;
    }
 
if (buffer.size() > 3) {
        if (buffer.size() > 4) {
           cout << "Bufor > 4 : " << buffer.size() << endl;
            signal(sem_bigger_than_4);
        }
      cout << "Bufor > 3 : " << buffer.size() << endl;
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
Buffer bufor;

void * ProducerA(void*) {
  int product;
  while ( 1 ) {
    bufor.produce_A();
    //sleep(rand()%10);
  }
}

void * ProducerB(void*) {
  while( 1 ) {
    bufor.produce_B();
    //sleep(rand()%10);
  }
}

void * ConsumerA(void*) {
  while( 1 ) {
    bufor.consume_A();
    //sleep(rand()%10);
  }
}

void * ConsumerB(void*) {
  while( 1 ) {
    bufor.consume_B();
    //sleep(rand()%10);
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