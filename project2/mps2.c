#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <sys/time.h>
#include <math.h>


typedef struct pcb
{
    int pid;
    int burstLength;
    int arrivalTime;
    int remainingTime;
    int finishTime;
    int turnaroundTime;
    int procId;
    
} pcb;

typedef struct node
{
    struct node *next;
    pcb * data;
} node;

void initPCB(pcb *process, int pid, int burst, int arrivalTime){
    process->pid = pid;
    process->burstLength = burst;
    process->arrivalTime = arrivalTime;
    process->remainingTime = burst;
    process->finishTime = -1;
    process->turnaroundTime = -1;
    process->procId = -1;
}

void printPCB(int time, pcb *process){
    if(process == NULL) {
        printf("No process\n");

    }
    else {
        printf("time= %d, cpu=%d, pid=%d, burstlen=%d, remainingtime=%d\n",time, process->procId + 1, process->pid, process->burstLength, process->remainingTime);


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

int isEmpty(node *head) {
    if(head == NULL) {
        return 1;
    }
    else {
        return 0;
    }
}

void * getFirst(node **head, pcb **item) { 
    if(*head == NULL) {
        *item = NULL;

    }
    node *temp = *head;
    if(temp != NULL) {
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
    else{
        return NULL;
    }
}

void getShortest(node **head, pcb **item) {
    if(*head == NULL) {
        *item = NULL;

    }

    if((*head) != NULL && (*head)->data->pid == -1) { //if there is only a signle element in the queue check if it is the dummy node (if not error)
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
        if(shortestNode != NULL) {
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
        else {
            *item = NULL;

        }
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
        if(tmp != NULL) {
            newNode->next = tmp->next;
            tmp->next = newNode;
        }
        

    }
}

struct args {
    int schedType;
    int timeQ;
    int procId;
    int outmode;
    int sap;
};

struct args* initArgs(int schedT, int timeQ,int procId, int outmode, int sap) {
    struct args new = (struct args)malloc(sizeof(struct args));
    new->procId =procId;
    new->schedType = schedT;
    new->timeQ = timeQ;
    new->outmode = outmode;
    new->sap = sap;
    return new;
}

node **queues;
node *finished;
pthread_mutex_t *finishedProcessLock;
pthread_mutex_t **locks;
int inputsFinished;
pthread_mutex_t *inputsFinishedLock;
struct timeval start;
FILE *outfile;


int findShortest(node **queues, int N) {
    int shortest = INT_MAX;
    int index = 0;
    for(int i = 0; i < N; i++) {
        int totalTime = 0;
        pthread_mutex_lock(locks[i]);
        node* temp = queues[i];
        while(temp != NULL){
            totalTime += temp->data->remainingTime;
            temp = temp->next;
        }
        if(totalTime < shortest) {
            shortest = totalTime;
            index = i;
        }
        pthread_mutex_unlock(locks[i]);

    }
    return index;
}



void *sched(void *args){
    struct args *funcArgs = (struct args *) args;
    int procId = funcArgs->procId;
    int outmode = funcArgs->outmode;
    int schedType = funcArgs->schedType;
    int timeQ = funcArgs->timeQ;
    int sap = funcArgs->sap;
    int index;
    if(sap == 0) {
        index = 0;
    }
    else{
        index = procId;
    }
    while(1) {
        struct timeval current;
        struct timeval finish;
        int unlocked = 0;
        pthread_mutex_lock(locks[index]);
        
        while(isEmpty(queues[index]) == 1) {
            if(unlocked != 1) {
                pthread_mutex_unlock(locks[index]);
                unlocked = 1;
            }
            usleep(1000);   
        }
        if(unlocked != 1) {
            pthread_mutex_unlock(locks[index]);
        }

        pcb *pcbPtr = NULL;

        if(schedType == 0 || schedType == 2){ //If scheduling algo is FCFS or RR, the first element will be picked from the queue
            pthread_mutex_lock(locks[index]);
            getFirst(&queues[index], &pcbPtr); 
            pthread_mutex_unlock(locks[index]);
        
        
        }
        else if(schedType == 1) { //For SJF shortest will be picked
            pthread_mutex_lock(locks[index]);
            getShortest(&queues[index], &pcbPtr); 
            pthread_mutex_unlock(locks[index]);
          

        }

        else {
            pthread_exit(0); //no such scheduling algorithm specified
        } 
        

        pthread_mutex_lock(inputsFinishedLock);
        if(pcbPtr == NULL && inputsFinished == 1) {
            pthread_mutex_unlock(inputsFinishedLock);
            pthread_exit(0);
        }
        else if(pcbPtr == NULL && inputsFinished != 1){
            pthread_mutex_unlock(inputsFinishedLock);
            continue;
        }
        pthread_mutex_unlock(inputsFinishedLock);
        
        
        if(pcbPtr) {
            pcbPtr->procId = procId;
        }
        gettimeofday(&current, NULL);
        int currentT = (current.tv_sec - start.tv_sec )*1000 + (current.tv_usec - start.tv_usec)/1000;
        if(outmode == 2) {
            printPCB(currentT, pcbPtr);
        }
        if(schedType == 0 || schedType == 1){ //If scheduling algo is FCFS or SJF, sleep as long as burst time
            int sleep = pcbPtr->burstLength;
            usleep(sleep * 1000);
            gettimeofday(&finish, NULL);
            int finishT = (finish.tv_sec - start.tv_sec )*1000 + (finish.tv_usec - start.tv_usec)/1000;
            pcbPtr->finishTime = finishT;
            pcbPtr->turnaroundTime = pcbPtr->finishTime - pcbPtr->arrivalTime;
            pthread_mutex_lock(finishedProcessLock);
            addAscending(&finished, pcbPtr);
            pthread_mutex_unlock(finishedProcessLock);

        }
        else if(schedType == 2) { //For RR sleep as long as time quantum
            if(pcbPtr->remainingTime > timeQ ) {
                usleep(timeQ* 1000);
                pcbPtr->remainingTime -= timeQ;
                pthread_mutex_lock(locks[index]);
                addToQueue(&queues[index], pcbPtr); //if the process is not finished, add it back to the queue
                pthread_mutex_unlock(locks[index]);

            }
            else {
                usleep(pcbPtr->remainingTime * 1000);
                pcbPtr->remainingTime -= pcbPtr->remainingTime;
                gettimeofday(&finish, NULL);
                int finishT = (finish.tv_sec - start.tv_sec )*1000 + (finish.tv_usec - start.tv_usec)/1000;
                pcbPtr->finishTime = finishT;
                pcbPtr->turnaroundTime = pcbPtr->finishTime - pcbPtr->arrivalTime;
                pthread_mutex_lock(finishedProcessLock);
                addAscending(&finished, pcbPtr);
                pthread_mutex_unlock(finishedProcessLock);
            }
        }      
    }
    pthread_exit(0);
    

}
void printQueue(node *head) {
    if(head == NULL) {
        printf("No item in queue\n");
    }
    while(head != NULL){
        printPCB(10, head->data);
        head = head->next;
    }
}

void outputFile (node *head) {
    if (head == NULL) {
        printf("No item in queue\n")
    }
    else {
        outfile = fopen("out.txt", "w");

        while (head != NULL) {
            pcb *process = head->data;
            if (process->pid == 1) {
                fprintf(outfile,"%5s %5s %10s %10s %15s %15s %15s\n","pid", "cpu", "burstlen", "arv", "finish", "waitingtime", "turnaround");
            }
            fprintF(outfile, "%5d %5d %10d %10d %15d %15d %15d\n",process->pid, process->procId, process->burstLength, process->arrivalTime, process->finishTime, 100, process->turnaroundTime);
            head = head->next;
        }
        fclose(outfile);
    }
}

int main(int argc, char *argv[]) {
    gettimeofday(&start, NULL);
    int N = 3;
    int SAP = 0;//1; // 0 -> S, 1 -> M
    int QS = 0; // -1 -> NA, 0 -> RM, 1 -> LM
    int ALG = 2;//2; // 0 -> FCFS, 1 -> SJF, 2 -> RR
    int Q = 20;
    char* INFILE = "in.txt";
    int OUTMODE = 1; // 1 -> nothing, 
    char* OUTFILE = "out.txt";
    int inputSpecified = 0;
    int randomSpecified = 0;

    double T = 200;
    double T1 = 10;
    double T2 = 1000;

    double L = 100;
    double L1 = 10;
    double L2 = 500;
    int PC = 5;
    pthread_t threads[N];
    struct args *argsList[N];
    finished = NULL;
    inputsFinishedLock = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(inputsFinishedLock, NULL);

    finishedProcessLock = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(finishedProcessLock, NULL);

    pthread_mutex_lock(inputsFinishedLock);
    inputsFinished = 0;
    pthread_mutex_unlock(inputsFinishedLock);

    for (int i = 1; i < argc; i++){

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


    if(SAP == 0) {
        queues = (node **)calloc(1,sizeof(node *));
        locks = (pthread_mutex_t **)calloc(1, sizeof(pthread_mutex_t *));
        locks[0] = malloc(sizeof(pthread_mutex_t));
        pthread_mutex_init(locks[0], NULL);
        queues[0] = NULL;
        for(int i = 0; i < N; i++) {
            argsList[i]  = initArgs(ALG, Q, i, OUTMODE, SAP);
            pthread_create(&threads[i], NULL, sched, argsList[i]);   
        } 
    }

    
    //Multi queue
    else {
        queues = (node **)calloc(N,sizeof(node *));
        locks = (pthread_mutex_t **)calloc(N, sizeof(pthread_mutex_t *));
        for (int i = 0; i < N; i++) {
            argsList[i]  = initArgs(ALG, Q, i, OUTMODE, SAP);
            queues[i] = NULL;
            locks[i] = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
            pthread_mutex_init(locks[i], NULL);
            pthread_create(&threads[i], NULL, sched, argsList[i]);   

        }
    }


    //----------------------------------------------------------------------
    if(inputSpecified && !randomSpecified) {

        struct timeval arrival;
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
                    gettimeofday(&arrival, NULL);
                    int arrivalT = (arrival.tv_sec - start.tv_sec )*1000 + (arrival.tv_usec - start.tv_usec)/1000;
                    initPCB(newPcb, pid, burstTime, arrivalT);
                    
                    pid ++;
                    if(SAP == 0) {
                        pthread_mutex_lock(locks[0]);
                        addToQueue(&queues[0], newPcb);
                        pthread_mutex_unlock(locks[0]);
                    }
                    else {
                        if(QS == 0) {
                            pthread_mutex_lock(locks[index]);
                            addToQueue(&queues[index], newPcb);
                            pthread_mutex_unlock(locks[index]);
                            index = (index + 1) % N;
                        }
                        else if(QS == 1) {
                            int shortestIndex = findShortest(queues, N);
                            pthread_mutex_lock(locks[shortestIndex]);
                            addToQueue(&queues[shortestIndex], newPcb);
                            pthread_mutex_unlock(locks[shortestIndex]);

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
            pthread_mutex_lock(inputsFinishedLock);
            inputsFinished = 1;
            pthread_mutex_unlock(inputsFinishedLock);

            for(int i = 0; i < N; i++) {
                pcb *dummy = malloc(sizeof(pcb));
                initPCB(dummy,-1,0, -1);
                pthread_mutex_lock(locks[i]);

                addToQueue(&queues[i], dummy);
                pthread_mutex_unlock(locks[i]);
                if(SAP == 0) {
                    break;
                }

            }
    }

    //----------------------------------------------------------------------
    else {
        struct timeval arrival;

        int pid =1;
        struct timeval tv;
        int index = 0;

        double x = 0;
        double u = 0;

        double y = 0;
        double v = 0;


        srand(time(0));

        double rateX = 1 / T;
        double rateY = 1 / L;

        for (int i = 0; i < PC; i++) {
            do {
                u = (double) rand() / (double) (RAND_MAX);
                x = ((- 1) * (log(1 - u))) / rateX;
            } while (x > T2 || x < T1);
            
            
            do {
                v = (double) rand() / (double) (RAND_MAX);
                y = ((- 1) * (log(1 - v))) / rateY;
            } while (y > L2 || y < L1);

            int roundedX = round(x);
            int roundedY = round(y);

            pcb *newPcb = malloc(sizeof(pcb));
            gettimeofday(&arrival, NULL);
            int arrivalT = (arrival.tv_sec - start.tv_sec )*1000 + (arrival.tv_usec - start.tv_usec)/1000;
            initPCB(newPcb, pid, roundedY,arrivalT);
            pid ++;
            if(SAP == 0) {
                pthread_mutex_lock(locks[0]);
                addToQueue(&queues[0], newPcb);
                pthread_mutex_unlock(locks[0]);
            }
            else {
                if(QS == 0) {
                    pthread_mutex_lock(locks[index]);
                    addToQueue(&queues[index], newPcb);
                    pthread_mutex_unlock(locks[index]);
                    index = (index + 1) % N;
                }
                else if(QS == 1) {
                   int shortestIndex = findShortest(queues, N);
                    pthread_mutex_lock(locks[shortestIndex]);
                    addToQueue(&queues[shortestIndex], newPcb);
                    pthread_mutex_unlock(locks[shortestIndex]);

                }
            } 

            if (i != PC -1) {
                usleep(roundedX * 1000);
            }

            
        }
        pthread_mutex_lock(inputsFinishedLock);
        inputsFinished = 1;
        pthread_mutex_unlock(inputsFinishedLock);
        for(int i = 0; i < N; i++) {

            pcb *dummy = malloc(sizeof(pcb));
            initPCB(dummy,-1,0, -1);
            pthread_mutex_lock(locks[i]);

            addToQueue(&queues[i], dummy);
            pthread_mutex_unlock(locks[i]);
            
            if(SAP == 0) {
                break;
            }

        }
    }



    //----------------------------------------------------------------------
    for(int i = 0; i < N; i++) {
        pthread_join(threads[i], NULL);
        free(argsList[i]);

    }

    for(int i = 0; i < N; i++) {
        free(locks[i]);
        freeQueue(queues[i]);
        if(SAP == 0) {
            break;
        }
    }
    free(queues);
    free(locks);
    free(inputsFinishedLock);
    free(finishedProcessLock);
    freeQueue(finished);
    return 0;
}