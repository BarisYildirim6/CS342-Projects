#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include "rm.h"


// global variables

int DA;  // indicates if deadlocks will be avoided or not
int N;   // number of processes
int M;   // number of resource types
int ExistingRes[MAXR]; // Existing resources vector
int Available[MAXR];
int Allocation[MAXP][MAXR] = {0};
int Request[MAXP][MAXR] = {0};
int MaxDemand[MAXP][MAXR] = {0};
int Need[MAXP][MAXR] = {0};

struct pthread_id {
    int given_id;
    int real_id;
    int alive;
};
struct pthread_id threads[MAXP] = {{0, 0, 0}}; 
//..... other definitions/variables .....
//.....
//.....

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

// end of global variables

int rm_thread_started(int tid)
{       
    int ret = 0;
    // 1 for alive, 0 for not alive
    if (tid >= 0 && tid < MAXP) {
        threads[tid].given_id = tid;
        threads[tid].real_id = pthread_self();
        threads[tid].alive = 1;
    } else {
        ret = -1;
    }
    return (ret);
}


int rm_thread_ended()
{
    int ret = 0;
    int tid;
    for (int i = 0; i < MAXP; i++) {
        if (threads[i].real_id == pthread_self()) {
            tid = threads[i].given_id;
        }
    }
    // 1 for alive, 0 for not alive
    if (tid > 0 && tid < MAXP) {
        threads[tid].given_id = tid;
        threads[tid].real_id = pthread_self();
        threads[tid].alive = 0;
    } else {
        ret = -1;
    }
    return (ret);
}


int rm_claim (int claim[])
{
    int ret = 0;
    int tid;
    for (int i = 0; i < N; i++) {
        if (threads[i].real_id == pthread_self()) {
            tid = threads[i].given_id;
        }
    }
    for (int i = 0; i < M; i++) {
        MaxDemand[tid][i] = claim[i];
    }
    return(ret);
}


int rm_init(int p_count, int r_count, int r_exist[],  int avoid)
{
    int i;
    int ret = 0;

    if (p_count > MAXP || r_count > MAXR || p_count < 0 || r_count < 0) {
        ret = -1;
    }
    DA = avoid;
    N = p_count;
    M = r_count;
    // initialize (create) resources
    for (i = 0; i < M; ++i){
        if (r_exist[i] < 0) {
            ret = -1;
        }
        ExistingRes[i] = r_exist[i];
        Available[i] = ExistingRes[i];
    }
    // resources initialized (created)
    
    //....
    // ...
    return  (ret);
}


int rm_request (int request[])
{
    int ret = 0;
    
    return(ret);
}


int rm_release (int release[])
{
    int ret = 0;

    return (ret);
}


int rm_detection()
{
    pthread_mutex_lock(&lock);
    int ret = 0;
    int flag = 0;
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < M; j++) {
            if (Need[i][j] > Available[j]) {
                flag = 1;
            }
        }
        if (flag) {
            ret++;
        }
        flag = 0;
    }
    pthread_mutex_unlock(&lock);
    return (ret);
}


void rm_print_state (char hmsg[])
{
    pthread_mutex_lock(&lock);
    printf("##########################\n");
    printf("%s\n", hmsg);
    printf("##########################\n");
    //Existing Resources
    printf("Exist:\n\t");
    for (int i = 0; i < M; i++) {
        printf("R%d\t", i);
    }
    printf("\n\t");
    for (int i = 0; i < M; i++) {
        printf("%d\t", ExistingRes[i]);
    }
    printf("\n\n");
    //Available Resources
    printf("Available:\n\t");
    for (int i = 0; i < M; i++) {
        printf("R%d\t", i);
    }
    printf("\n\t");
    for (int i = 0; i < M; i++) {
        printf("%d\t", Available[i]);
    }
    printf("\n\n");
    //Allocation
    printf("Allocation:\n\t");
    for (int i = 0; i < M; i++) {
        printf("R%d\t", i);
    }
    printf("\n");
    for (int i = 0; i < N; i++) {
        printf("T%d:\t", i);
        for (int j = 0; j < M; j++) {
            printf("%d\t", Allocation[i][j]);
        }
        printf("\n");
    }
    printf("\n");
    //Request
    printf("Request:\n\t");
    for (int i = 0; i < M; i++) {
        printf("R%d\t", i);
    }
    printf("\n");
    for (int i = 0; i < N; i++) {
        printf("T%d:\t", i);
        for (int j = 0; j < M; j++) {
            printf("%d\t", Request[i][j]);
        }
        printf("\n");
    }
    printf("\n");
    //Max Demand
    printf("Max Demand:\n\t");
    for (int i = 0; i < M; i++) {
        printf("R%d\t", i);
    }
    printf("\n");
    for (int i = 0; i < N; i++) {
        printf("T%d:\t", i);
        for (int j = 0; j < M; j++) {
            printf("%d\t", MaxDemand[i][j]);
        }
        printf("\n");
    }
    printf("\n");
    //Need
    printf("Need:\n\t");
    for (int i = 0; i < M; i++) {
        printf("R%d\t", i);
    }
    printf("\n");
    for (int i = 0; i < N; i++) {
        printf("T%d:\t", i);
        for (int j = 0; j < M; j++) {
            printf("%d\t", Need[i][j]);
        }
        printf("\n");
    }
    printf("##########################\n\n");
    pthread_mutex_unlock(&lock);
    return;
}
