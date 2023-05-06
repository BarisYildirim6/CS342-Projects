#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <sys/time.h>

pthread_mutex_t * locks;


typedef struct pcb
{
    int pid;
    int burstLength;
    struct timeval arrivalTime;
    int remainingTime;
    struct timeval finishTime;
    int turnaroundTime;
    int procId;
    
} pcb;

typedef struct node
{
    struct node *next;
    pcb * data;
} node;

void initPCB(pcb *process, int pid, int burst, struct timeval arrivalTime){
    process->pid = pid;
    process->burstLength = burst;
    process->arrivalTime = arrivalTime;
    process->remainingTime = burst;
    process->finishTime.tv_sec = -1;
    process->finishTime.tv_usec = -1;
    process->turnaroundTime = -1;
    process->procId = -1;
}

void printPCB(int time, pcb *process){
    if(process == NULL) {
        printf("No process\n");

    }
    else if (process->pid == -1) {
        printf("End of queue\n");
    }
    else {
        printf("time= %d, cpu=%d, pid=%d, burstlen=%d, remainingtime=%d\n",time, process->procId, process->pid, process->burstLength, process->remainingTime);


    }
}

void printPCBDetail(pcb *process, struct timeval start){
    if(process == NULL) {
        printf("No process\n");

    }
    else if (process->pid == -1) {
        printf("End of queue\n");
    }
    else {
        int arrv = (process->arrivalTime.tv_sec - start.tv_sec) * 1000 + (process->arrivalTime.tv_usec - start.tv_usec)/1000;
        int fin = (process->finishTime.tv_sec - start.tv_sec) * 1000 + (process->finishTime.tv_usec - start.tv_usec)/1000;
        int waitTime = process->turnaroundTime - process->burstLength;
        printf("%5d %5d %10d %10d %15d %15d %15d\n",process->pid, process->procId, process->burstLength, arrv, fin, waitTime, process->turnaroundTime);


    }
}

void printQueue(node *head, struct timeval t) {
    if(head == NULL) {
        printf("No item in queue\n");
    }
    while(head != NULL){
        printPCBDetail(head->data, t);
        head = head->next;
    }
}

void addToQueue(node **head, pcb *addItem){
    node *newNode = (node *)malloc(sizeof(node));
    newNode->next = NULL;
    newNode->data = addItem;
    if (*head == NULL) {
        *head = newNode;
    }
    else {
        node *tmp = *head;
        node *prev = NULL;
        while(tmp->next !=  NULL){
            prev = tmp;
            tmp = tmp->next;
        }
        if (tmp->data->pid == -1)
        {
            if((*head)->next == NULL) {
                *head = newNode;
                (*head)->next = tmp;
                tmp->next = NULL;
            }
            else {
                prev->next = newNode;
                newNode->next = tmp;
                tmp->next = NULL;
            }
            
        }
        else {
            tmp->next = newNode;
        }
        
    }
    
}

int isEmpty(node *head) {
    if(head == NULL) {
        return 1;
    }
    else {
        return 0;
    }
}
//get the pcb of the thread with given id
void getShortest(node **head, pcb **item) {
    if((*head)->data->pid == -1) { //if there is only a signle element in the queue check if it is the dummy node (if not error)
        *item = NULL;  
    }
    else {
        float shortest = INT_MAX;
        node *current = *head;
        node *prev = NULL; // to traverse all the list
        node *shortestPrev = NULL;
        node * shortestNode = *head; // to keeptrack of the minimum
        while(current != NULL){
            if(current->data->burstLength < shortest && current->data->pid != -1) {
                shortest = current->data->burstLength;
                shortestNode = current;
                shortestPrev = prev;
            }
            prev = current;
            current = current->next;
        }
        *item = shortestNode->data;
        if(shortestPrev == NULL) {
            *head = (*head)->next;
        }
        else {
            shortestPrev->next = shortestNode->next;
        }

        shortestNode->next = NULL;
        free(shortestNode);
    }

    }


//get the first pcb from the queue, use for FCFS and RR
void getFirst(node **head, pcb **item, int id) {
     node *temp = *head;
     if(temp == NULL) {
        *item = NULL;

     }
    //check if the first node is a dummy node, if it is dont remove and set pcb to null, check in main (set pid -1 for dummy nodes)
    if(temp->data->pid == -1){
     *item = NULL;
    }
    else {
        *head = (*head)->next;
        temp->next = NULL;
        *item = temp->data;
        free(temp);
    }
    
    

}

void freeQueue(node *head) {
    node *tmp;
    while (head != NULL)
    {
        tmp = head;
        head = head->next;
        free(tmp->data);
        free(tmp);
    }
}

void addAscending(node **head, pcb *addItem) {
    node *newNode = (node *)malloc(sizeof(node));
    newNode->next = NULL;
    newNode->data = addItem;
    if (*head == NULL || (*head)->data->pid >= addItem->pid) {
        newNode->next = *head;
        *head = newNode;
    }
    else {
        node *tmp = *head;
        while(tmp->next !=  NULL && tmp->next->data->pid <= addItem->pid){
            tmp = tmp->next;
        }
        
        newNode->next = tmp->next;
        tmp->next = newNode;

    }
}

struct args {
    node **waitQueue;
    pthread_mutex_t lock;
    int schedType;
    int timeQ;
    int *bool;
    int procId;
    int **queueLengths;
    struct timeval *t;
    node **finishedQueue;
    int outmode;
};

struct args* initArgs(node **waitQueue, pthread_mutex_t lock, int schedT, int timeQ, int *bool, int procId, int **queueLengths, struct timeval *start, node **finished, int outmode) {
    struct args *new = (struct args*)malloc(sizeof(struct args));
    new->bool = bool;
    new->lock = lock;
    new->procId =procId;
    new->schedType = schedT;
    new->timeQ = timeQ;
    new->waitQueue = waitQueue;
    new->finishedQueue = finished;
    new->queueLengths = queueLengths;
    new->t = start;
    new->outmode = outmode;
}

//Single wait queue implementation, also can be used for every queue in multiqueue
void *sched(void *args) { //schedType => 0 = FCFS, 1 = SJF, 2 = RR, timeQ used for RR only:set 0 for others
    struct args *funcArgs = (struct args *)args;
    pthread_mutex_t singleLock = funcArgs->lock;
    node **waitQueue = funcArgs->waitQueue;
    int schedType = funcArgs->schedType;
    int timeQ = funcArgs->timeQ;
    int procId = funcArgs->procId;
    node **finished = funcArgs->finishedQueue;
    int *queueLengths = *funcArgs->queueLengths;
    struct timeval fin;
    struct timeval start = *funcArgs->t;
    int outmode = funcArgs->outmode;

    while(1) {
        pthread_mutex_lock(&singleLock); //lock ı böyle kullanmak doğru mu?
        while(isEmpty(*waitQueue) == 1) {
            usleep(1000);
            
        }
        
        pcb *process;
        if(schedType == 0 || schedType == 2){ //If scheduling algo is FCFS or RR, the first element will be picked from the queue
            getFirst(waitQueue, &process, procId);

        
        }
       
        else if(schedType == 1) { //For SJF shortest will be picked
            getShortest(waitQueue, &process);
            
        }

        else {
            pthread_exit(0); //no such scheduling algorithm specified
        } 
        pthread_mutex_unlock(&singleLock);
       
        if (process == NULL && *(funcArgs->bool) == 1) { // this means there are no other processes to execute, exit the thread and end the simulation
            pthread_exit(0);

        }
        
        else {
            process->procId = procId;
            gettimeofday(&fin, NULL);
            int time = (fin.tv_sec - start.tv_sec)* 1000 + (fin.tv_usec - start.tv_usec)/1000;
            if(outmode == 2) {
                printPCB(time, process);
            }

            if(schedType == 0 || schedType == 1){ //If scheduling algo is FCFS or SJF, sleep as long as burst time
                usleep(process->burstLength * 1000);
                gettimeofday(&fin, NULL);
                process->finishTime = fin;
                //printf("\nprocess id: %d turnaround:%ld\n", process->pid, (fin.tv_sec - process->arrivalTime.tv_sec) * 1000 + (fin.tv_usec - process->arrivalTime.tv_usec)/1000);
                queueLengths[procId - 1] = queueLengths[procId - 1] - process->burstLength;
                process->turnaroundTime = (fin.tv_sec - process->arrivalTime.tv_sec) * 1000 + (fin.tv_usec - process->arrivalTime.tv_usec)/1000;
                addAscending(finished, process);
            }
            

            else if(schedType == 2) { //For RR sleep as long as time quantum
                if(process->remainingTime > timeQ ) {
                    usleep(timeQ* 1000);
                    process->remainingTime -= timeQ;
                    pthread_mutex_lock(&singleLock);
                    addToQueue(waitQueue, process); //if the process is not finished, add it back to the queue
                    pthread_mutex_unlock(&singleLock);
                    queueLengths[procId - 1] = queueLengths[procId - 1] + process->remainingTime;

                }
                else {
                    usleep(process->remainingTime * 1000);
                    process->remainingTime -= process->remainingTime;
                    gettimeofday(&fin, NULL);
                    process->finishTime = fin;
                    queueLengths[procId - 1] = queueLengths[procId - 1] - process->burstLength;
                    //printf("\nprocess id: %d turnaround:%ld\n", process->pid, (fin.tv_sec - process->arrivalTime.tv_sec) * 1000 + (fin.tv_usec - process->arrivalTime.tv_usec)/1000);
                    process->turnaroundTime = (fin.tv_sec - process->arrivalTime.tv_sec) * 1000 + (fin.tv_usec - process->arrivalTime.tv_usec)/1000;
                    addAscending(finished, process);
                }
                
            }
    
        } 
        
 
    }
    
    pthread_exit(0);
}

void fileOutput () {
    
}

int findShortest(int *queueLengths, int N) {
    int shortest = INT_MAX;
    int index = -1;
    for(int i = 0; i < N; i++) {
        if (queueLengths[i] < shortest) {
            shortest = queueLengths[i];
            index = i;
        }
    }
    return index;
}

//Main

int main(int argc, char *argv[]) {
    struct timeval start;
    gettimeofday(&start, NULL);


    //---------------COMMAND LINE ARGUMENTS----------------------
    int readFile = 0; //check if the whole file is read
    int N = 2;
    int SAP = 1; // 0 -> S, 1 -> M
    int QS = 0; // -1 -> NA, 0 -> RM, 1 -> LM
    int ALG = 2; // 0 -> FCFS, 1 -> SJF, 2 -> RR
    int Q = 20;
    char* INFILE = "in.txt";
    int OUTMODE = 1; // 1 -> nothing, 
    char* OUTFILE = "out.txt";
    int inputSpecified = 0;
    int randomSpecified = 0;

    for (int i = 1; i < argc; i++)
    {

        if(strcmp(argv[i], "-n") == 0){
            N = atoi(argv[i+1]);
        }
        if(strcmp(argv[i], "-a") == 0){
            if(strcmp(argv[i+1], "S") == 0){
                SAP = 0;
                QS = -1;

            }
            else if (strcmp(argv[i+1], "M") == 0){
                SAP = 1;
                if((strcmp(argv[i+2], "RM") == 0)) {
                    QS = 0;
                }
                else if((strcmp(argv[i+2], "LM") == 0)) {
                    QS = 1;
                }
                else {
                    printf("Wrong argument format!");
                    exit(0);
                }
            }
            else {
                printf("Wrong argument format!");
                exit(0);
            }
        }
        if(strcmp(argv[i], "-s") == 0){
            if(strcmp(argv[i+1], "FCFS") == 0){
                ALG = 0;
                Q = 0;
            }
            else if (strcmp(argv[i+1], "SJF") == 0){
                ALG = 1;  
                Q = 0;
              
            }
            else if (strcmp(argv[i+1], "RR") == 0) {
                ALG = 2;
                Q = atoi(argv[i+2]);
            }
            else {
                printf("Wrong argument format!");
                exit(0);
            }
        }
        if(strcmp(argv[i], "-i") == 0){
            INFILE = argv[i+1];
            inputSpecified = 1;
        }
        if(strcmp(argv[i], "-m") == 0){
            if(strcmp(argv[i+1], "1") == 0){
                OUTMODE = 1;
            }
            else if (strcmp(argv[i+1], "2") == 0){
                OUTMODE = 2;
              
            }
            else if (strcmp(argv[i+1], "3") == 0) {
                OUTMODE = 3;
            }
            else {
                printf("Wrong argument format!");
                exit(0);
            }
            
        }
        if(strcmp(argv[i], "-o") == 0){
            OUTFILE = argv[i+1];
            OUTMODE = 0;
            
        }
        if(strcmp(argv[i], "-r") == 0){
            randomSpecified = 1;
        }
    }

    //-------------------END OF COMMAND LINE ARGUMENTS----------------------

    int *queueLengths = calloc(N, sizeof(int)); // used for multi queue load balancing
    locks = calloc(N, sizeof(pthread_mutex_t));
    pthread_t threads[N];
    struct args *argsList[N];
    node *queueList[N];
    node *finishedProcesses = NULL;
    pthread_mutex_t finishedLock;
    pthread_mutex_init(&finishedLock, NULL);

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    

    //common queue
    if(SAP == 0) {
        pthread_mutex_init(&locks[0], NULL);
        queueList[0] = NULL;
        for(int i = 0; i < N; i++) {
            argsList[i] = initArgs(&queueList[0],locks[0], ALG, Q, &readFile, i+1, &queueLengths, &start, &finishedProcesses, OUTMODE) ;
            pthread_create(&threads[i], &attr, sched, argsList[i]);   
        } 
    }

    
    //Multi queue
    else {
        for (int i = 0; i < N; i++) {
            pthread_mutex_init(&locks[i], NULL);
            queueList[i] = NULL;
            argsList[i] = initArgs(&queueList[i],locks[i], ALG, Q, &readFile, i+1, &queueLengths, &start, &finishedProcesses, OUTMODE);
            pthread_create(&threads[i], &attr, sched, argsList[i]);   
            queueLengths[i] = 0;

        }
    }

    //------------------BURST INFO READ FROM FILE--------------------------------
    if(inputSpecified == 1 && randomSpecified == 0) {

        FILE *fp = fopen(INFILE, "r");
        if (fp == NULL) {
            exit(1);
        }

        char line[100];
        int pid =1;
        struct timeval tv;
        int index = 0;
        while (fgets(line, 100, fp) != NULL) {
            char word1[5], word2[70];
            sscanf(line, "%s %s", word1, word2);
            if(strcmp(word1, "PL") == 0) {

                int burstTime = atoi(word2);
                
                pcb *newPcb = malloc(sizeof(pcb));
                gettimeofday(&tv, NULL);
                initPCB(newPcb, pid, burstTime,tv);
                
                pid ++;
                if(SAP == 0) {
                    pthread_mutex_lock(&locks[0]);
                    addToQueue(&queueList[0], newPcb);
                    pthread_mutex_unlock(&locks[0]);
                }
                else {
                    if(QS == 0) {
                        pthread_mutex_lock(&locks[index]);
                        addToQueue(&queueList[index], newPcb);
                        pthread_mutex_unlock(&locks[index]);
                        index = (index + 1) % N;
                    }
                    else if(QS == 1) {
                        int shortestIndex = findShortest(queueLengths, N);
                        printf("%d", shortestIndex);
                        pthread_mutex_lock(&locks[shortestIndex]);
                        addToQueue(&queueList[shortestIndex], newPcb);
                        queueLengths[shortestIndex] += newPcb->burstLength;
                        pthread_mutex_unlock(&locks[shortestIndex]);
                    }
                }    
            }
            else if (strcmp(word1, "IAT") == 0) {
                    int sleepTime = atoi(word2) * 1000;
                    usleep(sleepTime);
            }
            else {
                exit(1);
            }
        }
        fclose(fp);
        readFile = 1;

        for(int i = 0; i < N; i++) {

            pcb *dummy = malloc(sizeof(pcb));
            gettimeofday(&tv, NULL);
            initPCB(dummy,-1,0, tv);
            pthread_mutex_lock(&locks[i]);

            addToQueue(&queueList[i], dummy);
            pthread_mutex_unlock(&locks[i]);

            if(SAP == 0) {
                break;
            }

        }
    }
    //----------------------------BURST INFO GENERATED RANDOMLY--------------------------------
    else {

    }

    //---------------------------FINISHED PROCESSES PRINTED TO SCREEN--------------------------

    //-----------------PROGRAM END-------------- FREE ALLOCATED MEMORY-------------------------
    for(int i = 0; i < N; i++) {
        pthread_join(threads[i], NULL);
        free(argsList[i]);
    }

    
    for(int i = 0; i < N; i++) {
        freeQueue(queueList[i]);
        pthread_mutex_destroy(&locks[i]);
        if(SAP == 0) {
            break;
        }
    }
    pthread_mutex_destroy(&finishedLock);
    printf("%5s %5s %10s %10s %15s %15s %15s\n","pid", "cpu", "burstlen", "arv", "finish", "waitingtime", "turnaround");
    printQueue(finishedProcesses, start);
    freeQueue(finishedProcesses);
    free(locks);
    free(queueLengths);
    
    return 0;

    
}