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

#define DIVULGE_SERVER_NAME "Divulge"
typedef struct middleware_entry {
    divulge_uri_handler_t handler;
    void* context;
} middleware_entry_t;
typedef struct route_entry {
    divulge_uri_t uri;
    dynamic_list_t* middlewares;
} route_entry_t;
typedef struct divulge {
    divulge_configuration_t configuration;
    dynamic_list_t* routes;
    divulge_uri_handler_t default_404_handler;
    void* default_404_handler_context;
} divulge_t;

typedef struct divulge_request_context {
    divulge_t* divulge;
    void* connection_context;
    char* response_buffer;
    size_t response_buffer_size;
    bool was_status_sent;
    bool was_header_sent;
} divulge_request_context_t;

const char* divulge_method_name_from_method(divulge_route_method_t method) {
    if (method == DIVULGE_ROUTE_METHOD_GET) {
        return "GET";
    } else if (method == DIVULGE_ROUTE_METHOD_POST) {
        return "POST";
    } else {
        return "ANY";
    }
}

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

static bool respond_with_404(divulge_request_t* request, void* context) {
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
    if (!configuration || !configuration->send || !configuration->close) {
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
    entry->middlewares = dynamic_list_create();
    memcpy(&entry->uri, uri, sizeof(*uri));
    dynamic_list_append(divulge->routes, entry);
}

void divulge_add_middleware_to_uri(divulge_t* divulge,
                                   divulge_uri_t* uri,
                                   divulge_uri_handler_t middleware,
                                   void* context) {
    if (!divulge || !uri || !middleware) {
        return;
    }
    for (dynamic_list_iterator_t* it = dynamic_list_begin(divulge->routes); it;
         it = dynamic_list_next(it)) {
        route_entry_t* entry = dynamic_list_get(it);
        if ((entry->uri.method == uri->method) &&
            (strcmp(entry->uri.uri, uri->uri) == 0)) {
            middleware_entry_t* middleware_entry =
                calloc(1, sizeof(middleware_entry_t));
            middleware_entry->context = context;
            middleware_entry->handler = middleware;
            dynamic_list_append(entry->middlewares, middleware_entry);
        }
    }
}

void divulge_set_default_404_handler(divulge_t* divulge,
                                     divulge_uri_handler_t handler,
                                     void* context) {
    if (!divulge || !handler) {
        return;
    }
    divulge->default_404_handler_context = context;
    divulge->default_404_handler = handler;
}

void divulge_process_request(divulge_t* divulge,
                             void* connection_context,
                             char* request_buffer,
                             size_t request_buffer_size,
                             char* response_buffer,
                             size_t response_buffer_size) {
    if (!divulge || !request_buffer || (request_buffer_size == 0) ||
        !response_buffer || (response_buffer_size == 0)) {
        return;
    }
    divulge_request_t request = {
        .header = NULL,
        .payload = NULL,
    };
    divulge_request_context_t request_context = {
        .divulge = divulge,
        .connection_context = connection_context,
        .response_buffer = response_buffer,
        .response_buffer_size = response_buffer_size,
        .was_status_sent = false,
        .was_header_sent = false,
    };

    char* method_name = strtok(request_buffer, " ");
    request.route = strtok(NULL, " ");
    request.method = convert_request_method_to_method_type(method_name);
    request.context = &request_context;
    D("Received request: [%s] %s", method_name, request.route);
    divulge_route_method_t method =
        convert_request_method_to_method_type(method_name);
    bool was_route_handled = false;
    for (dynamic_list_iterator_t* it = dynamic_list_begin(divulge->routes); it;
         it = dynamic_list_next(it)) {
        route_entry_t* entry = dynamic_list_get(it);
        for (dynamic_list_iterator_t* jt =
                 dynamic_list_begin(entry->middlewares);
             jt; jt = dynamic_list_next(jt)) {
            middleware_entry_t* middleware_entry = dynamic_list_get(jt);
            middleware_entry->handler(&request, middleware_entry->context);
        }
        if ((entry->uri.method == request.method) &&
            (strcmp(request.route, entry->uri.uri) == 0)) {
            entry->uri.handler(&request, entry->uri.context);
            was_route_handled = true;
        }
    }
    if (!was_route_handled) {
        divulge->default_404_handler(&request,
                                     divulge->default_404_handler_context);
    }
    if (divulge->configuration.close) {
        divulge->configuration.close(connection_context);
    }
}

void divulge_send_status(divulge_request_t* request, int return_code) {
    if (!request || request->context->was_status_sent) {
        return;
    }
    size_t size =
        (size_t)sprintf(request->context->response_buffer, "HTTP/1.1 %d %s\r\n",
                        return_code, convert_return_code_to_text(return_code));
    request->context->divulge->configuration.send(
        request->context->connection_context, request->context->response_buffer,
        size);
    request->context->was_status_sent = true;
}

static void send_header_entry(divulge_request_t* request,
                              const char* key,
                              const char* value) {
    if (!request->context->was_status_sent) {
        return;
    }
    size_t size = (size_t)sprintf(request->context->response_buffer,
                                  "%s: %s\r\n", key, value);
    request->context->divulge->configuration.send(
        request->context->connection_context, request->context->response_buffer,
        size);
}

void divulge_send_header(divulge_request_t* request,
                         divulge_response_t* response) {
    if (!request || !response || request->context->was_header_sent) {
        return;
    }
    if (!request->context->was_status_sent) {
        divulge_send_status(request, response->return_code);
    }
    send_header_entry(request, "Server", DIVULGE_SERVER_NAME);
    if (response->header.entries && (response->header.count > 0)) {
        for (size_t i = 0; i < response->header.count; i++) {
            divulge_header_entry_t* entry = response->header.entries + i;
            send_header_entry(request, entry->key, entry->value);
        }
    }
}

void divulge_send_payload(divulge_request_t* request,
                          divulge_response_t* response) {
    if (!request || !response) {
        return;
    }
    if (!request->context->was_header_sent) {
        divulge_send_header(request, response);
    }
    size_t response_size =
        (size_t)snprintf(request->context->response_buffer,
                         request->context->response_buffer_size - 1, "\r\n%*s",
                         (int)response->payload_size, response->payload);

    request->context->divulge->configuration.send(
        request->context->connection_context, request->context->response_buffer,
        response_size);
}

void divulge_respond(divulge_request_t* request, divulge_response_t* response) {
    if (!request || !response) {
        return;
    }
    divulge_send_status(request, response->return_code);
    divulge_send_header(request, response);
    divulge_send_payload(request, response);
}