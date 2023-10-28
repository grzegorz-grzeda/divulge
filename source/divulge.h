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
#ifndef DIVULGE_H
#define DIVULGE_H

#include <stdbool.h>
#include <stddef.h>
/**
 * @defgroup divulge Divulge
 * @brief Small HTTP router in C
 * @{
 */
typedef struct divulge divulge_t;

typedef enum divulge_route_method {
    DIVULGE_ROUTE_METHOD_GET,
    DIVULGE_ROUTE_METHOD_POST,
    DIVULGE_ROUTE_METHOD_ANY,
} divulge_route_method_t;

typedef struct divulge_request_context divulge_request_context_t;

typedef struct divulge_request {
    divulge_request_context_t* context;
    divulge_route_method_t method;
    const char* route;
    const char* url_query;
    const char* header;
    const char* payload;
} divulge_request_t;

typedef struct divulge_header_entry {
    const char* key;
    const char* value;
} divulge_header_entry_t;

typedef struct divulge_header {
    divulge_header_entry_t* entries;
    size_t count;
} divulge_header_t;

typedef struct divulge_response {
    int return_code;
    divulge_header_t header;
    const char* payload;
    size_t payload_size;
} divulge_response_t;

typedef bool (*divulge_uri_handler_t)(divulge_request_t* request, void* context);

typedef struct divulge_handler_object {
    divulge_uri_handler_t handler;
    void* context;
} divulge_handler_object_t;

typedef struct divulge_uri {
    const char* uri;
    divulge_route_method_t method;
    divulge_handler_object_t handler;
} divulge_uri_t;

typedef void (*divulge_socket_send_callback_t)(void* connection_context, const char* data, size_t data_size);

typedef void (*divulge_socket_close_callback_t)(void* connection_context);

typedef struct divulge_configuration {
    divulge_socket_send_callback_t send;
    divulge_socket_close_callback_t close;
} divulge_configuration_t;

const char* divulge_method_name_from_method(divulge_route_method_t method);

divulge_t* divulge_initialize(divulge_configuration_t* configuration);

void divulge_register_uri(divulge_t* divulge, divulge_uri_t* uri);

void divulge_add_middleware_to_uri(divulge_t* divulge, divulge_uri_t* uri, divulge_handler_object_t* middleware);

void divulge_set_default_404_handler(divulge_t* divulge, divulge_uri_handler_t handler, void* context);

void divulge_process_request(divulge_t* divulge,
                             void* connection_context,
                             char* request_buffer,
                             size_t request_buffer_size,
                             char* response_buffer,
                             size_t response_buffer_size);

const char* divulge_find_request_header_key(divulge_request_t* request, const char* key);

const char* divulge_get_request_header_entry_value(const char* header_entry);

bool divulge_send_status(divulge_request_t* request, int return_code);

bool divulge_send_header(divulge_request_t* request, divulge_response_t* response);

bool divulge_send_payload(divulge_request_t* request, divulge_response_t* response);

bool divulge_respond(divulge_request_t* request, divulge_response_t* response);

bool divulge_redirect(divulge_request_t* request, const char* new_location);
/**
 * @}
 */
#endif  // DIVULGE_H