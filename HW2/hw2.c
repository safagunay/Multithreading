#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <pthread.h>
#include <stdbool.h>

#define RSRC_NO_TYPEA 2
#define RSRC_NO_TYPEB 1

#define THREAD_NO_TYPEA 4
#define THREAD_NO_TYPEB 1
#define THREAD_NO_TYPEC 1

pthread_mutex_t *typeAresources;
pthread_mutex_t *typeBresources;

void* thread_function_typeA(void* arg);
void* thread_function_typeB(void* arg);
void* thread_function_typeC(void* arg);
bool cutPasteSingleLine(char* fpReadFile, char* fpWriteFile, char* fpReplaceFile);
void removeNegativeNums(char* readFile);
void removePrimeNums(char* readFile);
void allZero(int n,int* arr);
bool isPrime(int num);

int main()
{
    int i = 0;
    typeAresources = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t)*RSRC_NO_TYPEA);
    typeBresources = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t)*RSRC_NO_TYPEB);

    for(i = 0; i < RSRC_NO_TYPEA; i++)
        pthread_mutex_init (&typeAresources[i], NULL);

    char buffer[50];
    FILE *pFile;
    for(i = 0; i < RSRC_NO_TYPEB; i++) {
        pthread_mutex_init (&typeBresources[i], NULL);
        sprintf(buffer,"empty%d.txt",i+1);
        pFile = fopen(buffer,"w");
        if(pFile!=NULL)
            fclose(pFile);
        else
            printf("file error!!\n");
    }

    /*create threads */
    int total_no_threads = THREAD_NO_TYPEA + THREAD_NO_TYPEB + THREAD_NO_TYPEC;
    pthread_t threadIDs[total_no_threads];

    for(i = 0; i < RSRC_NO_TYPEA; i++)
        pthread_create (&threadIDs[i], NULL, &thread_function_typeA, NULL);

    for(i = RSRC_NO_TYPEA; i < RSRC_NO_TYPEB + RSRC_NO_TYPEA; i++)
        pthread_create (&threadIDs[i], NULL, &thread_function_typeB, NULL);

    for(i = RSRC_NO_TYPEA + RSRC_NO_TYPEB; i < total_no_threads; i++)
        pthread_create(&threadIDs[i], NULL, &thread_function_typeC, NULL);

    for(i = 0; i < total_no_threads; i++)
        pthread_join(threadIDs[i],NULL);

    printf("All threads returned\n");

    free(typeAresources);
    free(typeBresources);

    exit(0);
}

/*By enforcing the rule that before type B rsrc is captured, typeA resource must be captured
*we prevent deadlock
*
*Next resource to wait (rsrcAindex, rsrcBindex) is determined by the loop counter (i), 
*if trylock does not lock any resource in the resource list
* the resource with index 'rsrcAindex' is waited to be locked
*the same rule holds for 'rsrcBindex'
*/
void* thread_function_typeA(void* arg) {
    printf("typeA thread is in execution from: %lu\n", pthread_self () );
    bool bTypeACaptured = false;
    bool bTypeBCaptured = false;
    bool bAllTypeAConsumed = false;
    int iCapturedTypeA = 0; //index of captured typeA resource
    int iCapturedTypeB = 0; //index of captured typeB resource
    int nConsumedTypeA = 0; // number of consumed typeA resources
    int consumedTypeAIndexes[RSRC_NO_TYPEA]; // 1 -> consumed, 0-> not consumed
    allZero(RSRC_NO_TYPEA, consumedTypeAIndexes);
    int i = 0;
    while(!bAllTypeAConsumed) {
        int rsrcAIndex = (i - RSRC_NO_TYPEA) % RSRC_NO_TYPEA; //rsrcAIndex -> next resource to wait if can not lock any
        int rsrcBIndex = (i - RSRC_NO_TYPEB) % RSRC_NO_TYPEB;
        if(consumedTypeAIndexes[rsrcAIndex]==0) {
            int j = 0;
            for (j = 0; j < RSRC_NO_TYPEA; j++)
                if (consumedTypeAIndexes[j] == 0)
                    if (pthread_mutex_trylock(&typeAresources[j]) == 0) {
                        bTypeACaptured = true;
                        iCapturedTypeA = j;
                        break;
                    }

            if (!bTypeACaptured) {
                pthread_mutex_lock(&typeAresources[rsrcAIndex]);
                iCapturedTypeA = rsrcAIndex;
            }

            int k = 0;
            for (k = 0; k < RSRC_NO_TYPEB; k++)
                if (pthread_mutex_trylock(&typeBresources[k]) == 0) {
                    bTypeBCaptured = true;
                    iCapturedTypeB = k;
                    break;
                }
            if (!bTypeBCaptured) {
                pthread_mutex_lock(&typeBresources[rsrcBIndex]);
                iCapturedTypeB = rsrcBIndex;
            }

            /*Start file operations
             *update consumedTypeAIndexes
             */
            srand(time(NULL));
            int nlinesToCut = rand() % 10 + 1;
            char readFile[50];
            char writeFile[50];
            char replaceFile[50];
            sprintf(readFile, "numbers%d.txt", iCapturedTypeA + 1);
            sprintf(writeFile, "empty%d.txt", iCapturedTypeB + 1);
            sprintf(replaceFile, "%lu", pthread_self());
            printf("readFile: %s, writeFile: %s, replaceFile: %s \n", readFile, writeFile, replaceFile);
            int n = 0;
            for(n = 0; n < nlinesToCut; n++)
                if(!cutPasteSingleLine(readFile, writeFile, replaceFile)) {
                    consumedTypeAIndexes[iCapturedTypeA] = 1;
                    if(++nConsumedTypeA==RSRC_NO_TYPEA)
                        bAllTypeAConsumed = true;
                    break;
                }
            //file ops are finished
            pthread_mutex_unlock (&typeAresources[iCapturedTypeA]);
            pthread_mutex_unlock (&typeBresources[iCapturedTypeB]);
            bTypeACaptured = false;
            bTypeBCaptured = false;
        }
        i++;
    }
    return NULL;
}

void* thread_function_typeB(void* arg) {
    int i = 0;
    for(i=0;i<RSRC_NO_TYPEA;i++) {
        pthread_mutex_lock(&typeAresources[i]);
        char readFile[50];
        sprintf(readFile, "numbers%d.txt", i + 1);
        removeNegativeNums(readFile);
        pthread_mutex_unlock(&typeAresources[i]);
    }
    for(i=0;i<RSRC_NO_TYPEB;i++) {
        pthread_mutex_lock(&typeBresources[i]);
        char readFile[50];
        sprintf(readFile, "empty%d.txt", i + 1);
        removeNegativeNums(readFile);
        pthread_mutex_unlock(&typeBresources[i]);
    }
    return NULL;
}

void* thread_function_typeC(void* arg) {
    int i = 0;
    for(i=0;i<RSRC_NO_TYPEA;i++) {
        pthread_mutex_lock(&typeAresources[i]);
        char readFile[50];
        sprintf(readFile, "numbers%d.txt", i + 1);
        removePrimeNums(readFile);
        pthread_mutex_unlock(&typeAresources[i]);
    }
    for(i=0;i<RSRC_NO_TYPEB;i++) {
        pthread_mutex_lock(&typeBresources[i]);
        char readFile[50];
        sprintf(readFile, "empty%d.txt", i + 1);
        removePrimeNums(readFile);
        pthread_mutex_unlock(&typeBresources[i]);
    }
    return NULL;
}

bool cutPasteSingleLine(char* readFile, char* writeFile, char* replaceFile) {
    bool returnval = true;
    FILE *fpReadFile, *fpWriteFile, *fpReplaceFile;
    if( (fpReadFile = fopen(readFile, "r")) == NULL)
        fprintf(stderr,"error openning readFile\n");
    if( (fpWriteFile = fopen(writeFile, "a")) == NULL)
        fprintf(stderr,"error openning writeFile\n");
    if( (fpReplaceFile = fopen(replaceFile, "w")) == NULL)
        fprintf(stderr,"error openning replaceFile\n");
    char ch;
    ch = getc(fpReadFile);
    if(ch != EOF) {
        do {
            fputc(ch, fpWriteFile);
            ch = getc(fpReadFile);
        } while (ch != '\n');
        fputc(ch, fpWriteFile);
    }
    else
        returnval = false;

    ch = getc(fpReadFile);
    if(ch != EOF)
        do {
            fputc(ch, fpReplaceFile);
            ch = getc(fpReadFile);
        } while(ch != EOF);
    else
        returnval = false;

    fclose(fpReadFile);
    fclose(fpReplaceFile);
    fclose(fpWriteFile);
    remove(readFile);
    rename(replaceFile, readFile);
    return returnval;
}

void removeNegativeNums(char* readFile) {
    char replaceFile[50];
    sprintf(replaceFile, "%lu.c", pthread_self());
    FILE *fpRead, *fpReplace;
    fpRead = fopen(readFile, "r");
    fpReplace = fopen(replaceFile, "w");
    char ch = getc(fpRead);
    if(ch!=EOF)
        while (ch != EOF) {
            if (ch != '-') {
                do {
                    fputc(ch, fpReplace);
                    ch = getc(fpRead);
                } while (ch != '\n');
                fputc(ch, fpReplace);
                ch = getc(fpRead);
            } else {
                while (ch != '\n')
                    ch = getc(fpRead);
                ch = getc(fpRead);
            }
        }
    fclose(fpRead);
    fclose(fpReplace);
    remove(readFile);
    rename(replaceFile, readFile);
}

void removePrimeNums(char* readFile) {
    char replaceFile[50];
    char buff[10];
    int pos = 0;
    sprintf(replaceFile, "%lu.c", pthread_self());
    FILE *fpRead, *fpReplace;
    fpRead = fopen(readFile, "r");
    fpReplace = fopen(replaceFile, "w");
    char ch = getc(fpRead);
    if(ch!=EOF)
        while (ch != EOF) {
            if (ch != '-') {
                pos = 0;
                do {
                    buff[pos++] = ch;
                    ch = getc(fpRead);
                } while (ch != '\n');
                buff[pos] = '\0';
                if(!isPrime(atoi(buff))) {
                    int k = 0;
                    for(k = 0; k < pos; k++)
                        fputc(buff[k], fpReplace);
                    fputc('\n', fpReplace);
                }
                ch = getc(fpRead);
            } else {
                while (ch != '\n') {
                    fputc(ch, fpReplace);
                    ch = getc(fpRead);
                }
                fputc('\n', fpReplace);
                ch = getc(fpRead);
            }
        }
    fclose(fpRead);
    fclose(fpReplace);
    remove(readFile);
    rename(replaceFile, readFile);
}

void allZero(int n,int* arr) {
    int i = 0;
    for(i = 0; i < n; i++)
        arr[i]=0;
}

bool isPrime(int num) {
    bool prime = true;
    int i = 0;
    for(i=2;i < num; i++)
        if(num%i==0) {
            prime = false;
            break;
        }
    return prime;
}

