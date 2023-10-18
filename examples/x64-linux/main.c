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
#include <stdio.h>

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#define G2LABS_LOG_MODULE_LEVEL G2LABS_LOG_MODULE_LEVEL_INFO
#define G2LABS_LOG_MODULE_NAME "divulge-x64"
#include "g2labs-log.h"

#define DIVULGE_EXAMPLE_PORT (5000)
#define DIVULGE_EXAMPLE_MAX_WAITING_CONNECTIONS (100)

#define CHECK_IF_INVALID(x, msg) \
    do {                         \
        if ((x) < 0) {           \
            E(msg);              \
            exit(-1);            \
        }                        \
    } while (0)

int main(void) {
    I("Divulge example running on Linux(x64)");

    int socket_file_descriptor = socket(AF_INET, SOCK_STREAM, 0);
    CHECK_IF_INVALID(socket_file_descriptor, "Could not create a new socket");
    int true_value = 1;
    setsockopt(socket_file_descriptor, SOL_SOCKET, SO_REUSEADDR, &true_value,
               sizeof(int));
    struct sockaddr_in serv_addr;
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(DIVULGE_EXAMPLE_PORT);

    int status = bind(socket_file_descriptor, (struct sockaddr*)&serv_addr,
                      sizeof(serv_addr));
    CHECK_IF_INVALID(status, "Could not bind to port");

    status =
        listen(socket_file_descriptor, DIVULGE_EXAMPLE_MAX_WAITING_CONNECTIONS);
    CHECK_IF_INVALID(status, "Could not listen");

    while (1) {
        int connection_file_descriptor =
            accept(socket_file_descriptor, (struct sockaddr*)NULL, NULL);
        CHECK_IF_INVALID(connection_file_descriptor,
                         "Could not accept a connection");
        char recvBuff[1025];
        size_t bytes_read =
            read(connection_file_descriptor, recvBuff, sizeof(recvBuff) - 1);
        recvBuff[bytes_read] = '\0';
        printf("%s", recvBuff);
        char sendBuff[1025];
        snprintf(sendBuff, sizeof(sendBuff) - 1,
                 "HTTP/1.1 200 OK\r\n\r\nHello");
        write(connection_file_descriptor, sendBuff, strlen(sendBuff));
        close(connection_file_descriptor);
    }
    return 0;
}