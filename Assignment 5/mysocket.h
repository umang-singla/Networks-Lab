#pragma once
#ifndef _MYSOCKET_H
#define _MYSOCKET_H

#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#define SOCK_MyTCP 252
#define MAX_BUF_SIZE 10
#define MAX_MSG_SIZE 5001

typedef struct Msg
{
    int size;
    char buf[MAX_MSG_SIZE];
} msg;

typedef struct Table
{
    int size;
    int ptr;
    msg *buf;
} table;

int my_socket(int, int, int);
int my_bind(int, const struct sockaddr *, socklen_t);
int my_listen(int, int);
int my_accept(int, struct sockaddr *, socklen_t *);
int my_connect(int, const struct sockaddr *, socklen_t);
ssize_t my_send(int, void *, size_t, int);
ssize_t my_recv(int, void *, size_t, int);
int my_close(int);

void *send_thread(void *);
void *recv_thread(void *);

#endif