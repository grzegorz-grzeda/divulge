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
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#define G2LABS_LOG_MODULE_LEVEL G2LABS_LOG_MODULE_LEVEL_INFO
#define G2LABS_LOG_MODULE_NAME "divulge-x64"
#include "divulge.h"
#include "g2labs-log.h"
#include "server.h"

#define DIVULGE_EXAMPLE_PORT (5000)
#define DIVULGE_EXAMPLE_MAX_WAITING_CONNECTIONS (100)
#define DIVULGE_EXAMPLE_THREAD_POOL_SIZE (10)
#define DIVULGE_EXAMPLE_REQUEST_BUFFER_SIZE (1024)

static void socket_send_response(void* connection_context,
                                 const char* data,
                                 size_t data_size) {
    server_connection_t* connection = (server_connection_t*)connection_context;
    server_write(connection, data, data_size);
}

static bool root_handler(divulge_request_t* request) {
    static int counter = 0;
    char buffer[1024];
    snprintf(buffer, sizeof(buffer) - 1,
             "<h1>Hello from Divulge router!</h1><h3>Counter: %d</h3>",
             counter);
    counter++;
    divulge_response_t response = {
        .return_code = 200,
        .payload = buffer,
        .payload_size = strlen(buffer),
    };
    divulge_respond(request, &response);
    return true;
}

static bool default_404_handler(divulge_request_t* request) {
    char buffer[1024];
    snprintf(buffer, sizeof(buffer) - 1,
             "<body><h2>Divulge Router Error</h2><code>Resource '%s' not "
             "found!</code></body>",
             request->route);
    divulge_response_t response = {
        .return_code = 200,
        .payload = buffer,
        .payload_size = strlen(buffer),
    };
    divulge_respond(request, &response);
    return true;
}

static divulge_t* initialize_router(void) {
    divulge_configuration_t configuration = {
        .socket_send_response_callback_t = socket_send_response,
    };
    divulge_t* divulge = divulge_initialize(&configuration);
    divulge_register_handler_for_route(divulge, "/", DIVULGE_ROUTE_METHOD_GET,
                                       root_handler);
    divulge_set_default_404_handler(divulge, default_404_handler);
    return divulge;
}

static void connection_handler(server_t* server,
                               server_connection_t* connection,
                               void* context) {
    divulge_t* router = (divulge_t*)context;
    char buffer[DIVULGE_EXAMPLE_REQUEST_BUFFER_SIZE];
    size_t bytes_read = server_read(connection, buffer, sizeof(buffer) - 1);
    buffer[bytes_read] = '\0';
    divulge_process_request(router, connection, buffer, bytes_read);
    server_close(connection);
}

int main(void) {
    I("Divulge example running on Linux(x64)");

    divulge_t* router = initialize_router();

    server_t* server = server_create(
        DIVULGE_EXAMPLE_PORT, DIVULGE_EXAMPLE_MAX_WAITING_CONNECTIONS,
        DIVULGE_EXAMPLE_THREAD_POOL_SIZE, connection_handler, router);
    while (true) {
        server_loop(server);
    }
    return 0;
}