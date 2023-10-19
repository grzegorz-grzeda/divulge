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
#ifndef SERVER_H
#define SERVER_H
#include <stddef.h>
#include <stdint.h>

typedef struct server server_t;

typedef struct server_connection server_connection_t;

typedef void (*server_connection_handler_t)(server_t* server,
                                            server_connection_t* connection,
                                            void* context);

server_t* server_create(uint16_t port,
                        int max_waiting_connections,
                        size_t thread_pool_size,
                        server_connection_handler_t connection_handler,
                        void* connection_handler_context);

size_t server_read(server_connection_t* connection,
                   char* data,
                   size_t max_data_size);

void server_write(server_connection_t* connection,
                  const char* data,
                  size_t data_size);

void server_close(server_connection_t* connection);

void server_loop(server_t* server);

#endif  // SERVER_H