#include <stdio.h>
#include <x86intrin.h>  // _rdtsc()

#include "data.h"
#include "runtime.h"

uint8_t conv_relu_ans[32768], conv_relu_max_ans[8192];
uint8_t linear_ans[256], linear_relu_ans[256];
int32_t err = 0;

void verify_output(uint8_t *result, uint8_t *golden, int len,
                   const char *name) {
    for (int i = 0; i < len; i++) {
        if (result[i] != golden[i]) {
            printf("Error at %d: Expected = %d, Got = %d\n", i + 1, golden[i],
                   result[i]);
            err++;
        }
    }
    printf("%s: %s\n", name, err ? "FAIL" : "PASS");
}

int main() {
    setbuf(stdout, NULL);  // Immediate output

#ifdef CONV
    printf("Function: CONV\n");
    int cycle = qconv2d_relu_cpu(
        conv_activation, conv_weight, conv_relu_ans, conv_bias, conv_out_len,
        conv_in_len, conv_weight_len, 1, 1, 3, 3, conv_in_channel,
        conv_out_channel, conv_in_w, conv_in_h, conv_q_scale);
    verify_output(conv_relu_ans, conv_golden, conv_out_len, "CONV");

#elif defined(CONV_MAX)
    printf("Function: CONV_MAX\n");
    int cycle = qconv2d_relu_maxpool_cpu(
        conv_activation, conv_weight, conv_relu_max_ans, conv_bias,
        conv_max_out_len, conv_in_len, conv_weight_len, 1, 1, 3, 3,
        conv_in_channel, conv_out_channel, conv_in_w, conv_in_h, conv_q_scale);
    verify_output(conv_relu_max_ans, conv_max_golden, conv_max_out_len,
                  "CONV_MAX");

#elif defined(LINEAR)
    printf("Function: LINEAR\n");
    int cycle = qlinear_cpu(linear_activation, linear_weight, linear_ans,
                            linear_bias, linear_out_len, linear_in_len,
                            linear_weight_len, linear_q_scale);
    verify_output(linear_ans, linear_golden, linear_out_len, "LINEAR");

#elif defined(LINEAR_RELU)
    printf("Function: LINEAR_RELU\n");
    int cycle = qlinear_relu_cpu(
        linear_activation, linear_weight, linear_relu_ans, linear_bias,
        linear_out_len, linear_in_len, linear_weight_len, linear_q_scale);
    verify_output(linear_relu_ans, linear_relu_golden, linear_out_len,
                  "LINEAR_RELU");

#else
    int cycle = -1;
    printf("No function selected.\n");
#endif

    if (err == 0) printf("CYCLE: %d\n", cycle);
    return err;
}
