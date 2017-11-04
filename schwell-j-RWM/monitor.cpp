#ifndef MONITOR_CPP
#define MONITOR_CPP
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

class Monitor{

private:
	int numReading, numWaitingRs;
	int numWriting, numWaitingWs;
	pthread_cond_t canRead, canWrite;
	pthread_mutex_t monitorMutex = PTHREAD_MUTEX_INITIALIZER;
	std::string data;

	std::string timeToString(long ms){
		int mm = ms%1000;
		int ss = ms/1000;
		return ss + ":" + mm;
	}

public:
	Monitor()
	{
	    numReading = 0;
		numWaitingRs = 0;
		numWriting = 0;
		numWaitingWs = 0;
		data = timeToString(get_CPU_time());
	}

	std::string getDBValue(){
		return data;
	}

	std::string updateDBValue(){
		data = timeToString(get_CPU_time());
		return data;
	}

	void beginRead(){
		pthread_mutex_lock(&monitorMutex);
		// If any writers are currently writing or waiting to write, do not enter
		while ((numWriting > 0) || (numWaitingWs > 0)){
			numWaitingRs++;
			pthread_cond_wait(&canRead, &monitorMutex);
			numWaitingRs--;
		}
		// Increment the number of readers, let other readers know they can enter,
		// then release the lock.
		numReading++;
		pthread_cond_signal(&canRead);
		pthread_mutex_unlock(&monitorMutex);
		/************ READ DATA **********************/
	}

	void endRead(){
		pthread_mutex_lock(&monitorMutex);
		numReading--;
		// Begin reading signals additional readers to enter if no writers are waiting
		// so endRead only needs to signal writers (once all readers finish)
		if (numReading == 0){
			pthread_cond_signal(&canWrite);
		}
		pthread_mutex_unlock(&monitorMutex);
	}

	void beginWrite(){
		pthread_mutex_lock(&monitorMutex);
		// If any writers are writing or any readers are reading
		while ((numWriting > 0) || (numWaitingRs > 0)){
			numWaitingWs++;
			pthread_cond_wait(&canWrite, &monitorMutex);
			numWaitingWs--;
		}
		numWriting++;
		pthread_mutex_unlock(&monitorMutex);
	}

	void endWrite(){
		pthread_mutex_lock(&monitorMutex);
		numWriting--;
		// Since readers waiting to enter prevent additional readers from entering
		if (numWaitingRs > 0){
			pthread_cond_signal(&canRead);
		}
		else{
			pthread_cond_signal(&canWrite);
		}
		pthread_mutex_unlock(&monitorMutex);
	}
};
#endif
