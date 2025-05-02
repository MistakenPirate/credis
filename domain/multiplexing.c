#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/select.h>
#include <stdlib.h>
#include <string.h>
#include "handle.h"

#define PROMPT "redis > "
#define MAX_CLIENTS 10

KeyValue kv_store[100];
int kv_count = 0;

void parse_resp_command(char *buffer, char **tokens, int *token_count)
{
    char *token = strtok(buffer, "\r\n");
    *token_count = 0;

    while (token != NULL)
    {
        if (token[0] == '*' || token[0] == '$')
        {
            // Skip array length or string length
            token = strtok(NULL, "\r\n");
            continue;
        }
        tokens[(*token_count)++] = token;
        token = strtok(NULL, "\r\n");
    }
}

char *handle(char *buffer, int client_socket, int server_fd, fd_set master_set, char *result)
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
        if (kv_count < 100)
        {
            strncpy(kv_store[kv_count].key, tokens[1], MAX_KEY_LEN - 1);
            strncpy(kv_store[kv_count].value, tokens[2], MAX_VALUE_LEN - 1);
            kv_count++;
            snprintf(result, 1028, "+OK\r\n");
        }
        else
        {
            snprintf(result, 1028, "-ERR store limit reached\r\n");
        }
        return result;
    }

    if (strcasecmp(tokens[0], "GET") == 0 && token_count == 2)
    {
        for (int i = 0; i < kv_count; i++)
        {
            if (strcmp(kv_store[i].key, tokens[1]) == 0)
            {
                snprintf(result, 1028, "$%zu\r\n%s\r\n", strlen(kv_store[i].value), kv_store[i].value);
                return result;
            }
        }
        snprintf(result, 1028, "-ERR key not found\r\n");
        return result;
    }

    if (strcasecmp(tokens[0], "DEL") == 0 && token_count == 2)
    {
        int found = 0;
        for (int i = 0; i < kv_count; i++)
        {
            if (strcmp(kv_store[i].key, tokens[1]) == 0)
            {
                for (int j = i; j < kv_count - 1; j++)
                {
                    kv_store[j] = kv_store[j + 1];
                }
                kv_count--;
                found = 1;
                snprintf(result, 1028, ":1\r\n");
                return result;
            }
        }
        snprintf(result, 1028, ":0\r\n");
        return result;
    }

    if (strcasecmp(tokens[0], "EXISTS") == 0 && token_count == 2)
    {
        for (int i = 0; i < kv_count; i++)
        {
            if (strcmp(kv_store[i].key, tokens[1]) == 0)
            {
                snprintf(result, 1028, ":1\r\n");
                return result;
            }
        }
        snprintf(result, 1028, ":0\r\n");
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

void multiplexing(char *host, int port)
{
    int server_fd;
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd == -1)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

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

    if (listen(server_fd, MAX_CLIENTS) < 0)
    {
        perror("Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server running on http://%s:%d\n", host, port);

    fd_set master_set, read_fds;
    int fd_max = server_fd;
    FD_ZERO(&master_set);
    FD_SET(server_fd, &master_set);

    while (1)
    {
        read_fds = master_set;

        if (select(fd_max + 1, &read_fds, NULL, NULL, NULL) == -1)
        {
            perror("select failed");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i <= fd_max; i++)
        {
            if (FD_ISSET(i, &read_fds))
            {
                if (i == server_fd)
                {
                    int new_socket;
                    int addrlen = sizeof(address);
                    new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
                    if (new_socket == -1)
                    {
                        perror("accept failed");
                    }
                    else
                    {
                        FD_SET(new_socket, &master_set);
                        if (new_socket > fd_max)
                        {
                            fd_max = new_socket;
                        }
                        printf("New connection accepted\n");
                    }
                }
                else
                {
                    char buffer[1024] = {0};
                    int valread = read(i, buffer, 1024);
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
                        close(i);
                        FD_CLR(i, &master_set);
                    }
                    else
                    {
                        printf("Message received: %s\n", buffer);

                        char response[1028];
                        char *handled_resp = handle(buffer, i, server_fd, master_set, response);
                        send(i, handled_resp, strlen(handled_resp), 0);
                    }
                }
            }
        }
    }

    close(server_fd);
}