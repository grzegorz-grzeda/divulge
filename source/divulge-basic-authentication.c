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
#include "divulge-basic-authentication.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "encodings-base64.h"

typedef struct divulge_basic_authentication_context {
    const char* realm;
    divulge_basic_authentication_authenticate_user_callback_t authentication_callback;
    void* authentication_context;
} divulge_basic_authentication_context_t;

static bool handler(divulge_request_t* request, void* context) {
    divulge_basic_authentication_context_t* ctx = (divulge_basic_authentication_context_t*)context;
    const char* authorization_header = divulge_find_request_header_key(request, "Authorization");
    bool result = false;
    if (!authorization_header) {
        result = false;
        return false;
    } else {
        const char* value = divulge_get_request_header_entry_value(authorization_header);
        char* encoding = strstr(value, " ") + 1;
        char* decoded = calloc(encodings_base64_get_decode_buffer_size(encoding) + 1, sizeof(char));
        encodings_base64_decode(encoding, decoded);
        char* username = strtok(decoded, ":");
        char* password = strtok(NULL, ":");
        if (!username || !password || !ctx->authentication_callback(ctx->authentication_context, username, password)) {
            result = false;
        } else {
            result = true;
        }
        free(decoded);
    }
    if (!result) {
        char realm_buffer[100];
        sprintf(realm_buffer, "Basic realm=\"%s\"", ctx->realm);
        divulge_header_entry_t header_entries[] = {
            {.key = "WWW-Authenticate", .value = realm_buffer},
        };
        divulge_response_t response = {
            .return_code = 401,
            .header =
                {
                    .count = 1,
                    .entries = header_entries,
                },
            .payload = "",
            .payload_size = 0,
        };
        divulge_respond(request, &response);
    }
    return result;
}

divulge_handler_object_t* divulge_basic_authentication_create(
    const char* realm,
    divulge_basic_authentication_authenticate_user_callback_t authentication_callback,
    void* authentication_context) {
    if (!realm || !authentication_callback) {
        return NULL;
    }
    divulge_handler_object_t* object = calloc(1, sizeof(divulge_handler_object_t));
    if (!object) {
        return NULL;
    }
    divulge_basic_authentication_context_t* ctx = calloc(1, sizeof(divulge_basic_authentication_context_t));
    if (!ctx) {
        free(object);
        return NULL;
    }
    ctx->authentication_callback = authentication_callback;
    ctx->authentication_context = authentication_context;
    ctx->realm = realm;
    object->context = ctx;
    object->handler = handler;
    return object;
}