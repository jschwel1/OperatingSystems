/**
* Note pthreads headers are in C:\Temp/pthreads
*/
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#ifdef WIN32
#include <time_functions.h>
#include <pthread.h>
#include <semaphore.h>
#else
#include <pthread.h>
#include <semaphore.h>
#endif

#define BUFFER_SIZE 1024

void* readFrom(void* fromFile);
void* writeTo(void* toFile);

// Global Variables
char buffer[BUFFER_SIZE];
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
	sem_init(&emptySem, 0, BUFFER_SIZE);
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
	if (argc != 3) {
		printf("Call with %s <file_from> <file_to>\n", argv[0]);
		return 1;
	}
	strncat(fileFromPath, argv[1], sizeof(fileFromPath));
	strncat(fileToPath, argv[2], sizeof(fileToPath));

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

	fclose(fileFrom);
	fclose(fileTo);

	return 0;
}

void* readFrom(void* fromFile) 
{
	FILE* file = (FILE*)fromFile;
	int idx = 0;
	int ch;
	while ((ch = fgetc(file)) != EOF)
	{
		sem_wait(&emptySem);
		sem_wait(&inUseSem);
		buffer[idx] = ch;
		sem_post(&inUseSem);
		sem_post(&fullSem);

		idx = (idx+1)%BUFFER_SIZE;
	}
	doneReadingFile = true;
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
		fputc(buffer[idx], file);
		sem_post(&inUseSem);
		sem_post(&emptySem);
		sem_getvalue(&fullSem, &fullSemVal);
		idx = (idx+1)%BUFFER_SIZE;
	}
	return NULL;
}

