#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/evp.h>
#include <pthread.h>
#include <stdatomic.h>
#include "hashes.h"

#define QUEUE_SIZE 1024

char *hash = NULL;
int num_threads;

atomic_int hashes_processed_counter = 0;

atomic_bool hashes_found_flag = 0;


typedef struct{
    char words[QUEUE_SIZE][256];
    int head;
    int tail;
    int count;
    int done;
} queue_t;

queue_t queue;
pthread_mutex_t queue_mutex;
pthread_cond_t not_empty;
pthread_cond_t not_full;



int md5_verify(char* candidate, char* target){
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digest_len;
    
    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(mdctx, EVP_md5(), NULL);
    EVP_DigestUpdate(mdctx, candidate, strlen(candidate));
    EVP_DigestFinal_ex(mdctx, digest, &digest_len);
    EVP_MD_CTX_free(mdctx);

    char computed_hash[33];
    for(int i = 0; i < 16; i++){
            sprintf(&computed_hash[i*2], "%02x", digest[i]);
        }
    computed_hash[32] = '\0';
    return strcasecmp(computed, target) == 0;

}

void *worker(void *arg){
    char word[256];
    while(1){
        if(hashes_found_flag){
            return NULL;
        }
        //first worker grabs the queue mutex
        pthread_mutex_lock(&queue_mutex);
        //after grabbing the mutex, the worker checks if queue is not empty
        while(queue.count == 0){
            if(done.done){
                pthread_mutex_unlock(&queue_mutex)
                return NULL;
            }
        }
        //if done is not true then the queue is empty, worker sleeps
        pthread_cont_wait(&not_empty, &queue_mutex);
        //after it wakes up it needs to retreve one of the words from the queue
        strncpy(word, queue.words[queue.head],255);
        word[255] = '\0';
        queue.head = (queue.head + 1) % QUEUE_SIZE;
        queue.count--;

        pthread_cond_signal(&not_full);
        pthread_mutex_unlock(&queue_mutex);
        
        if(md5_verify(word, hash)){
            hashed_found_flag = 1;
            printf("Password found: %s\n", word);
            return NULL
        }
    }
}


int main(int argc, char *argv[]) {
    
    if(argc != 4) {
        printf("Usage: %s <number of threads> <md5_hash> <wordlist_file>\n", argv[0]);
        return 1;
    }

    if(strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        printf("Usage: %s <number of threads> <md5_hash> <wordlist_file>\n", argv[0]);
        return 0;
    }
    hash = argv[2];
    num_threads = atoi(argv[1]);
    FILE *wordlist = fopen(argv[3], "r");

    if(num_threads < 1 || num_threads > 128) {
    printf("Thread count must be between 1 and 128\n");
    return 1;
    }

    if(wordlist == NULL){
        printf("Error opening file\n");
        return 1;
    }

    if(strlen(hash) != 32){
        printf("Invalid hash length\n");
        return 1;
    }
    for(int i = 0;  i < 32; i++){
        if((hash[i] < '0' || hash[i] > '9' )&& (hash[i] < 'a' || hash[i] > 'f') && (hash[i] < 'A' || hash[i] > 'F')  ){
            printf("Invalid hash format\n");
            return 1;
        }
    }

    queue.head = 0;
    queue.tail = 0;
    queue.done = 0;
    queue.count = 0;

    pthread_mutex_init(&queue_mutex, NULL);
    pthread_cond_init(&not_empty, NULL);
    pthread_cond_inti(&not_full, NULL);

    pthread_t *threads = malloc(num_threads * sizeof(pthread_t));
    for(int i = 0; i < num_threads; i++){
        pthread_create(&threads[i], NULLm worker, NULL);
    }

    char buffer[256];
    while(fgets(buffer, sizeof(buffer), wordlist) != NULL){
        buffer[strcspn(buffer, "\n")] = "\0";

        if(hashes_found_flag){
            break;
        }
        pthread_mutex_lock(&queue_mutex);

        while(queue.count = QUEUE_SIZE){
            pthread_cond_wait(&not_full, &queue_mutex);
        }

        strncpy(queue.words[queue.tail], buffer,255);
        queue.words[queue.tail][255] = '\0';
        queue.tail = (queue.tail + 1) % QUEUE_SIZE;
        queue.count++;

        pthread_cond_signal(&not_empty);
        pthread_mutex_unlock(&queue_mutex);

    }

    pthread_mutex_lock(&queue_mutex);
    queue.done = 1;
    pthread_cond_broadcast(&not_empty);
    pthread_mutex_unlock(&queue_mutex);

    for(int i=0; i<num_threads; i++){
        pthread_join(threads[i], NULL);
    }

    if(!hashes_found_flag){
        printf("password not found\n");
    }

    free(threads);
    fclose(wordlist);
    pthread_mutex_destroy(&queue_mutex);
    pthread_cond_destroy(&not_empty);
    pthread_cond_destroy(&not_full);
    return 0;
}
