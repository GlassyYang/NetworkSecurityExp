/*
 * author: GlassyYang
 * data format: 
 *  ---------------------------------------------------------------------------------------------------------
 *  |2 bytes content length | 1 byte flag: more data | 1 byte flag: filename / error | data max length 1024 |
 *  ---------------------------------------------------------------------------------------------------------
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>

#define FILE_LENGTH 1024
#define BUFSIZE 1050
#define MAX_PACKGE_LENGTH 1028
//函数声明
void server(int sock, struct sockaddr_in addr);
void client(int sock, struct sockaddr_in addr, char *);
int store_file(FILE *, char *, ssize_t);
int read_file(FILE *, char *);
void *server_thread(void *);

int main(int argc, char **argv)
{
    char *format = "c:sp:f:";
    char flag = '\0';
    char ip[20];
    char filename[FILE_LENGTH];
    int res, port = -1;
    memset(ip, 0, sizeof(ip));
    int fp = 0;
    do
    {
        res = getopt(argc, argv, format);
        switch (res)
        {
        case 'c':
            flag = 'c';
            strncpy(ip, optarg, 15);
            break;
        case 's':
            flag = 's';
            break;
        case 'p':
            port = atoi(optarg);
            if (port < 0 || port > 65535)
            {
                printf("Wrong tcp port!\n");
                return 0;
            }
            break;
        case 'f':
            if (flag == 'c')
            {
                strncpy(filename, optarg, FILE_LENGTH - 1);
                fp = 1;
            }
            break;
        case ':':
            printf("missing parameter\n");
        default:
            if (res != -1)
            {
                printf("Unspecified oper: %c\n", res);
                return 0;
            }
        }
    } while (res != -1);
    if (flag == 0 || port == -1)
    {
        printf("Please input ip address and port!\n");
        return 0;
    }
    if (flag == 'c' && fp == 0)
    {
        printf("Must give transmitted file path!\n");
        return 0;
    }
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1)
    {
        perror("Cannot create: ");
        return 0;
    }
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(port);
    if (flag == 's')
    {
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
    }
    else if (0 == inet_pton(AF_INET, ip, &addr.sin_addr))
    {
        printf("Invalid ip address.\n");
        return 0;
    }
    switch (flag)
    {
    case 'c':
        client(sock, addr, filename);
        break;
    case 's':
        server(sock, addr);
        break;
    default:
        printf("Inner error!\n");
        return 0;
    }
    return 0;
}

void server(int sock, struct sockaddr_in addr)
{
    struct sockaddr_in cliaddr;
    socklen_t len;
    if (bind(sock, (struct sockaddr *)&addr, sizeof(struct sockaddr)) < 0)
    {
        perror("Cannot bind: ");
        exit(0);
    }
    if (listen(sock, 10) < 0)
    {
        perror("Cannot listen: ");
        exit(0);
    }
    printf("Server has opened. begin listening...\n");
    while (1)
    {
        int clisock = accept(sock, (struct sockaddr *)&cliaddr, &len);
        if(clisock < 0)
        {
            perror("Inner error!\n");
            exit(0);
        }
        printf("connect established from %s:%d\n", inet_ntoa(cliaddr.sin_addr), htons(cliaddr.sin_port));
        pthread_t thread;
        if (0 != pthread_create(&thread, NULL, server_thread, (void *)&clisock))
        {
            printf("Thread create failed, connect aborted.\n");
            if (close(clisock) < 0)
            {
                perror("cannot close: ");
            }
        }
    }
}

void client(int sock, struct sockaddr_in addr, char *filename)
{
    char buf[BUFSIZE];
    char *cbegin = buf + 4;
    ssize_t length = strlen(filename);
    FILE *fp = fopen(filename, "r");
    if (fp == NULL)
    {
        perror("Unable to open file: ");
        exit(0);
    }
    if (connect(sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) < 0)
    {
        perror("Cannot connect to target host: ");
        exit(0);
    }
    printf("Connected to trget server. Begin file transmit...\n");
    *(uint16_t *)&buf = length;
    buf[2] = 0;
    buf[3] = 1;
    memcpy(cbegin, filename, length);
    if (send(sock, buf, length + 4, 0) < 0)
    {
        perror("Send data filed: ");
        exit(0);
    }
    do
    {
        memset(buf, 0, BUFSIZE);
        length = read_file(fp, cbegin);
        *(uint16_t*)buf = length;
        buf[3] = 0;
        if(length < MAX_PACKGE_LENGTH - 4)
        {
            buf[2] = 0;
        }
        else
        {
            buf[2] = 1;
        }
        if (send(sock, buf, length + 4, 0) < 0)
        {
            perror("Error on sending data: ");
            exit(0);
        }
    } while (buf[2] == 1);
    printf("transmit finisned.\n");
    return;
}

int read_file(FILE *file, char *buffer)
{
    size_t length = fread((void *)buffer, sizeof(char), MAX_PACKGE_LENGTH - 4, file);
    if (length < MAX_PACKGE_LENGTH - 4)
    {
        if (!feof(file))
        {
            printf("Error occured while reading data from file.\n");
            return -1;
        }
    }
    return length;
}

int store_file(FILE *file, char *buffer, ssize_t size)
{
    int length = fwrite((void *)buffer, sizeof(char), size, file);
    for(int i = 0; i < 10000; i++);
    printf("length: %d size: %ld\n", length, size);
    if (length != size)
    {
        perror("Cannot write file: ");
        return -1;
    }
    else
    {
        return length;
    }
}

void *server_thread(void *arg)
{
    int sock = *(int*)arg;
    int more = 1;
    char buf[BUFSIZ];
    char *content_begin = NULL;
    memset((void *)buf, 0, BUFSIZE);
    FILE *fp = NULL;
    uint32_t content_length;
    ssize_t size;
    size = recv(sock, (void *)buf, MAX_PACKGE_LENGTH, 0);
    if (size < 0)
    {
        perror("Cannot receive data: ");
        goto OnExit;
    }
    if (size < 4)
    {
        printf("Inner error!\n");
        goto OnExit;
    }
    if (buf[3] != 1)
    {
        printf("Error on transmit!\n");
        goto OnExit;
    }
    content_begin = buf + 4;
    content_length = *(uint16_t *)buf;
    if (content_length > 1024)
    {
        printf("Error! data packge has been tampered!\n");
        goto OnExit;
    }
    if (content_begin[content_length] != '\0')
    {
        content_begin[content_length] = '\0';
    }
    fp = fopen(content_begin, "w+");
    if (fp == NULL)
    {
        perror("Cannot create or open file: \n");
        goto OnExit;
    }
    do
    {
        memset(buf, 0, BUFSIZE);
        size = recv(sock, buf, MAX_PACKGE_LENGTH, 0);
        if (size < 0)
        {
            perror("Cannot receive data from client: \n");
            goto OnExit;
        }
        content_length = *(uint16_t *)buf;
        if (content_length > 1024)
        {
            printf("Error! data packge has been tampered!\n");
            goto OnExit;
        }
        printf("%d\n", content_length);
        if (store_file(fp, content_begin, content_length) < 0)
        {
            printf("Error on store transminted file!\n");
            goto OnExit;
        }
    } while (buf[2] != 0);
    printf("Transmit finished.\n");
OnExit:
    if (close(sock) < 0)
    {
        perror("Cannot close socket: ");
    }
    if (fp != NULL)
    {
        if (fclose(fp) == EOF)
        {
            perror("Cannot close file: ");
        }
    }
    return NULL;
}
