#include <iostream>
#include <fstream>
#include <string>
#include <stdio.h>
#include <stdlib.h>


#ifdef WIN32
#include <time_functions.h>
#define HOME_DIR ""
#define FILE_PATH "C:\\Windows\\temp\\courseout\\p4-out.txt"
#else
#include "time_functions.h"
#define HOME_DIR getenv("HOME")
#define FILE_PATH "/temp/courseout/p4-out.txt"
#endif

#include <pthread.h>
#include <semaphore.h>

#define ROUNDS 10 

/** Monitor Class:
	This class defines the monitor and all its required functions to 
	allow synchronization and mutual exclusion for readers and writers.
**/
class Monitor{
private:
	int numReaders;
	int waitingRs;
	int writers;
	pthread_cond_t canRead = PTHREAD_COND_INITIALIZER;
	pthread_cond_t canWrite = PTHREAD_COND_INITIALIZER;
	pthread_mutex_t writeLock = PTHREAD_MUTEX_INITIALIZER;
	pthread_mutex_t fileIOLock = PTHREAD_MUTEX_INITIALIZER;
	std::string data;

	std::string timeToString(){
		char outstr[128];
		int ss, mm;
		get_wall_time_ints(&ss, &mm);
		snprintf(outstr, 128, "%d:%3d", ss, mm);
		return std::string(outstr);
	}

public:
	Monitor()
	{
	    numReaders = 0;
		waitingRs = 0;
		writers = 0;
		data = timeToString();
	}

	std::string getDBValue(){
		return data;
	}

	std::string updateDBValue(){
		data = timeToString();
		return data;
	}

	void beginRead(){
		pthread_mutex_lock(&writeLock);
		// If any writers are currently writing or waiting to write, do not enter
		while (writers > 0){
			waitingRs++;
			pthread_cond_wait(&canRead, &writeLock);
			waitingRs--;
		}
		// Increment the number of readers, let other readers know they can enter,
		// then release the lock.
		numReaders++;
		pthread_cond_signal(&canRead);
		pthread_mutex_unlock(&writeLock);
		pthread_mutex_lock(&fileIOLock);
	}

	void endRead(){
		pthread_mutex_unlock(&fileIOLock);
		pthread_mutex_lock(&writeLock);
		numReaders--;
		// Begin reading signals additional readers to enter if no writers are waiting
		// so endRead only needs to signal writers (once all readers finish)
		if (numReaders == 0){
			pthread_cond_signal(&canWrite);
		}
		pthread_mutex_unlock(&writeLock);
	}

	void beginWrite(){
		pthread_mutex_lock(&writeLock);
		// If any writers are writing or any readers are reading
		while (numReaders > 0){
			pthread_cond_wait(&canWrite, &writeLock);
		}
		writers++;
	} 
	void endWrite(){
		writers--;
		// Since readers waiting to enter prevent additional readers from entering
		if (waitingRs > 0){
			pthread_cond_signal(&canRead);
		}
		else{
			pthread_cond_signal(&canWrite);
		}
		pthread_mutex_unlock(&writeLock);
	}
};

/** argType
	This is the struct passed to threads as a parameter
**/
struct argType{
	Monitor* monitor;
	int threadNum;
	int delay;
	std::fstream* fs;
};

// Thread function declarations
void* readThreadStart(void* arg);
void* writeThreadStart(void* arg);

/** Main function
	This function obtains the number of threads and their delays
	from the user and starts the corresponding threads. Once all 
	the threads complete, it joins them back up and returns.
**/
int main(int argc, char** argv){
	int numReaders, numWriters, readDelay, writeDelay;
	pthread_t* threads;
	argType* threadArgs;
	Monitor mon;

	if (argc != 5){
		std::cout << "Run program as " << argv[0] << " <# readers> <# writers> <reader delay> <writer delay>" << std::endl;
		std::cout << "How many readers? ";
		std::cin >> numReaders;
		std::cout << "How many writers? ";
		std::cin >> numWriters;
		std::cout << "What is the delay for readers? ";
		std::cin >> readDelay;
		std::cout << "What is the delay for writers? ";
		std::cin >> writeDelay;
	}
	else {
		numReaders = atoi(argv[1]);
		numWriters = atoi(argv[2]);
		readDelay = atoi(argv[3]);
		writeDelay = atoi(argv[4]);
	}

	#ifdef WIN32
	std::string filePath = FILE_PATH;
	#else
	std::string filePath = getenv("HOME");
	filePath+=FILE_PATH;
	#endif
	std::fstream fs;
	fs.open(filePath.c_str(), std::fstream::in | std::fstream::out | std::fstream::app);
	if (!fs.is_open()){
		std::cout << "ERROR: Could not open file " << FILE_PATH << std::endl;
		return 1;
	}
	// Get the space for the threads and their arguments
	threads = (pthread_t*)malloc(sizeof(pthread_t)*(numReaders+numWriters));
	threadArgs = (argType*)malloc(sizeof(argType)*(numReaders+numWriters));
	
	int i;
	for (i = 0; i < numReaders+numWriters; i++){
		threadArgs[i].monitor = &mon;
		threadArgs[i].threadNum = (i < numReaders) ? i
										 : i - numReaders;
		threadArgs[i].delay = (i < numReaders) ? readDelay
								     : writeDelay;
		threadArgs[i].fs = &fs;
		pthread_create(&threads[i], NULL, ((i < numReaders)?readThreadStart
														   :writeThreadStart), (void*)&threadArgs[i]);
	}
	for (i = 0; i < numReaders+numWriters; i++){
		pthread_join(threads[i], NULL);
	}

	fs.close();
	free(threads);
	free(threadArgs);
	std::cout << "Hit <enter> to continue";
	fgetc(stdin);
	return 0;
}

void* readThreadStart(void* arg){
	argType* a = (argType*)arg;
	Monitor* monitor = a->monitor;
	int i;
	for (i = 0; i < ROUNDS; i++){
		monitor->beginRead();
		*(a->fs) << ">>> DB value read = " << monitor->getDBValue() << " by reader number " << a->threadNum << std::endl;
		monitor->endRead();
		millisleep(a->delay);
	}
	return NULL;
}

void* writeThreadStart(void* arg){
	argType* a = (argType*)arg;
	Monitor* monitor = a->monitor;
	int i;
	for (i = 0; i < ROUNDS; i++){
		monitor->beginWrite();
		*(a->fs) << "*** DB value set to " << monitor->updateDBValue() << " by writer number " << a->threadNum << std::endl;
		monitor->endWrite();
		millisleep(a->delay);
	}
	return NULL;
}

