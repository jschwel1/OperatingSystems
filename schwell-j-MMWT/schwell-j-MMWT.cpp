#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#ifdef WIN32
#include <time_functions.h>
#else
#include "time_functions.h"
#endif

#include <pthread.h>
#include <semaphore.h>

#ifdef WIN32
#define FILE_IN "C:\\temp\\coursein\\matrixin.txt"
#define FILE_OUT "C:\\temp\\courseout\\matrixout.txt"
#else
#define FILE_IN "/temp/coursein/matrixin.txt"
#define FILE_OUT "/temp/courseout/matrixout.txt"
#endif

typedef struct {
	int a;
	int b;
	int result;
} pthreadArgs;

int** readMatrixFromFile(FILE* file, int* rows, int* cols);
void freeMatrix(int** mat, int rows);
void* multiply(void* args);

int main(int argc, char** argv){
    FILE* fileIn;
    FILE* fileOut;
    int** matrix1 = NULL;
    int** matrix2 = NULL;
	int m1Rows, m1Cols, m2Rows, m2Cols;
    int i, j, k;

    // open file
	char fileInPath[256];
	char fileOutPath[256];
	#ifdef WIN32
	strncpy(fileInPath, FILE_IN, 256);
	strncpy(fileOutPath, FILE_OUT, 256);
	#else
	strncpy(fileInPath, getenv("HOME"), 256);
	strncpy(fileOutPath, getenv("HOME"), 256);
	strncat(fileInPath, FILE_IN, 256);
	strncat(fileOutPath, FILE_OUT, 256);
	#endif
    if ((fileIn=fopen(fileInPath, "r")) == NULL){
        printf("ERROR: Could not open file %s\n", fileInPath);
        return 1;
    }
	if ((fileOut=fopen(fileOutPath, "w")) == NULL){
        printf("ERROR: Could not open file %s\n", fileOutPath);
        return 1;
    }

    matrix1 = readMatrixFromFile(fileIn, &m1Rows, &m1Cols);
    matrix2 = readMatrixFromFile(fileIn, &m2Rows, &m2Cols);
	fclose(fileIn);
	if (m1Cols != m2Rows){
		printf("Invalid dimensions!\n");
		return 1;
	}
	
	// Calculate # multiplies needed -- dimension of result = m1Rows x m2Cols
	int numMultiplies = m1Cols*m1Rows*m2Cols; 
	pthreadArgs* args = (pthreadArgs*)malloc(sizeof(pthreadArgs)*numMultiplies);
	pthread_t* pthreads = (pthread_t*)malloc(sizeof(pthread_t)*numMultiplies);
	for (i = 0; i < m1Rows; i++){
		for (j = 0; j < m2Cols; j++){
			for (k = 0; k < m1Cols; k++){
				int idx = i*m1Cols*m2Cols+m1Cols*j+k;
				args[idx].a = matrix1[i][k];
				args[idx].b = matrix2[k][j];
				pthread_create(&pthreads[idx], NULL, multiply, (void*)&args[idx]);
				pthread_join(pthreads[idx], NULL);
			}
		}
	}

	// build and print result matrix
	char output[128];
	for (i = 0; i < m1Rows; i++){
		for (j = 0; j < m2Cols; j++){
			int sum = 0;
			for (k = 0; k < m1Cols; k++){
				int idx = i*m1Cols*m2Cols+m1Cols*j+k;
				sum+= args[idx].result;		
			}
			snprintf(output, 128, "%4d ", sum);
			fputs(output, fileOut);
		}
		fputc('\n', fileOut);
	}
	fclose(fileOut);
	free(args);
	free(pthreads);
	freeMatrix(matrix1, m1Rows);
	freeMatrix(matrix2, m2Rows);
	return 0;
}

void freeMatrix(int** mat, int rows){
	int r;
	for (r = 0; r < rows; r++){
		free(mat[r]);
	}
	free(mat);
}

int** readMatrixFromFile(FILE* file, int* rows, int* cols){
    char buffer[256];
    int tempMatrix[128][128]; // Matrix cannot be more than 128x128 elements
    int row = 0;
    int col = 0;
	int** mat;

    while(fgets(buffer, sizeof(buffer), file) != NULL){
        if (buffer[0] == '*'){
			break;
        }
        col = 0;
        size_t idx = 0;
        for (idx = 0; idx < strlen(buffer); idx++){
            if (buffer[idx] >= '0' && buffer[idx] <= '9'){
                tempMatrix[row][col++]=atoi((const char*)&buffer[idx]);
            }
        }
        row++;
    }
	mat = (int**)malloc(sizeof(int*)*row);
	int r;
	for (r = 0; r < row; r++){
		mat[r] = (int*)malloc(sizeof(int)*col);
		int c;
		for (c = 0; c < col; c++){
			mat[r][c] = tempMatrix[r][c];
		}
	}
	*rows = row;
	*cols = col;
	return mat;
}

void* multiply(void* args){
	pthreadArgs* arg = (pthreadArgs*)args;
	arg->result = arg->a * arg->b;
	return NULL;
}


