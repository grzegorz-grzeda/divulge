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
#ifndef STATIC_STRING_H
#define STATIC_STRING_H

#include <stddef.h>

/**
 * @defgroup static-string Static String
 * @brief Static parsing and extraction of C-strings
 *
 * Extract and process C-strings without (that often) memory allocation.
 * Tokenizing, extracting, slicing of string is easy and without all that memory
 * allocation.
 * @{
 */

typedef struct static_string {
    char* text;
    size_t length;
} static_string_t;

static_string_t static_string_create(const char* c_string);

static_string_t static_string_tokenize(const static_string_t* sstr, char token);

static_string_t static_string_snip_end(const static_string_t* sstr,
                                       const char* tokens);

/**
 * @}
 */
#endif  // STATIC_STRING_H