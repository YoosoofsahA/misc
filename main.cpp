#include <iostream>
#include <thread>
#include <chrono>
#include <semaphore.h>
#include <cstdlib>
#include <pthread.h>
#include <ctime>

using namespace std;

// Circular Queue class defintion
class Queue {
	
	private:
		int front;
		int rear;
		int size;

		int *items; // Queue is integer array

	public:
		Queue(int _size) {
			front = -1;
			rear = -1;
			size = _size;

			items = new int[size];
		}
		
		~Queue() {
			delete[] items;
		}

		// Check if queue is full
		bool isFull() {
			if (front == 0 && rear == size - 1) {
				return true;
			}
			if (front == rear + 1) {
				return true;
			}
			return false;
		}
			
		// Check if queue is empty
		bool isEmpty() {
			if (front == -1) {
				return true;
			}
			return false;
		}

		// Add element to queue
		void enQueue(int element) {
			if (isFull()) {
				cout << "Queue is full" << endl;
				return;
			}
			else if (front == -1) {
				front = 0;
				rear = 0;
				items[rear] = element;
			}
			else if (rear == size - 1 && front != 0) {
				rear = 0;
				items[rear] = element;
			}
			else {
				rear++;
				items[rear] =  element;
			}
		}

		// Remove element from queue
		int deQueue() {
			int element;

			if (isEmpty()) {
				cout << "Queue is empty" << endl;
				return -1;
			}
			element = items[front];
			if (front == rear) {
				front = -1;
				rear = -1;
			}
			else if (front == size - 1) {
				front = 0;
			}
			else {
				front++;
			}

			return element;
		}

};

// Declare variables
int queueSize;
int numberOfJobs;

// Create data structures and variable	
Queue items(queueSize);

// Set up and initialise semaphores
pthread_mutex_t queueMutex; // Mutex sempahore for critical section
sem_t space; 
sem_t item;

// Producer and Consumer function declarations
void *producer(void*);
void *consumer(void*);

// Main function
int main (int argc, char** argv) {

	// Read in command line arguments
	cout << "Program name is: " << argv[0] << endl;
	
	// Check number of command line arguments is correct
	if (argc != 5) {
		cerr << "Error. Enter 4 command line arguments: " << argv[0] << "<queueSize>, <numberOfJobs>, <noProducers>, <noConsumers>" << endl;
		return 1;
	}
	if (argc == 1) {
		cout << "No Command Line Arguments passed" << endl;
	}
	if (argc > 1) {		
		cout << "\n---The following command line arguments were passed---" << endl;
		cout << "Queue size: " << argv[1] << endl;
		cout << "Number of Jobs: " << argv[2] << endl;
		cout << "Number of Producers: " << argv[3] << endl;
		cout << "Number of Consumers: " << argv[4] << endl;
		cout << "\n";
	}

	// Assign to variables 
	queueSize = atoi(argv[1]);
	numberOfJobs = atoi(argv[2]);
	const int noProducers = atoi(argv[3]);
	const int noConsumers = atoi(argv[4]);

	// Check inputs are logical
	if (queueSize <= 0 || numberOfJobs <= 0 || noProducers <= 0 || noConsumers <= 0) {
		cerr << "Invalid input parameters. All values must be greater than zero" << endl;
		return 1;
	}

	// Initialise semaphores
	if (pthread_mutex_init(&queueMutex, NULL) != 0) {
		cerr << "Error initialising mutex" << endl;
	}
	if (sem_init(&space, 0, queueSize) != 0) {
		cerr << "Error initialising space semaphore" << endl;
	}
	if (sem_init(&item, 0, 0) != 0) {
		cerr << "Error initialising item semaphore" << endl;
	}

	// Create producers and consumer threads 
	pthread_t producerThreads[noProducers];
	pthread_t consumerThreads[noConsumers];

	// Create IDs for threads
	int prodID[noProducers];
	for (int i = 0; i < noProducers; i++) {
		prodID[i] = i + 1;
	}
	int consID[noConsumers];
	for (int i = 0; i < noConsumers; i++) {
		consID[i] = i + 1;
	}

	// Create threads
	for (int j = 0; j < noProducers; j++) {
		if (pthread_create(&producerThreads[j], NULL, producer, (void *) &prodID[j]) != 0) {
			cerr << "Error creating producer threads" << endl;
		}
	}

	for (int k = 0; k < noConsumers; k++) {
		if (pthread_create(&consumerThreads[k], NULL, consumer, (void *) &consID[k]) != 0) {
			cerr << "Error creating consumer threads" << endl;
		}
	}

	// Join threads
	for (int l = 0; l < noProducers; l++) {
		if (pthread_join(producerThreads[l], NULL) != 0) {
			cerr << "Error joining producer threads" << endl;
		}
	}

	for (int m = 0; m < noConsumers; m++) {
		if(pthread_join(consumerThreads[m], NULL) != 0) {
			cerr << "Error joining consumer threads" << endl;
		}	
	}

	// Destroy semaphores
	if (pthread_mutex_destroy(&queueMutex) != 0) {
		cerr << "Error destroying mutex semaphore" << endl;
	}
	if (sem_destroy(&space) != 0) {
		cerr << "Error destroying space semaphore" << endl;
	}
	if (sem_destroy(&item) != 0) {
		cerr << "Error destroying item semaphore" << endl;
	}

	return 0;
}

void *producer(void *_producerID) {

	int producerID = *(int *) _producerID;

	while (true) {
			
		for (int i = 0; i < numberOfJobs; i++) {	
			
			// Produce item
			int job = (rand() % 10) + 1;
			
			// If queue is full then wait for 10 seconds then exit or continue			
			if (items.isFull()) {
				struct timespec timeout;
				clock_gettime(CLOCK_REALTIME, &timeout);
				timeout.tv_sec += 10;	
				int wait = sem_timedwait(&space, &timeout); 
				if (wait != 0) {
					cout << "Producer " << producerID << " timed out waiting for space in the queue" << endl;
					break;
				}
			}
			else { // If queue is not full then decrement semaphore 
				sem_wait(&space);
			}
			
			// Lock mutex to enter critical section
			pthread_mutex_lock(&queueMutex);			

			// Deposit item into queue
			items.enQueue(job);
			cout << "Producer " << producerID << " added item with duration: " << job << endl;

			// Unlock mutex
			pthread_mutex_unlock(&queueMutex);

			// Increment item semaphore, singalling there is an item in the queue
			sem_post(&item);
		}
	}
	// Exit the thread
	pthread_exit(0);
}

void *consumer(void *_consumerID) {
	
	int consumerID = *(int *) _consumerID;

	while (true) {
		
		// If queue is empty, wait for 10 seconds and quit if semaphore not decrememted
		if (items.isEmpty()) {
			struct timespec timeout;
			clock_gettime(CLOCK_REALTIME, &timeout);
			timeout.tv_sec += 10;	
			int wait = sem_timedwait(&item, &timeout); 
			if (wait != 0) {
				cout << "Consumer " << consumerID << " timed out waiting for new jobs" << endl;
				break;
			}
		}
		else { // If queue is not empty then decremement semaphore
			sem_wait(&item);
		}

		// Lock mutex so only one thread can pass
		pthread_mutex_lock(&queueMutex);

		// Fetch item from queue
		int job = items.deQueue();
		if (job > 10) {
			continue;
		}
		cout << "Consumer " << consumerID << " took item with duration: " << job << endl;

		// Unlock mutex
		pthread_mutex_unlock(&queueMutex);

		// Increment space semaphore, signalling there is extra space for producers
		sem_post(&space);

		// Consume item by sleeping for job duration
		cout << "Consumer " << consumerID << " executing sleep duration: " << job << endl;
		
		this_thread::sleep_for(chrono::seconds(job));

		cout << "Consumer " << consumerID << " job " << job << " completed" << endl;

	}
	
	// Exit the thread
	pthread_exit(0);
}

