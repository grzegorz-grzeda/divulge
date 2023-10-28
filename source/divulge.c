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
    } else if (return_code == 301) {
        return "Moved Permanently";
    } else if (return_code == 404) {
        return "Not found";
    } else if (return_code == 500) {
        return "Internal server error";
    } else {
        return "Other";
    }
}

static bool respond_with_404(divulge_request_t* request, void* context) {
    char buffer[1024];
    snprintf(buffer, sizeof(buffer) - 1, "Divulge Error: [%s] '%s' failed!",
             divulge_method_name_from_method(request->method), request->route);
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
    if (!divulge || !uri || !uri->handler.handler || !uri->uri) {
        return;
    }
    route_entry_t* entry = calloc(1, sizeof(route_entry_t));
    entry->middlewares = dynamic_list_create();
    memcpy(&entry->uri, uri, sizeof(*uri));
    dynamic_list_append(divulge->routes, entry);
}

void divulge_add_middleware_to_uri(divulge_t* divulge,
                                   divulge_uri_t* uri,
                                   divulge_handler_object_t* middleware) {
    if (!divulge || !uri || !middleware) {
        return;
    }
    for (dynamic_list_iterator_t* it = dynamic_list_begin(divulge->routes); it;
         it = dynamic_list_next(it)) {
        route_entry_t* entry = dynamic_list_get(it);
        if ((entry->uri.method == uri->method) &&
            (strcmp(entry->uri.uri, uri->uri) == 0)) {
            divulge_handler_object_t* object =
                calloc(1, sizeof(divulge_handler_object_t));
            memcpy(object, middleware, sizeof(*object));
            dynamic_list_append(entry->middlewares, object);
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

static bool are_urls_equal(const char* request_url, const char* route_url) {
    return ((strcmp(request_url, route_url) == 0) &&
            (strlen(request_url) == strlen(route_url)));
}

static char* extract_query_from_request_url(char* request_url) {
    char* query_separator = strchr(request_url, '?');
    if (query_separator) {
        *query_separator = '\0';
        query_separator++;
    }
    return query_separator;
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
    request.header = strstr(request_buffer, "\r\n") + 2;
    request.payload = strstr(request_buffer, "\r\n\r\n") + 4;
    char* method_name = strtok(request_buffer, " ");
    request.route = strtok(NULL, " ");
    request.url_query = extract_query_from_request_url((char*)request.route);
    request.method = convert_request_method_to_method_type(method_name);
    request.context = &request_context;
    D("Received request: [%s] %s", method_name, request.route);
    divulge_route_method_t method =
        convert_request_method_to_method_type(method_name);
    bool was_route_handled = false;
    for (dynamic_list_iterator_t* it = dynamic_list_begin(divulge->routes); it;
         it = dynamic_list_next(it)) {
        route_entry_t* entry = dynamic_list_get(it);
        if ((entry->uri.method == request.method) &&
            are_urls_equal(request.route, entry->uri.uri)) {
            bool can_execute_handler = true;
            for (dynamic_list_iterator_t* jt =
                     dynamic_list_begin(entry->middlewares);
                 jt; jt = dynamic_list_next(jt)) {
                divulge_handler_object_t* object = dynamic_list_get(jt);
                can_execute_handler =
                    object->handler(&request, object->context);
                if (!can_execute_handler) {
                    break;
                }
            }
            if (can_execute_handler) {
                entry->uri.handler.handler(&request,
                                           entry->uri.handler.context);
                was_route_handled = true;
            }
        }
    }
    if (!request.context->was_status_sent && !was_route_handled) {
        divulge->default_404_handler(&request,
                                     divulge->default_404_handler_context);
    }
    if (divulge->configuration.close) {
        divulge->configuration.close(connection_context);
    }
}
const char* divulge_find_request_header_key(divulge_request_t* request,
                                            const char* key) {
    return strstr(request->header, key);
}

const char* divulge_get_request_header_entry_value(const char* header_entry) {
    char* value = strstr(header_entry, ": ") + 2;
    char* end = strstr(value, "\r\n");
    *end = '\0';
    return value;
}

bool divulge_send_status(divulge_request_t* request, int return_code) {
    if (!request || request->context->was_status_sent) {
        return false;
    }
    size_t size =
        (size_t)sprintf(request->context->response_buffer, "HTTP/1.1 %d %s\r\n",
                        return_code, convert_return_code_to_text(return_code));
    request->context->divulge->configuration.send(
        request->context->connection_context, request->context->response_buffer,
        size);
    request->context->was_status_sent = true;
    return true;
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

bool divulge_send_header(divulge_request_t* request,
                         divulge_response_t* response) {
    if (!request || !response || request->context->was_header_sent) {
        return false;
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
    request->context->was_header_sent = true;
    return true;
}

bool divulge_send_payload(divulge_request_t* request,
                          divulge_response_t* response) {
    if (!request || !response) {
        return false;
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
    return true;
}

bool divulge_respond(divulge_request_t* request, divulge_response_t* response) {
    if (!request || !response) {
        return false;
    }
    divulge_send_status(request, response->return_code);
    divulge_send_header(request, response);
    divulge_send_payload(request, response);
    return true;
}

bool divulge_redirect(divulge_request_t* request, const char* new_location) {
    if (!request || !new_location) {
        return false;
    }
    divulge_header_entry_t header_entries[] = {
        {.key = "Location", .value = new_location}};
    divulge_response_t response = {
        .return_code = 301,
        .header = {.count = 1, .entries = header_entries},
        .payload = "",
        .payload_size = 0};
    divulge_respond(request, &response);
    return true;
}