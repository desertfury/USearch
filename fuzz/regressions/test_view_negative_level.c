// Regression for fix/fuzz-view-negative-level.
//
// Replays the exact fuzz artifact that triggered the original
// heap-buffer-overflow in `reindex_keys_`. The crafted blob sets every
// `int16_t` level to -1; pre-fix, that promoted to a huge unsigned in
// `pre.neighbors_bytes * level`, wrapped back to a small per-node size,
// slipped past the `offsets[i] + size <= file_size` check, and produced
// a `nodes_[i].tape` 1 byte past the end of the mapped buffer. The fix
// rejects negative levels outright.
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "usearch.h"

// 612-byte payload taken verbatim from the fuzzer's crash artifact
// (with libFuzzer's 4-byte selector/metric/quant/dims prefix stripped).
static const unsigned char crash_payload[] = {
#include "view_negative_level_payload.inc"
};

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
        fprintf(stderr, "FAIL: usearch_init: %s\n", error ? error : "(null)");
        return 1;
    }

    error = NULL;
    usearch_view_buffer(index, (void*)crash_payload, sizeof(crash_payload), &error);
    // The bug under test is a crash. With the fix view_buffer either
    // sets an error string or succeeds without crashing; both are
    // acceptable.
    (void)error;

    usearch_free(index, &error);
    printf("PASS: view_negative_level regression\n");
    return 0;
}
