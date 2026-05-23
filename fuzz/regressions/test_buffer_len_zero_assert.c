// Regression for fix/fuzz-buffer-len-zero-assert.
//
// Every `usearch_*_buffer` C entry asserted on `length != 0` (and
// `buffer != NULL`). In a release build that's a no-op and the call
// proceeds; in a debug/sanitizer build the assertion aborted (the
// fuzzer surfaced this as a deadly signal). The fix returns a clean
// error via the error pointer instead.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "usearch.h"

int main(void) {
    usearch_init_options_t opts;
    memset(&opts, 0, sizeof(opts));
    opts.metric_kind = usearch_metric_l2sq_k;
    opts.quantization = usearch_scalar_f32_k;
    opts.dimensions = 4;
    opts.connectivity = 8;
    opts.expansion_add = 16;
    opts.expansion_search = 16;

    usearch_error_t error = NULL;
    usearch_index_t index = usearch_init(&opts, &error);
    if (!index || error) {
        fprintf(stderr, "FAIL: init: %s\n", error ? error : "(null)");
        return 1;
    }

    // Every *_buffer call with length == 0 must return a clean error
    // (and not abort the process).
    error = NULL;
    usearch_view_buffer(index, NULL, 0, &error);
    if (!error) {
        fprintf(stderr, "FAIL: view_buffer(NULL, 0) didn't report error\n");
        return 1;
    }

    error = NULL;
    usearch_load_buffer(index, NULL, 0, &error);
    if (!error) {
        fprintf(stderr, "FAIL: load_buffer(NULL, 0) didn't report error\n");
        return 1;
    }

    usearch_init_options_t meta;
    memset(&meta, 0, sizeof(meta));
    error = NULL;
    usearch_metadata_buffer(NULL, 0, &meta, &error);
    if (!error) {
        fprintf(stderr, "FAIL: metadata_buffer(NULL, 0) didn't report error\n");
        return 1;
    }

    usearch_free(index, &error);
    printf("PASS: buffer_len_zero_assert regression\n");
    return 0;
}
