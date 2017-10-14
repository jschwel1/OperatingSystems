#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "time_functions.h"
#include <pthread.h>
#include <semaphore.h>

#define FILE_PATH "matrix.txt"

typedef struct {
	int a;
	int b;
	int result;
} pthreadArgs;

int** readMatrixFromFile(FILE* file, int* rows, int* cols);
void freeMatrix(int** mat, int rows);
void* multiply(void* args);

int main(int argc, char** argv){
    FILE* dataFile;
    int** matrix1 = NULL;
    int** matrix2 = NULL;
	int** matrixResult = NULL;
	int m1Rows, m1Cols, m2Rows, m2Cols;
    
    // open file
    if ((dataFile=fopen(FILE_PATH, "r")) == NULL){
        printf("ERROR: Could not open file %s\n", FILE_PATH);
        return 1;
    }
    matrix1 = readMatrixFromFile(dataFile, &m1Rows, &m1Cols);
    matrix2 = readMatrixFromFile(dataFile, &m2Rows, &m2Cols);
	fclose(dataFile);
    int i, j, k;
    for (i = 0; i < m1Rows; i++){
		printf("| ");
        for (j = 0; j < m1Cols; j++){
            printf("%d ", matrix1[i][j]);
        }
		printf("|\n");
    }
	printf("X\n");
    for (i = 0; i < m2Rows; i++){
		printf("| ");
        for (j = 0; j < m2Cols; j++){
            printf("%d ", matrix2[i][j]);
        }
		printf("|\n");
    }

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
	matrixResult = (int**)malloc(sizeof(int*)*m1Rows);
	printf("=\n");
	for (i = 0; i < m1Rows; i++){
		matrixResult[i] = (int*)malloc(sizeof(int)*m2Cols);
		printf("|");
		for (j = 0; j < m2Cols; j++){
			int sum = 0;
			for (k = 0; k < m1Cols; k++){
				int idx = i*m1Cols*m2Cols+m1Cols*j+k;
				sum+= args[idx].result;		
			}
			matrixResult[i][j] = sum;
			printf("%4d ", sum);
		}
		printf("|\n");
	}

	free(args);
	free(pthreads);
	freeMatrix(matrixResult, m1Rows);
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
    char buffer[1024];
    int tempMatrix[1024][1024]; // Matrix cannot be more than 1024x1024
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
            if (isdigit(buffer[idx])){
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


