#ifndef HANDLE_H
#define HANDLE_H

#define MAX_KEY_LEN 256
#define MAX_VALUE_LEN 256
#define HASH_TABLE_SIZE 1024

typedef struct HashNode {
    char key[MAX_KEY_LEN];
    char value[MAX_VALUE_LEN];
    struct HashNode *next;
} HashNode;

typedef struct {
    HashNode *buckets[HASH_TABLE_SIZE];
    int count;
} HashTable;

void multiplexing(char *host, int port);
char *handle(char *buffer, char *result);

#endif