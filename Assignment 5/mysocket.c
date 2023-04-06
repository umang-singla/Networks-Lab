#include "mysocket.h"

// Declaring the global variables to be shared between threads
table send_table;
table recv_table;
pthread_t send_thread_id, recv_thread_id;
pthread_mutex_t send_mutex, recv_mutex;
pthread_cond_t send_cond, recv_cond;
int sock_fd;
_Bool connection_closed;

// Function to initialize the table, locks, condition variables, create the threads and return the socket file descriptor
int my_socket(int domain, int type, int protocol)
{
    int sockfd;
    if (type != SOCK_MyTCP)
    {
        printf("Only support SOCK_MyTCP\n");
        exit(1);
    }

    // Creating the socket
    sockfd = socket(domain, SOCK_STREAM, protocol);
    if (sockfd < 0)
    {
        perror("socket");
        exit(1);
    }

    // Initializing the locks and condition variables
    send_mutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
    recv_mutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
    send_cond = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
    recv_cond = (pthread_cond_t)PTHREAD_COND_INITIALIZER;

    // Initializing the tables
    send_table.buf = (msg *)malloc(sizeof(msg) * MAX_BUF_SIZE);
    recv_table.buf = (msg *)malloc(sizeof(msg) * MAX_BUF_SIZE);

    for (int i = 0; i < MAX_BUF_SIZE; i++)
    {
        send_table.buf[i].size = 0;
        recv_table.buf[i].size = 0;
    }

    send_table.size = 0;
    recv_table.size = 0;
    send_table.ptr = 0;
    recv_table.ptr = 0;

    sock_fd = -1;
    connection_closed = 0; // 0 means connection is open, 1 means connection is closed

    // Creating the threads
    int S = pthread_create(&send_thread_id, NULL, (void *)&send_thread, &sockfd);
    if (S != 0)
    {
        perror("pthread_create");
        exit(1);
    }

    int R = pthread_create(&recv_thread_id, NULL, (void *)&recv_thread, &sockfd);
    if (R != 0)
    {
        perror("pthread_create");
        exit(1);
    }

    return sockfd;
}

// Wrapper over the bind call
int my_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    return bind(sockfd, addr, addrlen);
}

// Wrapper over the listen call
int my_listen(int sockfd, int backlog)
{
    return listen(sockfd, backlog);
}

// Wrapper over the accept call, also sets the global variable sock_fd (a replacement for newsockfd)
int my_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
    sock_fd = accept(sockfd, addr, addrlen);
    return sock_fd;
}

// Wrapper over the connect call, also sets the global variable sock_fd (a replacement for sockfd)
int my_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    sock_fd = sockfd;
    return connect(sockfd, addr, addrlen);
}

// Utility function to find the minimum of two numbers
int min(int a, int b)
{
    return (a < b ? a : b);
}

// Wrapper over the send call
ssize_t my_send(int sockfd, void *buf, size_t len, int flags)
{
    if (len == 0)
        return 0;

    if (sock_fd == -1)
        return -1;

    len = min(len, MAX_MSG_SIZE);

    pthread_mutex_lock(&send_mutex);        // Lock the mutex
    while (send_table.size == MAX_BUF_SIZE) // If the buffer is full, wait
        pthread_cond_wait(&send_cond, &send_mutex);
    for (int i = 0; i < len; i++)
    {
        send_table.buf[send_table.ptr].buf[i] = ((char *)buf)[i];
    }
    send_table.buf[send_table.ptr].size = len;

    send_table.ptr++; // Increment the pointer
    send_table.ptr %= MAX_BUF_SIZE;
    send_table.size++;

    if (send_table.size == 1)
        pthread_cond_signal(&send_cond);
    pthread_mutex_unlock(&send_mutex);
    return len;
}

// Wrapper over the recv call
ssize_t my_recv(int sockfd, void *buf, size_t len, int flags)
{
    ssize_t recv_len = 0;

    if (sock_fd == -1)
        return -1;

    pthread_mutex_lock(&recv_mutex);                       // Lock the mutex
    while (recv_table.size == 0 && connection_closed == 0) // If the buffer is empty and connection is open, wait
        pthread_cond_wait(&recv_cond, &recv_mutex);

    if (connection_closed)
    {
        pthread_mutex_unlock(&recv_mutex);
        return -1;
    }

    recv_table.size--; // Decrement the size of the buffer
    recv_len = min(len, recv_table.buf[recv_table.ptr].size);
    memcpy(buf, recv_table.buf[recv_table.ptr].buf, recv_len); // Copy the data from the buffer to the user buffer
    recv_table.ptr = (recv_table.ptr + 1) % MAX_BUF_SIZE;
    pthread_mutex_unlock(&recv_mutex);

    if (recv_table.size == MAX_BUF_SIZE - 1) // If the buffer was full, signal the sender
        pthread_cond_broadcast(&recv_cond);

    return min(len, recv_len);
}

// Wrapper over the close call, Also cancels the threads and frees the memory, Also destroys the locks and condition variables, Also sets the global variable sock_fd to -1
int my_close(int sockfd)
{
    sleep(5);
    if (connection_closed == 0)
    {
        if (send_thread_id != -1)
            pthread_cancel(send_thread_id);

        if (recv_thread_id != -1)
            pthread_cancel(recv_thread_id);

        send_thread_id = -1;
        recv_thread_id = -1;

        if (send_table.buf != NULL)
            free(send_table.buf);
        if (recv_table.buf != NULL)
            free(recv_table.buf);

        send_table.buf = NULL;
        recv_table.buf = NULL;

        pthread_mutex_destroy(&send_mutex);
        pthread_mutex_destroy(&recv_mutex);
        pthread_cond_destroy(&send_cond);
        pthread_cond_destroy(&recv_cond);

        connection_closed = 1;
    }

    return close(sockfd);
}

// Thread to recieve data
void *recv_thread(void *arg)
{
    while (sock_fd == -1) // Wait till the connection is established
        sleep(1);

    while (1)
    {
        int BUFFER_SIZE = MAX_MSG_SIZE + 4; // 4 bytes for the length of the message
        char buffer[BUFFER_SIZE];
        int len_msg, len = 0, temp_len;
        char temp[5];

        while (len < 4) // Recieve the length of the message, till length becomes 4
        {
            temp_len = recv(sock_fd, buffer + len, 4-len, 0);
            len += temp_len;
            if (temp_len == 0)
                break;
        }

        if (temp_len) // If the connection is not closed
        {
            strncpy(temp, buffer, 4);
            temp[4] = '\0';
            len_msg = atoi(temp); // Convert the length to integer, to get the msg length

            while (len < len_msg + 4) // Recieve the complete message, till the length of the message is recieved
            {
                temp_len = recv(sock_fd, buffer + len, len_msg+4 - len, 0);
                len += temp_len;
                if (temp_len == 0)
                    break;
            }
        }
        pthread_mutex_lock(&recv_mutex);

        if (temp_len == 0) // If the connection is closed
        {
            connection_closed = 1;
            pthread_cond_broadcast(&recv_cond);
        }

        while (recv_table.size == MAX_BUF_SIZE)         // If the buffer is full, wait
            pthread_cond_wait(&recv_cond, &recv_mutex); // Wait for the buffer to be empty

        int end = (recv_table.ptr + recv_table.size) % MAX_BUF_SIZE; // Get the end of the buffer
        recv_table.buf[end].size = len_msg;                          // Store the length of the message
        memcpy(recv_table.buf[end].buf, buffer + 4, len_msg);
        recv_table.size++;

        if (recv_table.size == 1) // If the buffer was empty, signal the receiver
            pthread_cond_broadcast(&recv_cond);
        pthread_mutex_unlock(&recv_mutex);
    }
}

// send_thread function
void *send_thread(void *fd)
{
    // waiting for the connection
    while (sock_fd == -1)
        sleep(1);

    while (1)
    {
        // allocation the memory for string to send
        char *s = (char *)malloc(MAX_MSG_SIZE + 10);

        // aquiring the mutex lock
        pthread_mutex_lock(&send_mutex);

        // waiting for an entry in the send_table
        while (send_table.size == 0)
            pthread_cond_wait(&send_cond, &send_mutex);

        int send_pos = send_table.ptr - send_table.size;
        if (send_pos < 0)
            send_pos += MAX_BUF_SIZE;

        // adding length of message as the header
        sprintf(s, "%04d", send_table.buf[send_pos].size);

        int n = strlen(s);

        // fetching the string to send from the send_table
        for (int i = 0; i < send_table.buf[send_pos].size; i++)
        {
            s[n] = send_table.buf[send_pos].buf[i];
            n++;
        }

        // freeing the allocated memory
        free(s);

        // reducing the send_table size
        send_table.size--;

        // sending the signal to waiting threads if space is available in the table
        if (send_table.size == MAX_BUF_SIZE - 1)
            pthread_cond_signal(&send_cond);

        // releasing the mutex lock
        pthread_mutex_unlock(&send_mutex);

        // sending the message in chunks of 1000 bytes
        int i = 0;
        while (i < n)
        {
            if (n - i > 1000)
            {
                send(sock_fd, s + i, 1000, 0);
                i += 1000;
            }
            else
            {
                send(sock_fd, s + i, n - i, 0);
                i = n;
            }
        }

        // sleeping for 0.5 seconds
        sleep(0.5);
    }
}