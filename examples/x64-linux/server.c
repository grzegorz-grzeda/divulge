/*
 * MIT License
 *
 * Copyright (c) 2023 Grzegorz Grzęda
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "server.h"
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#define G2LABS_LOG_MODULE_LEVEL G2LABS_LOG_MODULE_LEVEL_INFO
#define G2LABS_LOG_MODULE_NAME "divulge-server"
#include "g2labs-log.h"
#include "queue.h"

#define CHECK_IF_INVALID(x, msg) \
    do {                         \
        if ((x) < 0) {           \
            E(msg);              \
            exit(-1);            \
        }                        \
    } while (0)

typedef struct server {
    pthread_t* thread_pool;
    pthread_mutex_t mutex;         // = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t condition_var;  // = PTHREAD_COND_INITIALIZER;
    queue_t* pool_queue;
    server_connection_handler_t connection_handler;
    void* connection_handler_context;
    int socket_fd;
} server_t;

typedef struct server_connection {
    int id;
} server_connection_t;

static void* thread_pool_handler(void* context) {
    server_t* server = (server_t*)context;
    while (true) {
        pthread_mutex_lock(&server->mutex);
        server_connection_t* connection = queue_dequeue(server->pool_queue);
        if (!connection) {
            pthread_cond_wait(&server->condition_var, &server->mutex);
            connection = queue_dequeue(server->pool_queue);
        }
        pthread_mutex_unlock(&server->mutex);
        if (connection) {
            server->connection_handler(server, connection,
                                       server->connection_handler_context);
            free(connection);
        }
    }
}

server_t* server_create(uint16_t port,
                        int max_waiting_connections,
                        size_t thread_pool_size,
                        server_connection_handler_t connection_handler,
                        void* connection_handler_context) {
    server_t* server = calloc(1, sizeof(server_t));
    if (!server) {
        return NULL;
    }
    server->connection_handler = connection_handler;
    server->connection_handler_context = connection_handler_context;
    server->pool_queue = queue_initialize();
    pthread_mutex_init(&server->mutex, NULL);
    pthread_cond_init(&server->condition_var, NULL);
    server->thread_pool = calloc(thread_pool_size, sizeof(pthread_t));
    for (size_t i = 0; i < thread_pool_size; i++) {
        pthread_create(server->thread_pool + i, NULL, thread_pool_handler,
                       server);
    }

    server->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    CHECK_IF_INVALID(server->socket_fd, "Could not create a new socket");
    int true_value = 1;
    int status = setsockopt(server->socket_fd, SOL_SOCKET, SO_REUSEADDR,
                            &true_value, sizeof(int));
    CHECK_IF_INVALID(status, "Could not set socket options");

    struct sockaddr_in serv_addr;
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port);

    status = bind(server->socket_fd, (struct sockaddr*)&serv_addr,
                  sizeof(serv_addr));
    CHECK_IF_INVALID(status, "Could not bind to port");

    status = listen(server->socket_fd, max_waiting_connections);
    CHECK_IF_INVALID(status, "Could not listen");
    return server;
}

size_t server_read(server_connection_t* connection,
                   char* data,
                   size_t max_data_size) {
    if (!connection || !data || (max_data_size < 1)) {
        return 0;
    }
    ssize_t read_bytes = read(connection->id, data, max_data_size);
    if (read_bytes > 0) {
        return (size_t)read_bytes;
    } else {
        return 0;
    }
}

void server_write(server_connection_t* connection,
                  const char* data,
                  size_t data_size) {
    if (!connection || !data || (data_size < 1)) {
        return;
    }
    write(connection->id, data, data_size);
}

void server_close(server_connection_t* connection) {
    if (!connection) {
        return;
    }
    close(connection->id);
}

void server_loop(server_t* server) {
    if (!server) {
        return;
    }
    int connection_fd = accept(server->socket_fd, (struct sockaddr*)NULL, NULL);
    CHECK_IF_INVALID(connection_fd, "Could not accept a connection");
    server_connection_t* connection = calloc(1, sizeof(server_connection_t));
    connection->id = connection_fd;
    pthread_mutex_lock(&server->mutex);
    queue_enqueue(server->pool_queue, connection);
    pthread_cond_signal(&server->condition_var);
    pthread_mutex_unlock(&server->mutex);
}