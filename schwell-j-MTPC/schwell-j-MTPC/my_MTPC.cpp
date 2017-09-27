/**
* Note pthreads headers are in C:\Temp/pthreads
*/
#include <iostream>
#ifdef WIN32
#include <time_functions.h>
#include <pthread.h>
#include <semaphore.h>
#else
#include <pthread.h>
#endif

int main() {
	sem_t x;
	char in;
	std::cout << "Hello, world!" << std::endl;
	std::cin >> in;

	return 0;
}

/*
	get time_functions.h and put in pthreads folder (and #include)

	else
	before include including time_functions.h include <time.h>

	Also,
	move move pthreads folder to C:\Temp\pthreads
*/