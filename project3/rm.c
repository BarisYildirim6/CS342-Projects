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
int TempAlloc[MAXP][MAXR] = {0};
int Request[MAXP][MAXR] = {0};
int MaxDemand[MAXP][MAXR] = {0};
int Need[MAXP][MAXR] = {0};

struct pthread_id {
    int given_id;
    long unsigned int real_id;
    int alive;
};
struct pthread_id threads[MAXP] = {{0, 0, 0}}; 
//..... other definitions/variables .....
//.....
//.....

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cv = PTHREAD_COND_INITIALIZER;

// end of global variables

int rm_thread_started(int tid)
{       
    int ret = 0;
    pthread_mutex_lock(&lock);
    // 1 for alive, 0 for not alive
    if (tid >= 0 && tid < MAXP) {
        threads[tid].given_id = tid;
        threads[tid].real_id = pthread_self();
        threads[tid].alive = 1;
    } else {
        ret = -1;
    }
    pthread_mutex_unlock(&lock);
    return (ret);
}


int rm_thread_ended()
{
    int ret = 0;
    pthread_mutex_lock(&lock);
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
    pthread_mutex_unlock(&lock);
    return (ret);
}


int rm_claim (int claim[])
{
    int ret = 0;
    pthread_mutex_lock(&lock);
    // Finds thread's given id
    int tid = 0;
    for (int i = 0; i < N; i++) {
        if (threads[i].real_id == pthread_self()) {
            tid = threads[i].given_id;
            break;
        }
    }
    // populates Max Demand matrix according to claims if DA = 1
    for (int i = 0; i < M; i++) {
        if (ExistingRes[i] < claim[i]) {
            ret = -1;
            break;
        }
        if (DA) {
            MaxDemand[tid][i] = claim[i];
        }
    }
    pthread_mutex_unlock(&lock);
    return(ret);
}


int rm_init(int p_count, int r_count, int r_exist[],  int avoid)
{
    int i;
    int ret = 0;

    pthread_mutex_lock(&lock);
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
    pthread_mutex_unlock(&lock);
    return  (ret);
}

int rm_request (int request[])
{
    pthread_mutex_lock(&lock);
    int ret = 0;

    // Finds thread's given id
    int tid;
    for (int i = 0; i < N; i++) {
        if (threads[i].real_id == pthread_self()) {
            tid = threads[i].given_id;
        }
    }

    // populates Request matrix according to requests 
    // && checks if any errors exist such as any request
    // exceeding the existing resource limit or being 
    // less than 0
    for (int i = 0; i < M; i++) {
        if (request[i] > ExistingRes[i] || request[i] < 0) {
            pthread_mutex_unlock(&lock);
            ret = -1;
            return(ret);
        }
        Request[tid][i] = request[i];
    }
    // Checks availability for allocation (does this after every release)
    int alloc_condition;
    do {
        alloc_condition = 1;
        for (int i = 0; i < M; i++) {
            if (request[i] > Available[i]) {
                alloc_condition = 0;
                break;
            }
        }
        if (!alloc_condition) {
            pthread_cond_wait(&cv, &lock);
        }
    } while (!alloc_condition);

    if (DA) {
        int safe;
        do {
            for (int i = 0; i < M; i++) {
                // Allocate(Reallocate) to check if safe
                Allocation[tid][i] += request[i];
                Available[i] -= request[i];
            }
            safe = rm_detection();

            if (safe == 0) {
                // If safe reset Request matrix and end waiting
                // while already allocated
                for (int i = 0; i < M; i++) {
                    Request[tid][i] = 0;
                } 
                ret = 0;
                pthread_mutex_unlock(&lock);
                return(ret);
            } else {
                for (int i = 0; i < M; i++) {
                    //Deallocate until safe in order to reallocate and check safety
                    Allocation[tid][i] -= request[i];
                    Available[i] += request[i];
                }
                pthread_cond_wait(&cv, &lock);
            }
        } while (safe > 0);
    } else {
        for (int i = 0; i < M; i++) {
            Allocation[tid][i] += request[i];
            Available[i] -= request[i];
            Request[tid][i] = 0;
        }
        ret =  0;
        pthread_mutex_unlock(&lock);
        return(ret);
    }
    // Should not reach here (Checked)
    ret = -1;
    pthread_mutex_unlock(&lock);
    return(ret);
}


int rm_release (int release[])
{
    pthread_mutex_lock(&lock);
    int ret = 0;
    // Finds thread's given id
    int tid;
    for (int i = 0; i < N; i++) {
        if (threads[i].real_id == pthread_self()) {
            tid = threads[i].given_id;
        }
    }
    // If allocation is less than release that should not happen
    // Also allocation can not be less then zero
    for (int i = 0; i < M; i++) {
        if (Allocation[tid][i] < release[i] || release[i] < 0) {
            ret = -1;
            pthread_mutex_unlock(&lock);
            return(ret);
        }
    }

    // Release occurs
    for (int i = 0; i < M; i++) {
        Allocation[tid][i] -= release[i];
        Available[i] += release[i];
        if (DA) {
            Need[tid][i] += release[i];
        }
    }

    // All condition variables are signaled in order to recheck
    // happen for both availability and safety
    pthread_cond_broadcast(&cv);
    pthread_mutex_unlock(&lock);
    return (ret);
}


int rm_detection()
{
    // Banker's algoritm implemented (without Safety Sequence)
    int ret = 0;
    int f[N], flag;
    int work[M];
    for (int i = 0; i < M; i++) {
        work[i] = Available[i];
    }
    for (int i = 0; i <  N; i++) {
        f[i] = 0;
    }

    // Need matrix is populated according to current MaxDemand
    // && Allocation matrices
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < M; j++) {
            Need[i][j] = MaxDemand[i][j] - Allocation[i][j];
        }
    }
    int i = 0, j = 0;
    for (i = 0; i < N; i++) {
        if (f[i] == 0) {
            flag = 0;
            for (j = 0; j < M; j++) {
                if (Need[i][j] > work[j]) {
                    flag = 1;
                    break;
                }
            }
            if (!flag) {
                f[i] = 1;
                for (j = 0; j < M; j++) {
                    work[j] += Allocation[i][j];
                }
                i = -1;
            }
        }
    }
    for (i = 0; i < N; i++) {
        if (f[i] == 0) {
            ret++;
        }
    }
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
