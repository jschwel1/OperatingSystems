#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "time_functions.h"
#include <pthread.h>
#include <semaphore.h>

#define FILE_PATH "matrix.txt"

void readMatrixFromFile(FILE* file, int** mat1, int** mat2);

int main(int argc, char** argv){
    FILE* dataFile;
    int** matrix1 = NULL;
    int** matrix2 = NULL;
    
    // open file
    if ((dataFile=fopen(FILE_PATH, "r")) == NULL){
        printf("ERROR: Could not open file %s\n", FILE_PATH);
        return 1;
    }
    readMatrixFromFile(dataFile, matrix1, matrix2);

    size_t i, j;
    for (i = 0; i < sizeof(matrix1)/sizeof(matrix1[0]); i++){
        for (j = 0; j < sizeof(matrix1[0]); j++){
            printf("%d ", matrix1[i][j]);
        }
    }
    for (i = 0; i < sizeof(matrix2)/sizeof(matrix2[0]); i++){
        for (j = 0; j < sizeof(matrix2[0])/sizeof(matrix2[0][0]); j++){
            printf("%d ", matrix2[i][j]);
        }
    }

}

void readMatrixFromFile(FILE* file, int** mat1, int** mat2){
    char buffer[1024];
    int tempMatrix[1024][1024]; // Matrix cannot be more than 1024x1024
    int row = 0;
    int col = 0;

    while(fgets(buffer, sizeof(buffer), file) != NULL){
        if (buffer[0] == '*'){
            printf("Finished Matrix1: %dx%d\n", row, col);
            mat1 = (int**)malloc(sizeof(int*)*row);
            int r;
            for (r = 0; r < row; r++){
                mat1[r] = (int*)malloc(sizeof(int)*col);
                int c;
                for (c = 0; c < col; c++){
                    mat1[r][c] = tempMatrix[r][c];
                }
            }
            row = 0;
            col = 0;
            continue;
        }
        col = 0;
        size_t idx = 0;
        for (idx = 0; idx < strlen(buffer); idx++){
            if (isdigit(buffer[idx])){
                tempMatrix[row][col++]=(int)(buffer[idx]);
            }
        }
        row++;
    }
    printf("Finished Matrix2: %dx%d\n", row, col);
    mat2 = (int**)malloc(sizeof(int*)*row);
    col = 0;
    int r;
    for (r = 0; r < row; r++){
        mat2[r] = (int*)malloc(sizeof(int)*col);
        int c;
        for (c = 0; c < col; c++){
            mat2[r][c] = tempMatrix[r][c];
        }
    }
}




