#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "pthread.h"
#include <stdlib.h>
#include "a2_helper.h"
#include <semaphore.h>

typedef struct {
    int threadNr;
    sem_t* semaphore;
} PARAM;

typedef struct {
    int processNr;
    int threadNr;
} DADPARAM;

sem_t tatalSexy;
int nrOfThreads = 0;
int waitingFor14 = 0;
int finishedThreads = 0;

#define NR_THREADS 50

void *thread_function8(void *args) {
    PARAM* param = (PARAM*)args; 
    sem_wait(param->semaphore);
    
    info(BEGIN, 8, param->threadNr);
    nrOfThreads++;
    if (param->threadNr == 14 && finishedThreads < 46) {
        waitingFor14 = 1;
        while (nrOfThreads != 4) ;
    } else  while (waitingFor14 == 1) ;

    nrOfThreads--;
    finishedThreads++;
    info(END, 8, param->threadNr);
    if (param->threadNr == 14) waitingFor14 = 0;

    sem_post(param->semaphore);
    return NULL;
}

void *limited_dad_area(void *args) {
    DADPARAM* param = (DADPARAM*)args; 

    info(BEGIN, param->processNr, param->threadNr);
    info(END, param->processNr, param->threadNr);

    return NULL;
}

void *thread61(void *unused) {
    info(BEGIN, 6, 1); info(END, 6, 1);
    return NULL;
}

void *thread31(void *sema) {
    sem_t* sem = (sem_t*)sema;

    sem_wait(sem);

    DADPARAM param;
    param.processNr = 3;
    param.threadNr = 1;
    
    limited_dad_area(&param);

    sem_post(sem);
    return NULL;
}

void *thread62(void *sema) {
    sem_t* sem = (sem_t*)sema;

    sem_wait(sem);

    DADPARAM param;
    param.processNr = 6;
    param.threadNr = 2;
    
    limited_dad_area(&param);

    sem_post(sem);

    pthread_t t31;
    pthread_create(&t31, NULL, thread31, &tatalSexy);
    pthread_join(t31, NULL);
    return NULL;
}

void *thread63(void *unused) {
    info(BEGIN, 6, 3); info(END, 6, 3);
    return NULL;
}

void *thread64(void *unused) {
    info(BEGIN, 6, 4); 

    pthread_t tid;
    pthread_create(&tid, NULL, thread63, NULL);
    pthread_join(tid, NULL);

    info(END, 6, 4);
    return NULL;
}

void *thread32(void *unused) {
    info(BEGIN, 3, 2); info(END, 3, 2);
    return NULL;
}

void *thread33(void *sema) {
    sem_t* sem = (sem_t*)sema;

    sem_wait(sem);

    DADPARAM param;
    param.processNr = 3;
    param.threadNr = 3;
    
    limited_dad_area(&param);

    sem_post(sem);
    return NULL;
}

void *thread34(void *unused) {
    info(BEGIN, 3, 4); info(END, 3, 4);
    return NULL;
}

void process8() {
    if (fork() != 0) return;
    info(BEGIN, 8, 0); 
    
    pthread_t tids[NR_THREADS];
    PARAM *params = (PARAM*)malloc(NR_THREADS * sizeof(PARAM));
    sem_t logSem;

    if(sem_init(&logSem, 0, 4) != 0) {
        perror("Could not init the semaphore");
        return;
    }


    for(int i=0; i<NR_THREADS; i++) {
        params[i].threadNr = i + 1;
        params[i].semaphore = &logSem;

        pthread_create(&tids[i], NULL, thread_function8, (void*)&params[i]);
    }

    for(int i=0; i<NR_THREADS; i++) pthread_join(tids[i], NULL);

    free(params);
    sem_destroy(&logSem);
    
    info(END, 8, 0);
    exit(0);
}

void process7() {
    if (fork() != 0) return;
    info(BEGIN, 7, 0);
    
    process8(); wait(NULL);

    info(END, 7, 0);
    exit(0);
}


void process6() {
    if (fork() != 0) return;
    info(BEGIN, 6, 0);
    pthread_t tid[3];

    pthread_create(&tid[0], NULL, thread61, NULL);
    pthread_create(&tid[1], NULL, thread62, &tatalSexy);
    pthread_create(&tid[2], NULL, thread64, NULL);

    pthread_join(tid[0], NULL);
    pthread_join(tid[1], NULL);
    pthread_join(tid[2], NULL);

    info(END, 6, 0);
    exit(0);
}

void process5() {
    if (fork() != 0) return;
    info(BEGIN, 5, 0); info(END, 5, 0);
    exit(0);
}

void process4() {
    if (fork() != 0) return;
    info(BEGIN, 4, 0); info(END, 4, 0);
    exit(0);
}

void process3() {
    if (fork() != 0) return;
    info(BEGIN, 3, 0);

    if(sem_init(&tatalSexy, 0, 1) != 0) {
        perror("Could not init the dad semaphore");
        return;
    }

    pthread_t tid[4];
    pthread_create(&tid[1], NULL, thread32, NULL);
    pthread_create(&tid[2], NULL, thread33, &tatalSexy);
    pthread_create(&tid[3], NULL, thread34, NULL);

    pthread_join(tid[1], NULL);
    pthread_join(tid[2], NULL);
    pthread_join(tid[3], NULL);
    
    process6(); wait(NULL);

    info(END, 3, 0); 
    exit(0);
}

void process2() {
    if (fork() != 0) return;
    info(BEGIN, 2, 0);

    process3(); process7();
    wait(NULL); wait(NULL);

    info(END, 2, 0);
    exit(0);
}


int main()
{
    init();
    info(BEGIN, 1, 0);

    process2(); process4(); process5(); 
    wait(NULL); wait(NULL); wait(NULL); 

    info(END, 1, 0);
    return 0;
}
