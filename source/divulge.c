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
#include "divulge.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dynamic-list.h"

#define G2LABS_LOG_MODULE_LEVEL G2LABS_LOG_MODULE_LEVEL_INFO
#define G2LABS_LOG_MODULE_NAME "divulge"
#include "g2labs-log.h"

typedef struct route_entry {
    divulge_uri_t uri;
} route_entry_t;
typedef struct divulge {
    divulge_configuration_t configuration;
    dynamic_list_t* routes;
    divulge_route_handler_t default_404_handler;
} divulge_t;

static divulge_route_method_t convert_request_method_to_method_type(
    const char* method_name) {
    if (strcmp(method_name, "GET") == 0) {
        return DIVULGE_ROUTE_METHOD_GET;
    } else if (strcmp(method_name, "POST") == 0) {
        return DIVULGE_ROUTE_METHOD_POST;
    } else {
        return DIVULGE_ROUTE_METHOD_ANY;
    }
}

static const char* convert_return_code_to_text(int return_code) {
    if (return_code == 200) {
        return "OK";
    } else if (return_code == 404) {
        return "Not found";
    } else {
        return "Other";
    }
}

static bool respond_with_404(divulge_request_t* request) {
    char buffer[1024];
    snprintf(buffer, sizeof(buffer) - 1,
             "Divulge Routing Error: resource '%s' not found!", request->route);
    divulge_response_t response = {
        .payload = buffer,
        .payload_size = strlen(buffer),
        .return_code = 404,
    };
    divulge_respond(request, &response);
    return true;
}

divulge_t* divulge_initialize(divulge_configuration_t* configuration) {
    if (!configuration || !configuration->socket_send_response_callback_t) {
        return NULL;
    }
    divulge_t* divulge = calloc(1, sizeof(divulge_t));
    if (!divulge) {
        return NULL;
    }
    memcpy(&divulge->configuration, configuration,
           sizeof(divulge_configuration_t));
    divulge->routes = dynamic_list_create();
    divulge->default_404_handler = respond_with_404;
    return divulge;
}

void divulge_register_uri(divulge_t* divulge, divulge_uri_t* uri) {
    if (!divulge || !uri || !uri->handler || !uri->uri) {
        return;
    }
    route_entry_t* entry = calloc(1, sizeof(route_entry_t));
    memcpy(&entry->uri, uri, sizeof(*uri));
    dynamic_list_append(divulge->routes, entry);
}

void divulge_set_default_404_handler(divulge_t* divulge,
                                     divulge_route_handler_t handler) {
    if (!divulge || !handler) {
        return;
    }
    divulge->default_404_handler = handler;
}

void divulge_process_request(divulge_t* divulge,
                             void* connection_context,
                             char* request_buffer,
                             size_t request_buffer_size,
                             const char* response_buffer,
                             size_t response_buffer_size) {
    if (!divulge || !request_buffer || (request_buffer_size == 0) ||
        !response_buffer || (response_buffer_size == 0)) {
        return;
    }
    divulge_request_t request = {
        .divulge = divulge,
        .connection_context = connection_context,
        .header = NULL,
        .payload = NULL,
    };
    char* method_name = strtok(request_buffer, " ");
    request.route = strtok(NULL, " ");
    request.method = convert_request_method_to_method_type(method_name);
    request._response_buffer = response_buffer;
    request._response_buffer_size = response_buffer_size;
    D("Received request: [%s] %s", method_name, request.route);
    divulge_route_method_t method =
        convert_request_method_to_method_type(method_name);
    bool was_route_handled = false;
    for (dynamic_list_iterator_t* it = dynamic_list_begin(divulge->routes); it;
         it = dynamic_list_next(it)) {
        route_entry_t* entry = dynamic_list_get(it);
        if ((entry->uri.method == request.method) &&
            (strcmp(request.route, entry->uri.uri) == 0)) {
            request.handler_context = entry->uri.handler_context;
            entry->uri.handler(&request);
            was_route_handled = true;
        }
    }
    if (!was_route_handled) {
        divulge->default_404_handler(&request);
    }
}

void divulge_respond(divulge_request_t* request, divulge_response_t* response) {
    if (!request || !response || !request->_response_buffer ||
        (request->_response_buffer_size < 2)) {
        return;
    }
    snprintf(request->_response_buffer, request->_response_buffer_size - 1,
             "HTTP/1.1 %d %s\r\n\r\n%*s", response->return_code,
             convert_return_code_to_text(response->return_code),
             (int)response->payload_size, response->payload);

    size_t response_size = strlen(request->_response_buffer);
    request->divulge->configuration.socket_send_response_callback_t(
        request->connection_context, request->_response_buffer, response_size);
}