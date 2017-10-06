/**
* Note pthread headers are in C:\Temp\pthreads for windows
*/
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// The location of time_functions.h will either be in the pthreads folder (windows) or in the compiling folder (linux)
#ifdef WIN32
#include <time_functions.h>
#else
#include "time_functions.h"
#endif

#include <pthread.h>
#include <semaphore.h>

// Get the file paths according to the OS
#ifdef WIN_32
#define FILE_IN "C:\\temp\\coursein\\p2-in.txt"
#define FILE_OUT "C:\\temp\\courseout\\p2-out.txt"
#else
#define FILE_IN "/temp/coursein/p2-in.txt"
#define FILE_OUT "/fileio/p2-out.txt"
#endif

#define NUM_BUFFERS 10
#define BUFFER_SIZE 1024

void* readFrom(void* fromFile);
void* writeTo(void* toFile);

// Global Variables
char buffer[NUM_BUFFERS][BUFFER_SIZE];
sem_t inUseSem;
sem_t emptySem;
sem_t fullSem;
char doneReadingFile;

int main(int argc, char **argv)
{
	FILE* fileFrom;
	FILE* fileTo;
	sem_init(&inUseSem, 0, 1);
	sem_init(&emptySem, 0, NUM_BUFFERS);
	sem_init(&fullSem, 0, 0);
	pthread_t producerThread;
	pthread_t consumerThread;
	doneReadingFile = false;	

    // open the files 
	fileFrom = fopen(FILE_IN, "r");
    fileTo = fopen(FILE_OUT, "w");
	
    // Ensure both files opened correctly
	if (fileFrom == NULL)
	{
		printf("Could not open %s\n", FILE_IN);
		return 1;
	}	
	if (fileTo == NULL)
	{
		printf("Could not open %s\n", FILE_OUT);
		return 1;
	}
	
	start_timing();
    // start threads
    if (pthread_create(&producerThread, NULL, readFrom, (void*)fileFrom))
	{
        // If thread was not created, exit program
		printf("Error creating producer thread.\n");
		return 1;
	}
	if (pthread_create(&consumerThread, NULL, writeTo, (void*)fileTo))
	{
        // If consumer thread could not be created, wait for producer thread to complete, then exit program
		printf("Error creating consumer thread.\n");
		printf("Waiting for producer thread to join.\n");
		if (pthread_join(producerThread, NULL))
		{
			printf("Error joining producer.\n");
		}
		return 1;
	}

    // When the producer thread completes, it should join here
	if (pthread_join(producerThread, NULL))
	{
		printf("Error joining producer.\n");
		return 1;
	}
    // When the consumer thread complete, it should join here
	if (pthread_join(consumerThread, NULL))
	{
		printf("Error joining consumer.\n");
		return 1;
	}
    // Stop the timer, close the files and print the results
	stop_timing();
	fclose(fileFrom);
	fclose(fileTo);
	printf("Wall time: %fs\n", get_wall_clock_diff());
	printf("CPU time:  %fs\n", get_CPU_time_diff());
	return 0;
}

/**
* Producer thread
* void* readFrom()
* void* fromFile - FILE* of the file stream to read from
*/
void* readFrom(void* fromFile) 
{
	FILE* file = (FILE*)fromFile;
	int idx = 0;
    // Loop as long as there are lines to be read
	while (!doneReadingFile)
	{
        // Make sure that there is at least 1 empty buffer slot to put another line into
        // and make sure the consumer thread is not in it's critical area
		sem_wait(&emptySem);
		sem_wait(&inUseSem);
		// Read one line (up to \n, EOF, or BUFFER_SIZE) into the current slot in the buffer.
        // If the function returns NULL, nothing was read into the buffer, so the file is done being read.
		if ((fgets(buffer[idx], BUFFER_SIZE, file)) == NULL)
		{
            // If the file is done, set doneReadingFile and undo the semaphores
			doneReadingFile = true;
			sem_post(&inUseSem);
			sem_post(&emptySem);
			return NULL;
		}
        // Exit the critical section and update the # full slots semaphore
		sem_post(&inUseSem);
		sem_post(&fullSem);
        // Increment the buffer index mod NUM_BUFFERS (# buffer slots)
		idx = (idx+1)%NUM_BUFFERS;
	}
	return NULL;
}

/**
* Consumer Thread
* void* writeTo()
* void* toFile - FILE* of the file stream to write to
*/
void* writeTo(void* toFile)
{
	FILE* file = (FILE*)toFile;
	int idx = 0;
	int fullSemVal;
    // Try writing lines from the buffer as long as the producer is still reading
    // the file and there are lines to be written.
	while((!doneReadingFile) || (fullSemVal > 0)) 
	{
		// Wait until there is at least one buffer slot to write,
        // then make sure the other thread is not in the critical section
        sem_wait(&fullSem);
		sem_wait(&inUseSem);
        // Write the current buffer slot (line) to the file.
		fputs(buffer[idx], file);
		// Exit the critical section and update the # empty buffer slots
        sem_post(&inUseSem);
		sem_post(&emptySem);
		// Get the current number of full buffer slots and store it in fullSemVal
        // so the while loop can check that there are still items to be read after
        // the producer completed.
        sem_getvalue(&fullSem, &fullSemVal);
        // Increment the counter mod NUM_BUFFERS
		idx = (idx+1)%NUM_BUFFERS;
	}
	return NULL;
}

