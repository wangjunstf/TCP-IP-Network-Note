#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>

#define NUM_THREAD 100
void* thread_inc(void* arg);
void* thread_des(void* arg);
long long num=0;

int main(int argc, char* argv[]){
    pthread_t thread_id[NUM_THREAD];
    for(int i=0; i<NUM_THREAD; i++){
        if(i%2){
            pthread_create(&thread_id[i], NULL, thread_inc, NULL);
        }else{
            pthread_create(&thread_id[i], NULL, thread_des, NULL);
        }
    }

    for(int i=0; i<NUM_THREAD; i++){
        pthread_join(thread_id[i], NULL);
    }

    printf("result : %lld\n",num);
    return 0;
}

void * thread_inc(void * arg){
    for(int i=0; i<10000;i++){
        num+=1;
    }

    return NULL;
}

void *thread_des(void *arg)
{
    for (int i = 0; i < 10000; i++)
    {
        num -= 1;
    }

    return NULL;
}