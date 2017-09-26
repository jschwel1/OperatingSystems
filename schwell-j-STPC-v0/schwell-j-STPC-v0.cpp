#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "time_functions.h"

int copyFile(char * source, char * dest, uint8_t mode);
void copyCharByChar(FILE * source, FILE * dest);
void copyLineByLine(FILE * source, FILE * dest);

int main(int argc, char ** argv)
{
	char filePath[128] = "";
	char fromFile[256];
	char toFile[256];
	int mode;

	#ifdef WIN32
	strcpy(filePath, "C:\\temp\\coursedata\\");
	#else
	strcpy(filePath, getenv("HOME"));
	strcat(filePath, "/temp/coursedata/");
	#endif // WIN32

	
	strcpy(fromFile, filePath);
	strcpy(toFile, filePath);
	if (argc != 3)
	{
		char temp[128];
		printf("Call function as %s <source_file> <dest_file>\n", argv[0]);
		printf("Enter the source file: ");
		std::cin >> temp;
		strcat(fromFile, temp);
		printf("Enter the destination file: ");
		std::cin >> temp;
		strcat(toFile, temp);
	}
	else
	{
		strcat(fromFile, argv[1]);
		strcat(toFile, argv[2]);
	}
	printf("Copying %s to %s\n", fromFile, toFile);
	printf("Would you like to copy character-by-character (0) or line-by-line (1)? ");
	std::cin >> mode;
	
	return copyFile(fromFile, toFile, mode);;
}

int copyFile(char * source, char * dest, uint8_t mode)
{
	// open both files
	FILE * srcFile = fopen(source, "r");
	if (srcFile == NULL)
	{
		printf("%s could not be opened. Check that it exists\n", source);
		return 1;
	}
	FILE * destFile = fopen(dest, "w");
	if (destFile == NULL)
	{
		printf("%s could not be opened.\n", dest);
		return 1;
	}
	// Copy file
	start_timing();
	(mode == 0) ? copyCharByChar(srcFile, destFile)
				: copyLineByLine(srcFile, destFile);
	stop_timing();
	printf("Wall clock time to copy: %fs\n", get_wall_clock_diff());
	printf("CPU time to copy: %fs\n", get_CPU_time_diff());
	// Close file
	fclose(destFile);
	fclose(srcFile);

	return 0;
}

void copyCharByChar(FILE * source, FILE * dest)
{
	// Read & write each Byte
	int ch;
	while ((ch = fgetc(source)) != EOF)
	{
		fputc(ch, dest);	
	}
}

void copyLineByLine(FILE * source, FILE * dest)
{
	char line[1024];
	while (fgets(line, sizeof(line), source) != NULL)
	{
		fputs(line, dest);
	}
}
