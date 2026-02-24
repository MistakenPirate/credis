#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "handle.h"

#ifdef __linux__
#include <sys/epoll.h>
#elif defined(__APPLE__)
#include <sys/event.h>
#endif

#define MAX_EVENTS 64

HashTable kv_store = {0};

// Hash function (djb2)
static unsigned long hash(const char *str)
{
    unsigned long hash = 5381;
    int c;
    while ((c = *str++))
    {
        hash = ((hash << 5) + hash) + c;
    }
    return hash % HASH_TABLE_SIZE;
}

static HashNode *ht_find(const char *key)
{
    unsigned long index = hash(key);
    HashNode *node = kv_store.buckets[index];
    while (node)
    {
        if (strcmp(node->key, key) == 0)
        {
            return node;
        }
        node = node->next;
    }
    return NULL;
}

static int ht_set(const char *key, const char *value)
{
    HashNode *existing = ht_find(key);
    if (existing)
    {
        strncpy(existing->value, value, MAX_VALUE_LEN - 1);
        existing->value[MAX_VALUE_LEN - 1] = '\0';
        return 1;
    }

    unsigned long index = hash(key);
    HashNode *node = malloc(sizeof(HashNode));
    if (!node)
    {
        return 0;
    }

    strncpy(node->key, key, MAX_KEY_LEN - 1);
    node->key[MAX_KEY_LEN - 1] = '\0';
    strncpy(node->value, value, MAX_VALUE_LEN - 1);
    node->value[MAX_VALUE_LEN - 1] = '\0';

    node->next = kv_store.buckets[index];
    kv_store.buckets[index] = node;
    kv_store.count++;
    return 1;
}

static int ht_delete(const char *key)
{
    unsigned long index = hash(key);
    HashNode *node = kv_store.buckets[index];
    HashNode *prev = NULL;

    while (node)
    {
        if (strcmp(node->key, key) == 0)
        {
            if (prev)
            {
                prev->next = node->next;
            }
            else
            {
                kv_store.buckets[index] = node->next;
            }
            free(node);
            kv_store.count--;
            return 1;
        }
        prev = node;
        node = node->next;
    }
    return 0;
}

void parse_resp_command(char *buffer, char *tokens[], int *token_count)
{
    *token_count = 0;
    char *ptr = buffer;
    char *end;

    while (*ptr && *token_count < 10)
    {
        if (*ptr == '*' || *ptr == '$')
        {
            end = strstr(ptr, "\r\n");
            if (end)
            {
                ptr = end + 2;
            }
            else
            {
                break;
            }
            continue;
        }

        end = strstr(ptr, "\r\n");
        if (end)
        {
            *end = '\0';
            tokens[(*token_count)++] = ptr;
            ptr = end + 2;
        }
        else
        {
            if (*ptr)
            {
                tokens[(*token_count)++] = ptr;
            }
            break;
        }
    }
}

char *handle(char *buffer, char *result)
{
    memset(result, 0, 1028);

    char *tokens[10];
    int token_count = 0;
    parse_resp_command(buffer, tokens, &token_count);

    if (token_count == 0)
    {
        snprintf(result, 1028, "-ERR no command provided\r\n");
        return result;
    }

    if (strcasecmp(tokens[0], "SET") == 0 && token_count == 3)
    {
        if (ht_set(tokens[1], tokens[2]))
        {
            snprintf(result, 1028, "+OK\r\n");
        }
        else
        {
            snprintf(result, 1028, "-ERR failed to set key\r\n");
        }
        return result;
    }

    if (strcasecmp(tokens[0], "GET") == 0 && token_count == 2)
    {
        HashNode *node = ht_find(tokens[1]);
        if (node)
        {
            snprintf(result, 1028, "$%zu\r\n%s\r\n", strlen(node->value), node->value);
        }
        else
        {
            snprintf(result, 1028, "$-1\r\n");
        }
        return result;
    }

    if (strcasecmp(tokens[0], "DEL") == 0 && token_count == 2)
    {
        int deleted = ht_delete(tokens[1]);
        snprintf(result, 1028, ":%d\r\n", deleted);
        return result;
    }

    if (strcasecmp(tokens[0], "EXISTS") == 0 && token_count == 2)
    {
        HashNode *node = ht_find(tokens[1]);
        snprintf(result, 1028, ":%d\r\n", node ? 1 : 0);
        return result;
    }

    if (strcasecmp(tokens[0], "PING") == 0)
    {
        snprintf(result, 1028, "+PONG\r\n");
        return result;
    }

    snprintf(result, 1028, "-ERR unknown command\r\n");
    return result;
}

static void handle_client(int fd)
{
    char buffer[1024] = {0};
    int valread = read(fd, buffer, sizeof(buffer) - 1);
    if (valread <= 0)
    {
        if (valread == 0)
        {
            printf("Client disconnected\n");
        }
        else
        {
            perror("read error");
        }
        close(fd);
        return;
    }

    printf("Message received: %s\n", buffer);

    char response[1028];
    char *handled_resp = handle(buffer, response);
    send(fd, handled_resp, strlen(handled_resp), 0);
}

void multiplexing(char *host, int port)
{
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(host);
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, SOMAXCONN) < 0)
    {
        perror("Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server running on %s:%d\n", host, port);

#ifdef __linux__
    // Linux: use epoll
    int epfd = epoll_create1(0);
    if (epfd == -1)
    {
        perror("epoll_create1 failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    struct epoll_event ev, events[MAX_EVENTS];
    ev.events = EPOLLIN;
    ev.data.fd = server_fd;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, server_fd, &ev) == -1)
    {
        perror("epoll_ctl failed");
        close(server_fd);
        close(epfd);
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        int nfds = epoll_wait(epfd, events, MAX_EVENTS, -1);
        if (nfds == -1)
        {
            perror("epoll_wait failed");
            break;
        }

        for (int i = 0; i < nfds; i++)
        {
            int fd = events[i].data.fd;

            if (fd == server_fd)
            {
                int addrlen = sizeof(address);
                int new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
                if (new_socket == -1)
                {
                    perror("accept failed");
                    continue;
                }

                ev.events = EPOLLIN;
                ev.data.fd = new_socket;
                if (epoll_ctl(epfd, EPOLL_CTL_ADD, new_socket, &ev) == -1)
                {
                    perror("epoll_ctl add client failed");
                    close(new_socket);
                    continue;
                }
                printf("New connection accepted\n");
            }
            else
            {
                handle_client(fd);
                epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
            }
        }
    }

    close(epfd);

#elif defined(__APPLE__)
    // macOS: use kqueue
    int kq = kqueue();
    if (kq == -1)
    {
        perror("kqueue failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    struct kevent change;
    EV_SET(&change, server_fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
    if (kevent(kq, &change, 1, NULL, 0, NULL) == -1)
    {
        perror("kevent failed");
        close(server_fd);
        close(kq);
        exit(EXIT_FAILURE);
    }

    struct kevent events[MAX_EVENTS];

    while (1)
    {
        int nev = kevent(kq, NULL, 0, events, MAX_EVENTS, NULL);
        if (nev == -1)
        {
            perror("kevent wait failed");
            break;
        }

        for (int i = 0; i < nev; i++)
        {
            int fd = (int)events[i].ident;

            if (events[i].flags & EV_EOF)
            {
                printf("Client disconnected\n");
                close(fd);
                continue;
            }

            if (fd == server_fd)
            {
                int addrlen = sizeof(address);
                int new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
                if (new_socket == -1)
                {
                    perror("accept failed");
                    continue;
                }

                EV_SET(&change, new_socket, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
                if (kevent(kq, &change, 1, NULL, 0, NULL) == -1)
                {
                    perror("kevent add client failed");
                    close(new_socket);
                    continue;
                }
                printf("New connection accepted\n");
            }
            else
            {
                handle_client(fd);
            }
        }
    }

    close(kq);

#else
#error "Unsupported platform. Use Linux or macOS."
#endif

    close(server_fd);
}
