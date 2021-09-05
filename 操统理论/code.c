#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/time.h>
#include <unistd.h>
// #define DELAY
#define BUFFERSIZE 4

int buffer[BUFFERSIZE];
int in = 0, out = 0;

sem_t remains, mutex, used;

long thread_count;

void Get_args(int argc, char *argv[]);
void* producer();
void* consumer();

int main(int argc, char*argv[]){
    long       thread;  /* Use long in case of a 64-bit system */
    pthread_t* thread_handles;

    Get_args(argc, argv);
    thread_handles = (pthread_t*) malloc (thread_count*sizeof(pthread_t));
    sem_init(&remains, 0, BUFFERSIZE);
    sem_init(&mutex, 0, 1);
    sem_init(&used, 0, 0);

    for (thread = 0; thread < thread_count / 2; thread++)
        pthread_create(&thread_handles[thread], NULL,
                       producer, NULL);

    for (thread = thread_count / 2; thread < thread_count; thread++)
        pthread_create(&thread_handles[thread], NULL,
                       consumer, NULL);

    for (thread = 0; thread < thread_count; thread++)
        pthread_join(thread_handles[thread], NULL);


    sem_destroy(&remains);
    sem_destroy(&mutex);
    sem_destroy(&used);
    free(thread_handles);

    return 0;
}

void Get_args(int argc, char* argv[]) {
    if (argc != 2)
        printf("Miss thread_count.");
    else
        thread_count = strtol(argv[1], NULL, 10);
}  /* Get_args */

void* producer(){
    int n;
    while(1){

        int num = rand() % 10;
        printf("PRODUCER produce: %d\n", num);

        sem_wait(&remains);
        sem_wait(&mutex);

        buffer[in] = num;
        in = (in + 1) % BUFFERSIZE;
        sem_getvalue(&used,&n);
        printf("PRODUCER append: %d when used: %d \n", num, n);

        sem_post(&mutex);
        sem_post(&used);
    }
    
}

void* consumer(){
    int n;
    while(1){
        sem_wait(&used);
        sem_wait(&mutex);

        int num = buffer[out];
        out= (out+ 1) % BUFFERSIZE;
        sem_getvalue(&remains,&n);
        printf("CONSUMER take: %d when remains: %d \n",num, n);

        sem_post(&mutex);
        sem_post(&remains);

        printf("CONSUMER consume: %d\n", num);
    }
}

#define Size 64
int A[Size, Size], B[Size, Size], C[Size, Size];
int register i, j;
for (i = 0; i < Size; i++){
    for (j = 0; j < Size; j++)
        C[i][j] = A[i][j] + B[i][j];
}