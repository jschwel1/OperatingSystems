#include <iostream>
#include <string>
#include <stdio.h>
#include <stdlib.h>


#ifdef WIN32
#include <time_functions.h>
#else
#include "time_functions.h"
#endif

#include <pthread.h>
#include <semaphore.h>

#ifdef DEBUGGING
#define DEBUG(...) (__VA_ARGS__)
#else
#define DEBUG(...)
#endif

#define ROUNDS 3

class Monitor{

private:
	int numReading, numWaitingRs;
	int numWriting, numWaitingWs;
	pthread_cond_t canRead = PTHREAD_COND_INITIALIZER;
	pthread_cond_t canWrite = PTHREAD_COND_INITIALIZER;
	pthread_mutex_t monitorMutex = PTHREAD_MUTEX_INITIALIZER;
	std::string data;

	std::string timeToString(){
		DEBUG(std::cout << "timeToString passed: " << get_CPU_time() << std::endl);
		char outstr[128];
		long ms = get_CPU_time();
		int mm = ms%1000;
		int ss = ms/1000;
		snprintf(outstr, 128, "%d:%3d", ss, mm);
		return std::string(outstr);
	}

public:
	Monitor()
	{
	    numReading = 0;
		numWaitingRs = 0;
		numWriting = 0;
		numWaitingWs = 0;
		data = timeToString();
	}

	std::string getDBValue(){
		return data;
	}

	std::string updateDBValue(){
		data = timeToString();
		return data;
	}

	void beginRead(int i){
		pthread_mutex_lock(&monitorMutex);
		// If any writers are currently writing or waiting to write, do not enter
		while ((numWriting > 0) || (numWaitingWs > 0)){
			numWaitingRs++;
			std::cout << "Num Waiting Ws: " << numWaitingWs << "\t Currently Writing: " << numWriting << std::endl;
			pthread_cond_wait(&canRead, &monitorMutex);
			numWaitingRs--;
		}
		// Increment the number of readers, let other readers know they can enter,
		// then release the lock.
		numReading++;
		std::cout << "-------------- reader " << i << " entered ------" << std::endl;
		pthread_cond_signal(&canRead);
		pthread_mutex_unlock(&monitorMutex);
	}

	void endRead(int i){
		pthread_mutex_lock(&monitorMutex);
		numReading--;
		// Begin reading signals additional readers to enter if no writers are waiting
		// so endRead only needs to signal writers (once all readers finish)
		if (numReading == 0){
			pthread_cond_signal(&canWrite);
		}
		std::cout << "-------------- reader " << i << " done ------" << std::endl;
		pthread_mutex_unlock(&monitorMutex);
	}

	void beginWrite(int i){
		pthread_mutex_lock(&monitorMutex);
		// If any writers are writing or any readers are reading
		while ((numWriting > 0) || (numReading > 0)){
			numWaitingWs++;
			std::cout << "Writing: " << numWriting << "\tWs Waiting: " << numWaitingWs << "\tnumReading: " << numReading << "\t waiting Rs " << numWaitingRs << std::endl;
			pthread_cond_wait(&canWrite, &monitorMutex);
			numWaitingWs--;
		}
		numWriting++;
		std::cout << "-------------- writer " << i << " entered ------" << std::endl;
		pthread_mutex_unlock(&monitorMutex);
	} 
	void endWrite(int i){
		pthread_mutex_lock(&monitorMutex);
		numWriting--;
		// Since readers waiting to enter prevent additional readers from entering
		std::cout << "-------------- writer " << i << " done ------";
		if (numWaitingRs > 0){
			pthread_cond_signal(&canRead);
			std::cout << " --- signaled canRead " << std::endl;
		}
		else{
			pthread_cond_signal(&canWrite);
			std::cout << " --- signaled canWrite " << std::endl;
		}
		pthread_mutex_unlock(&monitorMutex);
	}
};

struct argType{
	Monitor* monitor;
	int threadNum;
	int delay;
};

void* readThreadStart(void* arg);
void* writeThreadStart(void* arg);

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

	threads = (pthread_t*)malloc(sizeof(pthread_t)*(numReaders+numWriters));
	threadArgs = (argType*)malloc(sizeof(argType)*(numReaders+numWriters));
	
	int i;
	for (i = 0; i < numReaders+numWriters; i++){
		threadArgs[i].monitor = &mon;
		threadArgs[i].threadNum = (i < numReaders) ? i
										 : i - numReaders;
		threadArgs[i].delay = (i < numReaders) ? readDelay
								     : writeDelay;
		pthread_create(&threads[i], NULL, ((i < numReaders)?readThreadStart
														   :writeThreadStart), (void*)&threadArgs[i]);
	}
	for (i = 0; i < numReaders+numWriters; i++){
		pthread_join(threads[i], NULL);
	}

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
//		DEBUG(std::cout << "Reader " << a->threadNum << " beginRead() -- round " << i << std::endl);
		monitor->beginRead(a->threadNum);
		std::cout << ">>> DB value read = " << monitor->getDBValue() << " by reader number " << a->threadNum << std::endl;
//		DEBUG(std::cout << "Reader " << a->threadNum << " endRead() -- round " << i << std::endl);
		monitor->endRead(a->threadNum);
		millisleep(a->delay);
	}
	return NULL;
}

void* writeThreadStart(void* arg){
	argType* a = (argType*)arg;
	Monitor* monitor = a->monitor;
	int i;
	for (i = 0; i < ROUNDS; i++){
		monitor->beginWrite(a->threadNum);
//		DEBUG(std::cout << "Writer " << a->threadNum << " beginWrite() -- round " << i << std::endl);
		std::cout << "*** DB value set to " << monitor->updateDBValue() << " by writer number " << a->threadNum << std::endl;
//		DEBUG(std::cout << "Writer " << a->threadNum << " endWrite() -- round " << i << std::endl);
		monitor->endWrite(a->threadNum);
		millisleep(a->delay);
	}
	return NULL;
}

