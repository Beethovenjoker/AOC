#include <stdio.h>

#include "data.h"
#include "runtime.h"

uint8_t conv_relu_ans[1024];

int main() {
    int32_t i;
    int32_t err = 0;

    dla_init();

    printf(
        "[DLA test] qconv2d_relu\nInput shape (N=%d, C=%d, H=%d, W=%d)\nKernel "
        "shape (K=%d, C=%d, H=%d, W=%d)\n\n",
        1, 3, 8, 8, 16, 3, 3, 3);

    /* Run test funcion */
    /*  N -> C -> H -> W

        ofmap_len: 1024 = 1 * 8 * 8 * 16
        ifmap_len: 192 = 1 * 8 * 8 * 3
        filter_len: 432 = 16 * 3 * 3 * 3
    */

    qconv2d_relu(activation_flat_array, nchw_flat_array, conv_relu_ans, bias,
                 (8 * 8 * 16), (8 * 8 * 3), (16 * 3 * 3 * 3),
                 // mapping parameter
                 64, DEFAULT_e, DEFAULT_p, DEFAULT_q, DEFAULT_r, DEFAULT_t,
                 // shape parameter
                 1, 1, 3, 3, 3, 16, 8, 8,
                 // quantize scale
                 8);

    /* Verify */
    for (int i = 0; i < 1024; i++) {
        if (conv_relu_ans[i] != conv_relu_ans_golden[i]) {
            printf("Index %d is wrong: Correct answer: %d Your answer: %d", i,
                   conv_relu_ans_golden[i], conv_relu_ans[i]);
            printf("\n");
            err += 1;
        }
    }

    if (err == 0) {
        printf("Simulation all pass!\n");
    } else {
        printf("There are %d errors", err);
        printf("\n");
    }

    dla_final();
    return 0;
}
