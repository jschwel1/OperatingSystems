/**
* Note pthread headers are in C:\Temp\pthreads for windows
*/
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>

#ifdef WIN32
#include <time_functions.h>
#else
#include "time_functions.h"
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
	char fileFromPath[512];
	char fileToPath[512];
	FILE* fileFrom;
	FILE* fileTo;
	sem_init(&inUseSem, 0, 1);
	sem_init(&emptySem, 0, NUM_BUFFERS);
	sem_init(&fullSem, 0, 0);
	pthread_t producerThread;
	pthread_t consumerThread;
	doneReadingFile = false;	

	printf("Created variable, starting stuff.\n");
	#ifdef WIN32
		strncpy(fileFromPath, "C:\\temp\\coursein\\", sizeof(fileFromPath));
		strncpy(fileToPath, "C:\\temp\\courseout\\", sizeof(fileToPath));
	#else
		strncpy(fileFromPath, getenv("HOME"), sizeof(fileFromPath));
		strncpy(fileToPath, getenv("HOME"), sizeof(fileToPath));
		strncat(fileFromPath, "/temp/coursein/", sizeof(fileFromPath));
		strncat(fileToPath, "/temp/courseout/", sizeof(fileToPath));
	#endif
	printf("created paths\n");	
	if (argc != 3) 
	{
		char fileIn[256];
		char fileOut[256];

		printf("Call with %s <file_from> <file_to>\n", argv[0]);
		printf("Enter source file name: %s", fileFromPath);
		std::cin >> fileIn;
		printf("Enter destination file name: %s", fileToPath);
		std::cin >> fileOut;
		strncat(fileFromPath, fileIn , sizeof(fileFromPath));
		strncat(fileToPath, fileOut, sizeof(fileToPath));

	}
	else 
	{
		strncat(fileFromPath, argv[1], sizeof(fileFromPath));
		strncat(fileToPath, argv[2], sizeof(fileToPath));
	}

	// open the files
	fileFrom = fopen(fileFromPath, "r");
	fileTo = fopen(fileToPath, "w");	
	if (fileFrom == NULL)
	{
		printf("Could not open %s\n", fileFromPath);
		return 1;
	}	
	if (fileTo == NULL)
	{
		printf("Could not open %s\n", fileToPath);
		return 1;
	}
	printf("successfuly opened files\n");
	
	// start threads
	start_timing();
	if (pthread_create(&producerThread, NULL, readFrom, (void*)fileFrom))
	{
		printf("Error creating producer thread.\n");
		return 1;
	}
	if (pthread_create(&consumerThread, NULL, writeTo, (void*)fileTo))
	{
		printf("Error creating consumer thread.\n");
		printf("Waiting for producer thread to join.\n");
		if (pthread_join(producerThread, NULL))
		{
			printf("Error joining producer.\n");
		}
		return 1;
	}

	if (pthread_join(producerThread, NULL))
	{
		printf("Error joining producer.\n");
		return 1;
	}
	if (pthread_join(consumerThread, NULL))
	{
		printf("Error joining consumer.\n");
		return 1;
	}
	stop_timing();
	fclose(fileFrom);
	fclose(fileTo);

	printf("Wall time: %fs\n", get_wall_clock_diff());
	printf("CPU time:  %fs\n", get_CPU_time_diff());
	return 0;
}

void* readFrom(void* fromFile) 
{
	FILE* file = (FILE*)fromFile;
	int idx = 0;
	while (!doneReadingFile)
	{
		sem_wait(&emptySem);
		sem_wait(&inUseSem);
		
		if ((fgets(buffer[idx], BUFFER_SIZE, file)) == NULL)
		{
			doneReadingFile = true;
			sem_post(&inUseSem);
			sem_post(&emptySem);
			break;
		}
		sem_post(&inUseSem);
		sem_post(&fullSem);
		idx = (idx+1)%NUM_BUFFERS;
	}
	return NULL;
}

void* writeTo(void* toFile)
{
	FILE* file = (FILE*)toFile;
	int idx = 0;
	int fullSemVal;
	while((!doneReadingFile) || (fullSemVal > 0)) 
	{
		sem_wait(&fullSem);
		sem_wait(&inUseSem);
		fputs(buffer[idx], file);
		sem_post(&inUseSem);
		sem_post(&emptySem);
		sem_getvalue(&fullSem, &fullSemVal);
		idx = (idx+1)%NUM_BUFFERS;
	}
	return NULL;
}

