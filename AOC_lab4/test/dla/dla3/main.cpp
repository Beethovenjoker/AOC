#include <stdio.h>

#include "runtime.h"

uint8_t input_tensor[16384];
int8_t weight_tensor[110592];
int32_t bias_tensor[192];
uint8_t cpu_output[12288];
uint8_t dla_output[12288];

int main() {
    int32_t i;
    int32_t err = 0;

    dla_init();

    printf(
        "[DLA test] qconv2d_relu_maxpool v.s. qconv2d_relu_maxpool_cpu\nInput "
        "shape (N=%d, C=%d, H=%d, "
        "W=%d)\nKernel "
        "shape (K=%d, C=%d, H=%d, W=%d)\n\n",
        1, 64, 16, 16, 192, 64, 3, 3);

    /* generate test data */
    for (int i = 0; i < 16384; i++) input_tensor[i] = i % 32 + 128;
    for (int i = 0; i < 110592; i++) weight_tensor[i] = i % 64 - 32;
    for (int i = 0; i < 192; i++) bias_tensor[i] = 10 * (i % 32) + 128;

    /* Run test funcion */
    /*  N -> C -> H -> W

        ofmap_len: 12288 = 1 * 192 * 8 * 8
        ifmap_len: 16384 = 1 * 64 * 16 * 16
        filter_len: 110592 = 192 * 64 * 3 * 3
    */
    qconv2d_relu_maxpool(input_tensor, weight_tensor, dla_output, bias_tensor,
                         (192 * 8 * 8), (64 * 16 * 16), (192 * 64 * 3 * 3),
                         // mapping parameter
                         64, DEFAULT_e, DEFAULT_p, DEFAULT_q, DEFAULT_r,
                         DEFAULT_t,
                         // shape parameter
                         1, 1, 3, 3, 64, 192, 16, 16,
                         // quantize scale
                         7);

    qconv2d_relu_maxpool_cpu(input_tensor, weight_tensor, cpu_output,
                             bias_tensor, (192 * 8 * 8), (64 * 16 * 16), (192 * 64 * 3 * 3),
                             // DEFAULT_t, shape parameter
                             1, 1, 3, 3, 64, 192, 16, 16,
                             // quantize scale
                             7);

    /* Verify */
    printf("Start checking...\n");
    for (int i = 0; i < 12288; i++) {
        if ((i & 0xff) == 0) {
            printf("[Checking] %d/12288\n", i);
        }
        if (cpu_output[i] != dla_output[i]) {
            printf(
                "Index %d is wrong: cpu_output answer: %d dla_output answer: "
                "%d",
                i, cpu_output[i], dla_output[i]);
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
