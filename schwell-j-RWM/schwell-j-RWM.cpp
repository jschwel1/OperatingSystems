#include <iostream>
#include <fstream>
#include <string>
#include <stdio.h>
#include <stdlib.h>

// comment next line to get CPU time for the database value
// or leave commented (or delete) to use wall clock time
#define USE_CPU_TIME

#ifdef WIN32
#include <time_functions.h>
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
* This class defines the monitor and all its required functions to 
* allow synchronization and mutual exclusion for readers and writers.
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

    // This function returns a formatted version of the time
    // (used by writers to update the data)
    std::string timeToString(){
        char outstr[128];
        int ss, mm;
        #ifdef USE_CPU_TIME
        long ms = get_CPU_time();
        mm = ms%1000;
        ss = (ms/1000)%1000; // truncate to 3 digits so it looks neater
        #else
        get_wall_time_ints(&ss, &mm);
        #endif
        snprintf(outstr, 128, "%03d:%03d", ss, mm);
        return std::string(outstr);
    }

public:
    Monitor(){
        numReaders = 0;
        waitingRs = 0;
        writers = 0;
        data = timeToString();
    }
    // Returns the current data value (used by readers)
    std::string getDBValue(){
        return data;
    }

    // Updates the data and returns the new value (used by writers)
    std::string updateDBValue(){
        data = timeToString();
        return data;
    }

    /**
    *    Function: beginRead()
    *     Description: This function should be called by readers before they read the data and write 
    *                 to the output file. It will prevent reader threads from accessing the data
    *                 while a writer is currently there and prevent reading from trying to write
    *                 to the file at the same time.
    **/
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
    /** 
    *    Function: endRead()
    *     Description: This function should be called by read threads when they finish writing to the file.
    *                 If all threads in the critical section finish reading (some might be waiting for a 
    *                 writer to enter), it will signal that a waiting writer can enter.
    **/
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

    /**
    *     Function: beginWrite()
    *     Description: This function should be called by writer threads to ensure mutual exclusion in the
    *                 critical section. It prevents any writer from entering while any other thread is in
    *                 the critical section. Like beginRead, it uses condition variable to queue waiting threads
    *                 until the appropriate time.
    **/
    void beginWrite(){
        pthread_mutex_lock(&writeLock);
        // If any writers are writing or any readers are reading
        while (numReaders > 0){
            pthread_cond_wait(&canWrite, &writeLock);
        }
        writers++;
    } 

    /**
    *    Function: endWrite()
    *    Description: This function should be called by a writer threads once they finish with the critical section.
    *                 After a writer updates the data and writes to the file, it will signal that a reader can start
    *                 reading as long as one is waiting. Otherwise it will signal that the next writer can begin.
    **/
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

// This is the struct passed to threads as a parameter
struct argType{
    Monitor* monitor;
    int threadNum;
    int delay;
    std::fstream* fs;
};

// Thread function declarations
void* readThreadStart(void* arg);
void* writeThreadStart(void* arg);

/** 
*   Function: Main 
*   Description: This obtains the number of threads and their delays
*                from the user and starts the corresponding threads. Once all 
*                the threads complete, it joins them back up and returns.
**/
int main(int argc, char** argv){
    int numReaders, numWriters, readDelay, writeDelay;
    pthread_t* threads;
    argType* threadArgs;
    Monitor mon;

    // If the number of readers and writers along with their delays were not entered as arguments, ask for them
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
    
    // Get the correct filepath depending on OS
    #ifdef WIN32
    std::string filePath = FILE_PATH;
    #else
    std::string filePath = getenv("HOME");
    filePath+=FILE_PATH;
    #endif
    std::fstream fs;
    fs.open(filePath.c_str(), std::fstream::in | std::fstream::out | std::fstream::trunc);
    if (!fs.is_open()){
        std::cout << "ERROR: Could not open file " << FILE_PATH << std::endl;
        std::cout << "Hit <enter> to continue";
        fs.close();
        fgetc(stdin);
        return 1;
    }
    // Get the space for the threads and their arguments, then start the threads
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

    // Give memory back
    mon.~Monitor();
    fs.close();
    free(threads);
    free(threadArgs);
    std::cout << "Hit <enter> to continue";
    fgetc(stdin);
    return 0;
}

/**
*    Function: readThreadStart()
*    Description: This is the function that reader threads should enter from.
*                 It will loop ROUNDS times, trying to read, reading, writing
*                 to the file, leaving the critical section, then waiting the
*                 requested delay time before repeating.
*/
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

/**
*    Function: writeThreadStart()
*    Description: This is the function that writer threads should enter from.
*                 It will loop ROUNDS times, update and get the data, write it
*                 to the file, leaving the critical section, then waiting the
*                 requested delay time before repeating.
*/
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

