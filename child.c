#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <windows.h>

#define NO_OF_THREADS 4  //the number of item_types
#define BUFSIZE 128
#define FILENAME "market.txt"

typedef struct
{
	int milk_count;
	int biscuit_count;
	int chips_count;
	int coke_count;
} DAY_REPORT;

typedef struct
{
	const char* searchItem;
	int item_count;
	int threadNo;
	char *dayToRead;
}THREAD_PARAMETERS;

DWORD WINAPI threadWork(LPVOID);

//global variable for absolute path to the FILENAME
char FILEPATH[BUFSIZ]; 

//used to obtain full path to the FILENAME
void getFullPath(void);

//used when reading the file
void cleanBuffer(char*);

int main(int argc, char* argv[])
{
	//get the absolute file path to the FILENAME 
	getFullPath();

	//declare variables for threads
	HANDLE* handles;
	THREAD_PARAMETERS* lpParameters;
	DWORD* threadID;
	DAY_REPORT dr;
	int i = 0;

	//getDayInfo
	CHAR dayToRead[2];
	DWORD dwRead, dwWritten;
	HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
	HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
	//Read day info from standard input
	if (!ReadFile(hStdin, dayToRead, 2, &dwRead, NULL))
	{
		fprintf(stderr, "error reading day info\n");
		ExitProcess(-1);
	}
	// Write to standard output 
	if (!WriteFile(hStdout, dayToRead, dwRead, &dwWritten, NULL))
	{
		fprintf(stderr, "error writing day info\n");
		ExitProcess(-1);
	}

	//allocate memory for every parameters needed
	handles = (HANDLE*)malloc(sizeof(HANDLE)* NO_OF_THREADS);
	lpParameters = (THREAD_PARAMETERS*)malloc(sizeof(THREAD_PARAMETERS)* NO_OF_THREADS);
	threadID = (DWORD*)malloc(sizeof(int)* NO_OF_THREADS);

	//initialize thread parameters for each thread
	lpParameters[0].searchItem = "MILK";
	lpParameters[1].searchItem = "BISCUIT";
	lpParameters[2].searchItem = "CHIPS";
	lpParameters[3].searchItem = "COKE";

	//for each thread
	for (i = 0; i < NO_OF_THREADS; i++)
	{
		//initialize other parameters
		lpParameters[i].item_count = 0;
		lpParameters[i].threadNo = i + 1;
		lpParameters[i].dayToRead = dayToRead;

		handles[i] = CreateThread(NULL, 0, threadWork, &lpParameters[i], 0, &threadID[i]);

		//check errors in creation
		if (handles[i] == INVALID_HANDLE_VALUE)
		{
			fprintf(stderr, "error creating threads\n");
			exit(0);
		}

	}

	WaitForMultipleObjects(NO_OF_THREADS, handles, TRUE, INFINITE);

	//extracting info from each thread params and storing in DAY_REPORT structure
	dr.milk_count = lpParameters[0].item_count;
	dr.biscuit_count = lpParameters[1].item_count;
	dr.chips_count = lpParameters[2].item_count;
	dr.coke_count = lpParameters[3].item_count;

	//send day report to parent process
	if (!WriteFile(hStdout, &dr, sizeof(DAY_REPORT), &dwWritten, NULL))
		fprintf(stderr, "error writing day report %s", dayToRead);


	//close thread handles
	for (i = 0; i < NO_OF_THREADS; i++) {
		if (!CloseHandle(handles[i]))
			fprintf(stderr, "error closing the handle\n");
	}

	//free allocated memory
	free(handles);
	free(lpParameters);
	free(threadID);

	return 1;
}

DWORD WINAPI threadWork(LPVOID parameters)
{
	//cast parameters to THREAD_PARAMETERS
	THREAD_PARAMETERS *pThreadParams = (THREAD_PARAMETERS*)parameters;
	FILE *fpTransactions;

	//open the file
	fopen_s(&fpTransactions, FILEPATH, "r");
	if (fpTransactions == NULL)
		fprintf(stderr, "error opening the file %s from threadNo %d\n",
			FILEPATH, pThreadParams->threadNo);

	/*read until reading the char ',' then
	store the remaining characters in the buffer
	untill seeing the char ',' again
	then check whether it contains the substirng MILK or
	CHIPS or etc */
	char chBuf[BUFSIZ];
	char currentChar;
	int i = 0;

	//find the day segment to read in market.txt
	BOOL bDayFound = FALSE;
	do
	{
		currentChar = getc(fpTransactions);
		if (currentChar == EOF)
			break;
		if (currentChar == '#')
			do
			{
				currentChar = getc(fpTransactions);
				if ((char)currentChar != '#')
					chBuf[i++] = currentChar;
				else
				{
					chBuf[i++] = '\0';
					if (strstr(chBuf, pThreadParams->dayToRead) != NULL)
					{
						cleanBuffer(chBuf);
						i = 0;
						bDayFound = TRUE;
						break;
					}
					else {
						i = 0;
						cleanBuffer(chBuf);
						break;
					}
				}
			} while (1);
	} while (!bDayFound);

	//find the number of items sold in that day
	int nItemsFound = 0;
	do
	{
		currentChar = getc(fpTransactions);
		if ((char)currentChar == -1)
			break;
		if ((char)currentChar == '#')
			break;

		if (currentChar != ',')
			chBuf[i++] = currentChar;
		else
		{
			chBuf[i++] = '\0';
			if (strstr(chBuf, pThreadParams->searchItem) != NULL)
			{
				nItemsFound++;
				i = 0;
				cleanBuffer(chBuf);
			}
			else
			{
				i = 0;
				cleanBuffer(chBuf);
			}
		}
	} while (1);

	//close the file
	fclose(fpTransactions);

	//store the item_count in pThreadParams
	pThreadParams->item_count = nItemsFound;

	return 0;
}

void cleanBuffer(char* buffer) {
	ZeroMemory(buffer, BUFSIZ);
}

void getFullPath()
{
	GetFullPathNameA(FILENAME, BUFSIZ, FILEPATH, NULL);
	int i = 0;
	int j = 0;
	char val[BUFSIZ];
	while (FILEPATH[i])
	{
		if (FILEPATH[i] == '\\') {
			val[j++] = FILEPATH[i];
			val[j++] = '\\';
		}
		else
			val[j++] = FILEPATH[i];

		i++;
	}
	val[j] = '\0';
	i = 0;
	while (val[i])
		FILEPATH[i++] = val[i];
};