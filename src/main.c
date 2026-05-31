#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/evp.h>
#include <pthread.h>
#include <stdatomic.h>
#include "hashes.h"

#define QUEUE_SIZE 1024

char *hash = NULL;
FILE *wordlist = NULL;
int num_threads;

atomic_int hashes_processed_counter = 0;
atomic_bool hashes_found_flag = 0;
atomic_int words_checked = 0;

struct threadData {
    char *hashString;
    FILE *filePointer;
    pthread_mutex_t mutex;
    int foundFlag;
    pthread_mutex_t *foundMutex;
    char *foundPassword;
};

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
    if(strcasecmp(computed_hash, target) == 0){
            return 1;
    }

    return 0;
}


int main(int argc, char *argv[]) {
    
    pthread_mutex_init(&queue_mutex, NULL);

    char buffer[256];
    if(argc != 4) {
        printf("Usage: %s <number of threads> <md5_hash> <wordlist_file>\n", argv[0]);
        return 1;
    }

    if(strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        printf("Usage: %s <number of threads> <md5_hash> <wordlist_file>\n", argv[0]);
        return 0;
    }else{
        hash = argv[2];
        num_threads = atoi(argv[1]);
        wordlist = fopen(argv[3], "r");
    }

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
    }else{
        for(int i = 0;  i < 32; i++){
            if((hash[i] < '0' || hash[i] > '9' )&& (hash[i] < 'a' || hash[i] > 'f') && (hash[i] < 'A' || hash[i] > 'F')  ){
                printf("Invalid hash format\n");
                return 1;
            }
        }
    }

    while(fgets(buffer, sizeof(buffer), wordlist) != NULL){
        queue_t *item = malloc(sizeof(queue_t));
        pthread_mutex_lock(&queue_mutex);
        buffer[strcspn(buffer, "\n")] = '\0'; 
        item -> words = strdup(buffer);
        pthread_cond_signal(&not_empty);
        pthread_mutex_unlock(&queue_mutex);
        if(md5_verify(buffer, hash)){
            printf("Password found: %s\n", buffer);
            fclose(wordlist);
            return 1;
        }
        free(item);
    }

    printf("Password not found\n");
    fclose(wordlist);
    return 0;
}
