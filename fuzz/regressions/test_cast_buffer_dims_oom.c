// Regression for fix/fuzz-cast-buffer-dims-oom.
//
// `index_dense_gt::load_from_stream` sized the cast buffer as
// `threads * metric_.bytes_per_vector()` straight from the attacker-supplied
// `head.dimensions`. A small crafted file made libFuzzer kill the process
// for a ~2 GB allocation. The fix rejects dimensions larger than the
// known input buffer.
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "usearch.h"

static void expect(int cond, char const* msg) {
    if (!cond) {
        fprintf(stderr, "FAIL: %s\n", msg);
        exit(1);
    }
}

int main(void) {
    unsigned char buf[8 + 64 + 40];
    memset(buf, 0, sizeof(buf));
    memcpy(buf + 8, "usearch", 7);
    uint16_t version_major = 2;
    memcpy(buf + 8 + 7, &version_major, sizeof(version_major));
    buf[8 + 7 + 6 + 1] = 1; // scalar f32
    buf[8 + 7 + 6 + 2] = 2; // key uint64
    buf[8 + 7 + 6 + 3] = 1; // slot uint32
    // dimensions = 1<<29 → bytes_per_vector ≈ 2GB
    uint64_t dims = (uint64_t)1 << 29;
    memcpy(buf + 8 + 7 + 6 + 4 + 8 + 8, &dims, sizeof(dims));

    uint64_t header[5] = {0};
    memcpy(buf + 72, header, sizeof(header));

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
    expect(index && !error, "usearch_init");

    error = NULL;
    usearch_load_buffer(index, buf, sizeof(buf), &error);
    expect(error != NULL, "load_buffer should reject huge dimensions");

    usearch_free(index, &error);
    printf("PASS: cast_buffer_dims_oom regression\n");
    return 0;
}
