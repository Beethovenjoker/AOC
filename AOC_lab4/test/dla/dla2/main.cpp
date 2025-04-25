#include <stdio.h>

#include "data.h"
#include "runtime.h"

uint8_t compute_output[12288];

int main() {
    int32_t i;
    int32_t err = 0;

    dla_init();

    printf(
        "[DLA test] qconv2d_relu_maxpool\nInput shape (N=%d, C=%d, H=%d, "
        "W=%d)\nKernel "
        "shape (K=%d, C=%d, H=%d, W=%d)\n\n",
        1, 64, 16, 16, 192, 64, 3, 3);

    /* Run test funcion */
    /*  N -> C -> H -> W

        ofmap_len: 12288 = 1 * 192 * 8 * 8
        ifmap_len: 16384 = 1 * 64 * 16 * 16
        filter_len: 110592 = 192 * 64 * 3 * 3
    */
    qconv2d_relu_maxpool(input_tensor, weight_tensor, compute_output,
                         bias_tensor, (192 * 8 * 8), (64 * 16 * 16), (192 * 64 * 3 * 3),
                         // mapping parameter
                         64, DEFAULT_e, DEFAULT_p, DEFAULT_q, DEFAULT_r,
                         DEFAULT_t,
                         // shape parameter
                         1, 1, 3, 3, 64, 192, 16, 16,
                         // quantize scale
                         7);

    /* Verify */
    printf("Start checking...\n");
    for (int i = 0; i < 12288; i++) {
        if ((i & 0xff) == 0) {
            printf("[Checking] %d/12288\n", i);
        }
        if (compute_output[i] != golden_tensor[i]) {
            printf("Index %d is wrong: Correct answer: %d Your answer: %d", i,
                   golden_tensor[i], compute_output[i]);
            printf("\n");
            err += 1;
        }
    }

    if (err == 0) {
        printf("Simulation all pass!\n");
    } else {
        printf("There are %d errors\n", err);
    }

    dla_final();
    return 0;
}
