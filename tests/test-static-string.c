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
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "cmocka.h"

#include "static-string.h"

static void test_create_empty(void** state) {
    const char* empty = NULL;
    static_string_t sstr = static_string_create(empty);
    assert_ptr_equal(sstr.text, NULL);
    assert_int_equal(sstr.length, 0);

    empty = "";
    sstr = static_string_create(empty);
    assert_ptr_equal(sstr.text, empty);
    assert_int_equal(sstr.length, 0);
}

static void test_create(void** state) {
    const char* c_string = "Test C-String";
    static_string_t sstr = static_string_create(c_string);
    assert_memory_equal(c_string, sstr.text, strlen(c_string));
    assert_int_equal(sstr.length, strlen(c_string));
}

static void test_tokenize(void** state) {
    const char* test_url = "/some-resource/detail?q=1&a=3&5=a";
    static_string_t url = static_string_create(test_url);
    assert_memory_equal(test_url, url.text, strlen(test_url));
    assert_int_equal(url.length, strlen(test_url));

    static_string_t tokenized = static_string_tokenize(&url, '/');
    assert_ptr_equal(tokenized.text, test_url + 1);
    assert_int_equal(tokenized.length, strlen(test_url) - 1);

    // repeated to see if the source is not modified
    tokenized = static_string_tokenize(&url, '/');
    assert_ptr_equal(tokenized.text, test_url + 1);
    assert_int_equal(tokenized.length, strlen(test_url) - 1);

    tokenized = static_string_tokenize(&tokenized, '/');
    assert_ptr_equal(tokenized.text, test_url + 15);
    assert_int_equal(tokenized.length, strlen(test_url) - 15);

    // no more '/' in the tokenized content
    static_string_t no_more_slashes = static_string_tokenize(&tokenized, '/');
    assert_ptr_equal(no_more_slashes.text, NULL);
    assert_int_equal(no_more_slashes.length, 0);

    static_string_t query_first_pair = static_string_tokenize(&tokenized, '?');
    assert_ptr_equal(query_first_pair.text, test_url + 22);
    assert_int_equal(query_first_pair.length, strlen(test_url) - 22);

    // get value of first pair
    static_string_t query_first_pair_value =
        static_string_tokenize(&query_first_pair, '=');
    assert_ptr_equal(query_first_pair_value.text, test_url + 24);
    assert_int_equal(query_first_pair_value.length, strlen(test_url) - 24);

    tokenized = static_string_tokenize(&query_first_pair, '&');
    assert_ptr_equal(tokenized.text, test_url + 26);
    assert_int_equal(tokenized.length, strlen(test_url) - 26);

    tokenized = static_string_tokenize(&tokenized, '&');
    assert_ptr_equal(tokenized.text, test_url + 30);
    assert_int_equal(tokenized.length, strlen(test_url) - 30);

    // end of string - no more stuff
    tokenized = static_string_tokenize(&tokenized, '&');
    assert_ptr_equal(tokenized.text, NULL);
    assert_int_equal(tokenized.length, 0);

    // no more '?' tokens - return an empty string
    static_string_t no_more_question_marks =
        static_string_tokenize(&tokenized, '?');
    assert_ptr_equal(no_more_question_marks.text, NULL);
    assert_int_equal(no_more_question_marks.length, 0);

    // tested again, if the source static-string is not modified
    assert_memory_equal(test_url, url.text, strlen(test_url));
    assert_int_equal(url.length, strlen(test_url));
}

static void test_snip_end(void** state) {
    const char* test_url = "/some-resource/detail?q=1&a=34&asdew5=a";
    static_string_t url = static_string_create(test_url);

    static_string_t query = static_string_tokenize(&url, '?');
    static_string_t first_pair = static_string_snip_end(&query, "&");
    assert_ptr_equal(first_pair.text, query.text);
    assert_int_equal(first_pair.length, 3);

    static_string_t second_pair = static_string_tokenize(&query, '&');
    second_pair = static_string_snip_end(&second_pair, "&");
    assert_ptr_equal(second_pair.text, query.text + 4);
    assert_int_equal(second_pair.length, 4);

    query = static_string_tokenize(&query, '&');
    static_string_t third_pair = static_string_tokenize(&query, '&');
    third_pair = static_string_snip_end(&third_pair, "&");
    assert_ptr_equal(third_pair.text, query.text + 5);
    assert_int_equal(third_pair.length, 8);
}

int main(int argc, char** argv) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_create_empty),
        cmocka_unit_test(test_create),
        cmocka_unit_test(test_tokenize),
        cmocka_unit_test(test_snip_end),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}