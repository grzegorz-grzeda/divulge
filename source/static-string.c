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
#include "static-string.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define G2LABS_LOG_MODULE_LEVEL G2LABS_LOG_MODULE_LEVEL_INFO
#define G2LABS_LOG_MODULE_NAME "static-string"
#include "g2labs-log.h"

#define STATIC_STRING_INITIALIZER \
    { .text = NULL, .length = 0 }

static_string_t static_string_create(const char* c_string) {
    static_string_t sstr = STATIC_STRING_INITIALIZER;
    sstr.text = c_string;
    if (sstr.text) {
        sstr.length = strlen(sstr.text);
    }
    return sstr;
}

static_string_t static_string_tokenize(const static_string_t* sstr,
                                       char token) {
    static_string_t result = STATIC_STRING_INITIALIZER;
    if (!sstr) {
        return result;
    }
    for (size_t i = 0; i < sstr->length; i++) {
        if (sstr->text[i] == token) {
            if (i < (sstr->length - 1)) {
                result.text = (sstr->text + i + 1);
                result.length = (sstr->length - i - 1);
            }
            return result;
        }
    }
    return result;
}

static_string_t static_string_snip_end(const static_string_t* sstr,
                                       const char* tokens) {
    static_string_t result = STATIC_STRING_INITIALIZER;
    if (!sstr || !tokens) {
        return result;
    }
    result = *sstr;
    size_t tokens_length = strlen(tokens);
    if (!tokens_length) {
        return result;
    }
    for (size_t i = 0; i < sstr->length; i++) {
        for (size_t j = 0; j < tokens_length; j++) {
            if (sstr->text[i] == tokens[j]) {
                result.length = i;
                return result;
            }
        }
    }
    return result;
}