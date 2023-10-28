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
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#define G2LABS_LOG_MODULE_LEVEL G2LABS_LOG_MODULE_LEVEL_INFO
#define G2LABS_LOG_MODULE_NAME "divulge-x64"
#include "divulge-basic-authentication.h"
#include "divulge.h"
#include "file-names.h"
#include "g2labs-log.h"
#include "static-string.h"
#include "stream-server.h"

#define DIVULGE_EXAMPLE_PORT (5000)
#define DIVULGE_EXAMPLE_MAX_WAITING_CONNECTIONS (100)
#define DIVULGE_EXAMPLE_THREAD_POOL_SIZE (20)
#define DIVULGE_EXAMPLE_BUFFER_SIZE (1024)

static void socket_send_response(void* connection_context,
                                 const char* data,
                                 size_t data_size) {
    stream_server_connection_t* connection =
        (stream_server_connection_t*)connection_context;
    stream_server_write(connection, data, data_size);
}

static void socket_close(void* connection_context) {
    stream_server_connection_t* connection =
        (stream_server_connection_t*)connection_context;
    stream_server_close(connection);
}

static bool root_handler(divulge_request_t* request, void* context) {
    char* file_buffer = NULL;
    divulge_header_entry_t header_entries[] = {
        {.key = "Content-Type", .value = "text/html"}};
    divulge_response_t response = {
        .return_code = 200,
        .header = {.count = 1, .entries = header_entries},
        .payload = "",
        .payload_size = 0};
    struct stat st;
    const char* file_name = ROOT_FILE_NAME;
    int result = stat(file_name, &st);
    if (result != 0) {
        response.return_code = 500;
        response.payload = "Error while accessing file",
        response.payload_size = strlen(response.payload);
    } else {
        size_t file_size = st.st_size;
        file_buffer = calloc(file_size, sizeof(char));
        FILE* f = fopen(file_name, "r");
        size_t bytes_read = fread(file_buffer, file_size, 1, f);
        fclose(f);
        response.payload = file_buffer;
        response.payload_size = bytes_read;
    }
    bool status = divulge_respond(request, &response);
    if (file_buffer) {
        free(file_buffer);
    }
    return status;
}

static bool root_post_handler(divulge_request_t* request, void* context) {
    I("Received POST /: '%s'", request->payload);
    return divulge_redirect(request, "/");
}

static bool restricted_access_handler(divulge_request_t* request,
                                      void* context) {
    char buffer[DIVULGE_EXAMPLE_BUFFER_SIZE];
    snprintf(buffer, sizeof(buffer) - 1,
             "<h2>Restricted access area!</h2><p>If you can see this, it means "
             "you logged in :)</p>");
    divulge_header_entry_t header_entries[] = {
        {.key = "Content-Type", .value = "text/html"}};
    divulge_response_t response = {
        .return_code = 200,
        .header = {.count = 1, .entries = header_entries},
        .payload = buffer,
        .payload_size = strlen(buffer)};
    return divulge_respond(request, &response);
}

static divulge_uri_t root_uri = {
    .uri = "/",
    .handler = {.handler = root_handler},
    .method = DIVULGE_ROUTE_METHOD_GET,
};

static divulge_uri_t root_post_uri = {
    .uri = "/",
    .handler = {.handler = root_post_handler},
    .method = DIVULGE_ROUTE_METHOD_POST,
};

static divulge_uri_t restricted_uri = {
    .uri = "/restricted",
    .handler = {.handler = restricted_access_handler},
    .method = DIVULGE_ROUTE_METHOD_GET,
};

static bool logger_middleware_handler(divulge_request_t* request,
                                      void* context) {
    I("[%s] '%s'", divulge_method_name_from_method(request->method),
      request->route);
    return true;
}

static divulge_handler_object_t logger_middleware = {
    .handler = logger_middleware_handler,
    .context = NULL,
};

static bool authenticate_user(void* context,
                              const char* username,
                              const char* password) {
    return ((strcmp(username, "g2") == 0) && (strcmp(password, "g3") == 0));
}

static divulge_t* initialize_router(void) {
    divulge_configuration_t configuration = {
        .send = socket_send_response,
        .close = socket_close,
    };
    divulge_t* divulge = divulge_initialize(&configuration);
    divulge_register_uri(divulge, &root_uri);
    divulge_add_middleware_to_uri(divulge, &root_uri, &logger_middleware);
    divulge_register_uri(divulge, &root_post_uri);
    divulge_add_middleware_to_uri(divulge, &root_post_uri, &logger_middleware);
    divulge_register_uri(divulge, &restricted_uri);
    divulge_add_middleware_to_uri(divulge, &restricted_uri,
                                  divulge_basic_authentication_create(
                                      "G2Labs realm", authenticate_user, NULL));
    return divulge;
}

static void connection_handler(stream_server_t* server,
                               stream_server_connection_t* connection,
                               void* context) {
    divulge_t* router = (divulge_t*)context;
    char request_buffer[DIVULGE_EXAMPLE_BUFFER_SIZE];
    char response_buffer[DIVULGE_EXAMPLE_BUFFER_SIZE];
    size_t request_bytes_read = stream_server_read(connection, request_buffer,
                                                   sizeof(request_buffer) - 1);
    request_buffer[request_bytes_read] = '\0';
    divulge_process_request(router, connection, request_buffer,
                            request_bytes_read, response_buffer,
                            sizeof(response_buffer));
}

int main(void) {
    I("Divulge example running on Linux(x64)");

    divulge_t* router = initialize_router();

    stream_server_t* server = stream_server_create(
        DIVULGE_EXAMPLE_PORT, DIVULGE_EXAMPLE_MAX_WAITING_CONNECTIONS,
        DIVULGE_EXAMPLE_THREAD_POOL_SIZE, connection_handler, router);
    while (true) {
        stream_server_loop(server);
    }
    return 0;
}