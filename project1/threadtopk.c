#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <pthread.h>
#include <time.h>     
#include <sys/time.h>
// implementation for  array
struct Array
{
    char **str_array;
    int *array;
    int size;
    int used;
};

void initArr(struct Array *arr)
{
    // initialize the array with one entry
    arr->array = (int *)malloc(sizeof(int));
    arr->str_array = (char **)malloc(sizeof(char *));
    arr->str_array[0] = malloc(64);
    arr->size = 1;
    arr->used = 0;
}

void insertArr(struct Array *arr, int item, char *string)
{
    if (arr->size == arr->used)
    {
        arr->size = arr->size * 2;
        arr->array = (int *)realloc(arr->array, arr->size * sizeof(int));
        arr->str_array = (char **)realloc(arr->str_array, arr->size * sizeof(char *));
        for (int i = arr->used; i < arr->size; i++)
        {
            arr->str_array[i] = malloc(64);
        }
    }
    arr->array[arr->used] = item;
    strcpy(arr->str_array[arr->used], string);
    arr->used += 1;
}

void freeArr(struct Array *arr)
{

    free(arr->array);
    for (int i = 0; i < arr->size; i++)
    {
        free(arr->str_array[i]);
    }
    free(arr->str_array);
}

void printArr(struct Array *arr)
{
    for (int i = 0; i < arr->used; i++)
    {
        printf("key: %s value: %d \n", arr->str_array[i], arr->array[i]);
    }
}

int findKey(struct Array *arr, char *key)
{
    for (int i = 0; i < arr->used; i++)
    {
        if (strcmp(key, arr->str_array[i]) == 0)
        {
            return i;
        }
    }
    return -1;
}

void insertionSort(struct Array *arr)
{
    int i, key, j;
    for (i = 1; i < arr->used; i++)
    {
        key = arr->array[i];
        char *str_kry = malloc(64);
        strcpy(str_kry, arr->str_array[i]);

        j = i - 1;
        while ((j >= 0 && arr->array[j] < key) | (j >= 0 && strcmp(arr->str_array[j], str_kry) > 0 && arr->array[j] == key))
        {
            arr->array[j + 1] = arr->array[j];
            strcpy(arr->str_array[j + 1], arr->str_array[j]);
            j = j - 1;
        }
        arr->array[j + 1] = key;
        strcpy(arr->str_array[j + 1], str_kry);
        free(str_kry);
    }
}


typedef struct WordCount
{
    char word[64];
    int count;
} WordCount;

typedef struct 
{
    int i;
    int k;
}threadArg;

char **names;
WordCount **results;
void *processFile(void *args){
    threadArg* threadArgs = (threadArg *)args;
    int threadNumberInt = threadArgs->i;
    int k = threadArgs->k;
    FILE *inputFile;
    inputFile = fopen(names[threadNumberInt], "r");

    if (inputFile == NULL)
    {
        printf("%s named file can't be opened \n", names[threadNumberInt]);
        exit(-1);
    }

    struct Array arr;
    initArr(&arr);
    char *buff = malloc(64);
    while (fscanf(inputFile, "%s", buff) != EOF)
    {
        for (int len = 0; len < strlen(buff); len++)
        {
            buff[len] = toupper(buff[len]);
        }
        if (arr.used == 0)
        {
            insertArr(&arr, 1, buff);
        }
        else
        {
            if (findKey(&arr, buff) > -1)
            {
                int index = findKey(&arr, buff);
                arr.array[index] += 1;
            }
            else
            {
                insertArr(&arr, 1, buff);
            }
        }
    }
    insertionSort(&arr);
    int end;
    if (k <= arr.used){
        end = k;
    }
    else{
        end = arr.used;
    } 
    for (int i = 0; i < end; i++){
        results[threadNumberInt][i].count = arr.array[i];
        strcpy(results[threadNumberInt][i].word, arr.str_array[i]);
    }

    freeArr(&arr);
    free(buff);
    fclose(inputFile); 

    pthread_exit(0);
    
}

int main(int argc, char *argv[])
{

    /* struct timeval start, end;
 
    gettimeofday(&start, NULL); */

    int k = atoi(argv[1]);
    int n = atoi(argv[3]);
    char * outputFile = argv[2];
    names = (char **)malloc(n * sizeof(char*));
    for (int i = 0; i < n; i++) {
        names[i] = malloc(sizeof(char) * 64);
    } 


    for(int arg_count = 4; arg_count<argc; arg_count++) {
        strcpy(names[arg_count - 4], argv[arg_count]);
    }

   
    results = (WordCount **)malloc(n * sizeof(WordCount *));
    for (int i = 0; i < n; i++)
    {
        results[i] = (WordCount *)calloc(k , sizeof(WordCount));
    }
    pthread_t threadIds[n];
    

    for (int i = 0; i < n; i++)
    {
        threadArg args;
        args.i = i;
        args.k = k;
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_create(&threadIds[i], &attr, processFile, &args);
        pthread_join(threadIds[i], NULL);
    }
    
    struct Array finalArray;
    initArr(&finalArray);
    int arrSize = 0;
    for(int i = 0; i < n; i++) {
        WordCount *data = results[i];
        for(int j = 0; j < k; j++) {
            if(data[j].count != 0) {

                if(findKey(&finalArray, data[j].word) > -1) {
                int index = findKey(&finalArray,data[j].word);
                finalArray.array[index] += data[j].count;
                }

                else {
                    insertArr(&finalArray, data[j].count, data[j].word);
                    arrSize ++;
                }
            }
            else {
                break;
            }

        }
    }
    insertionSort(&finalArray);

    if(arrSize > k) {
            arrSize = k;
        }

    FILE  *outfile;
    outfile = fopen(outputFile, "w");
    for(int i = 0; i < arrSize; i++) {
        fprintf(outfile, "%s %d", finalArray.str_array[i], finalArray.array[i]);
        if (i != arrSize - 1) {
            fprintf(outfile, "\n");
        }
    }


    for(int i = 0; i < n; i++) {
        free(names[i]);
        names[i] = NULL;
    }
    free(names);

    for (int i = 0; i < n; i++)
    {
        free(results[i]);
    }
    free(results);
    fclose(outfile);
    freeArr(&finalArray);
    /* gettimeofday(&end, NULL);
 
    long seconds = (end.tv_sec - start.tv_sec);
    long micros = ((seconds * 1000000) + end.tv_usec) - (start.tv_usec);
 
    printf("The elapsed time is %ld micros\n",  micros); */

    return 0;
}
