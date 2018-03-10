#include <windows.h> 
#include <tchar.h>
#include <stdio.h> 
#include <strsafe.h>

#define NO_OF_PROCESS 7 // the number of days
#define NO_OF_THREADS 4 // the number of item_types
#define BUFSIZE 128

typedef struct
{
	int milk_count;
	int biscuit_count;
	int chips_count;
	int coke_count;
} DAY_REPORT;

int main(int argc, char* argv[])
{
	STARTUPINFO si[NO_OF_PROCESS];
	PROCESS_INFORMATION pi[NO_OF_PROCESS];
	SECURITY_ATTRIBUTES sa;
	HANDLE processHandles[NO_OF_PROCESS];
	HANDLE hWriteSendingPipes[NO_OF_PROCESS]; 
	HANDLE hReadSendingPipes[NO_OF_PROCESS];
	HANDLE hWriteReceivingPipes[NO_OF_PROCESS];
	HANDLE hReadReceivingPipes[NO_OF_PROCESS];
	DAY_REPORT weekReport[NO_OF_PROCESS]; //each day's report is stored in this array
	int i = 0;

	//initializing security attribute parameter for pipes
	sa.bInheritHandle = TRUE;
	sa.lpSecurityDescriptor = NULL;
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);

	//creating pipes
	for (i = 0; i < NO_OF_PROCESS; i++)
	{
		//creating the sending pipes
		if (!CreatePipe(&hReadSendingPipes[i], &hWriteSendingPipes[i], &sa, 0))
		{
			printf("unable to create sending pipe %d \n", i + 1);
			system("pause");
			exit(0);
		}

		if (!SetHandleInformation(hWriteSendingPipes[i], HANDLE_FLAG_INHERIT, 0))
			printf("Error Stdout SetHandleInformation");

		//creating the receiving pipes
		if (!CreatePipe(&hReadReceivingPipes[i], &hWriteReceivingPipes[i], &sa, 0))
		{
			printf("unable to create receiving pipe %d \n", i + 1);
			system("pause");
			exit(0);
		}

		if (!SetHandleInformation(hReadReceivingPipes[i], HANDLE_FLAG_INHERIT, 0))
			printf("Error Stdout SetHandleInformation");
	}

	//initializing startup info and process info attributes for child processes
	for (i = 0; i < NO_OF_PROCESS; i++)
	{
		SecureZeroMemory(&si[i], sizeof(STARTUPINFO));
		SecureZeroMemory(&pi[i], sizeof(PROCESS_INFORMATION));

		si[i].cb = sizeof(STARTUPINFO);
		si[i].hStdInput = hReadSendingPipes[i];
		si[i].hStdOutput = hWriteReceivingPipes[i];
		si[i].hStdError = GetStdHandle(STD_ERROR_HANDLE);
		si[i].dwFlags = STARTF_USESTDHANDLES;

		//create child processes
		if (!CreateProcess(NULL, "child.exe", NULL, NULL, TRUE, 0, NULL, NULL, &si[i], &pi[i]))
		{
			printf("cannot create child process %d \n", i + 1);
			system("pause");
			exit(0);
		}
		processHandles[i] = pi[i].hProcess;
	}
	//send day information to each child process 
	//through anonymous pipes
	DWORD dwWritten;
	CHAR message[2];
	for (i = 0; i < NO_OF_PROCESS; i++)
	{
		sprintf_s(message, sizeof(message), "%d", (i + 1));
		WriteFile(hWriteSendingPipes[i], message, 2, &dwWritten, NULL);
	}

	//wait for all child processes to terminate
	WaitForMultipleObjects(NO_OF_PROCESS, processHandles, TRUE, INFINITE);

	//get day information to check
	DWORD dwRead;
	for (i = 0; i < NO_OF_PROCESS; i++)
	{
		if (!ReadFile(hReadReceivingPipes[i], message, 2, &dwRead, NULL))
			fprintf(stderr, "error reading the day info from processNo:%d \n", (i + 1));

		printf("Process id %d, day:%s\n", (i + 1), message);
	}

	//read the day_report from child processes
	printf("\nThe total number of each item sold in each day \n");
	for (i = 0; i < NO_OF_PROCESS; i++) {
		if (!ReadFile(hReadReceivingPipes[i], &weekReport[i], sizeof(DAY_REPORT), &dwRead, NULL))
			fprintf(stderr, "error reading the day report from processNo:%d \n", (i + 1));

		//Printing the number of each item sold in each day
		else
			printf("\tDay %d -> milk_count: %d, biscuit_count: %d, chips_count: %d, coke_count: %d\n",
			(i + 1), weekReport[i].milk_count, weekReport[i].biscuit_count, weekReport[i].chips_count, weekReport[i].coke_count);
	}

	
	/*  This part of the program works to print statistical data 
	*	to the standard output
	*	No more read and write operations performed
	*	but algorithms to find max min values inside data
	*/

	//printing the most sold items for each day
	printf("\nThe most sold items for each day\n");

	//convert week report to 2D int array
	int dataArray[NO_OF_PROCESS][NO_OF_THREADS];
	int totalNumberOfEachItem[NO_OF_THREADS];
	ZeroMemory(totalNumberOfEachItem, sizeof(totalNumberOfEachItem));
	int dailyMostSoldItemIndexes[NO_OF_PROCESS][NO_OF_THREADS];
	for (i = 0; i < NO_OF_PROCESS; i++)
	{
		DAY_REPORT *day = &weekReport[i];

		int milk_count = day->milk_count;
		int biscuit_count = day->biscuit_count;
		int chips_count = day->chips_count;
		int coke_count = day->coke_count;

		totalNumberOfEachItem[0] += milk_count;
		totalNumberOfEachItem[1] += biscuit_count;
		totalNumberOfEachItem[2] += chips_count;
		totalNumberOfEachItem[3] += coke_count;

		dataArray[i][0] = milk_count;
		dataArray[i][1] = biscuit_count;
		dataArray[i][2] = chips_count;
		dataArray[i][3] = coke_count;

		int j = 0;
		int dailyMaxSellCount = 0;
		dailyMaxSellCount = -1;
		for (j = 0; j < NO_OF_THREADS; j++) {
			if (dataArray[i][j] > dailyMaxSellCount) {
				dailyMaxSellCount = dataArray[i][j];
				dailyMostSoldItemIndexes[i][j] = 1;
				int c = j;
				while (c > 0)
					dailyMostSoldItemIndexes[i][--c] = 0;
			}
			else if (dataArray[i][j] == dailyMaxSellCount)
				dailyMostSoldItemIndexes[i][j] = 1;
		}

		printf("\tDay %d -> ", i + 1);
		int k = 0;
		for (k = 0; k < NO_OF_THREADS; k++) {
			if (dailyMostSoldItemIndexes[i][k] == 1) {
				if (k == 0)
					printf("MILK:%d ", milk_count);
				if (k == 1)
					printf("BISCUIT:%d ", biscuit_count);
				if (k == 2)
					printf("CHIPS:%d ", chips_count);
				if (k == 3)
					printf("COKE:%d ", coke_count);

			}
		}
		printf("\n");
	}

	//Print the total number of each item sold over all the days
	printf("\nThe total number of sales over %d days:", NO_OF_PROCESS);
	printf("\n\tMILK: %d ", totalNumberOfEachItem[0]);
	printf("\n\tBISCUIT: %d ", totalNumberOfEachItem[1]);
	printf("\n\tCHIPS: %d ", totalNumberOfEachItem[2]);
	printf("\n\tCOKE: %d\n", totalNumberOfEachItem[3]);

	//Print the most sold item over the all days
	printf("\nThe most sold items in %d days:\n", NO_OF_PROCESS);
	int maxIndexes[NO_OF_THREADS];
	ZeroMemory(maxIndexes, sizeof(maxIndexes));
	int max = 0;
	for (i = 0; i < NO_OF_THREADS; i++) {
		if (max < totalNumberOfEachItem[i]) {
			max = totalNumberOfEachItem[i];
			maxIndexes[i] = 1;
			int c = i;
			while (c > 0)
				maxIndexes[--c] = 0;
		}
		else if (max == totalNumberOfEachItem[i])
			maxIndexes[i] = 1;
	}
	for (i = 0; i < NO_OF_THREADS; i++) {
		if (maxIndexes[i] == 1) {
			if (i == 0)
				printf("\tMILK: %d\n", totalNumberOfEachItem[i]);
			if (i == 1)
				printf("\tBISCUIT: %d\n", totalNumberOfEachItem[i]);
			if (i == 2)
				printf("\tCHIPS: %d\n", totalNumberOfEachItem[i]);
			if (i == 3)
				printf("\tCOKE: %d\n", totalNumberOfEachItem[i]);
		}
	}

	printf("\n");

	//Close handles just before terminating
	for (i = 0; i < NO_OF_PROCESS; i++) {
		CloseHandle(pi[i].hProcess);
		CloseHandle(pi[i].hThread);
		CloseHandle(hWriteSendingPipes[i]);
		CloseHandle(hReadSendingPipes[i]);
		CloseHandle(hReadReceivingPipes[i]);
		CloseHandle(hWriteReceivingPipes[i]);
	}

	printf("\n");
	system("pause");
	return 0;
}