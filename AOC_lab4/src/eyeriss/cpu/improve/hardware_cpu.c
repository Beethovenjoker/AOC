#include "hardware_cpu.h"
#include <stdio.h>

#define O_BLK 8    /* number of output neurons per block   */
#define I_BLK 16   /* number of input elements per chunk   */

static inline uint8_t quant_clip(int32_t v, uint32_t scale)
{
    /* requantize : >>scale  + zero-point(128) */
    v = (v >> scale) + 128;

    /* clamp to 0‥255 */
    if (v <   0)
        return 0;
    if (v > 255)
        return 255;
    return (uint8_t)v;
}

static inline uint8_t quant_clamp(int32_t v, uint32_t scale)
{
    /* ReLU (negative → 0) */
    if (v < 0)
        v = 0;

    /* requantize : >>scale  + zero-point(128) */
    v = (v >> scale) + 128;

    /* clamp to 0‥255 */
    if (v <   0)
        return 0;
    if (v > 255)
        return 255;
    return (uint8_t)v;
}

void conv_maxpooling(uint32_t input_C, uint32_t input_H, uint32_t input_W,
                     uint8_t* activation, uint32_t filter_N, uint32_t filter_C,
                     uint32_t filter_H, uint32_t filter_W, int8_t* filter,
                     int32_t* bias, uint32_t padding, uint8_t* output,
                     uint32_t scale) {
    /*! <<<========= Implement here =========>>>*/
    const uint32_t out_H = input_H + 2 * padding - filter_H + 1;
    const uint32_t out_W = input_W + 2 * padding - filter_W + 1;
    const uint32_t pooled_H = out_H / 2;
    const uint32_t pooled_W = out_W / 2;
    const uint32_t input_HW = input_H * input_W;
    const uint32_t filter_HW = filter_H * filter_W;
    const uint32_t OC_KSTRIDE = filter_C * filter_HW;

    for (uint32_t oc = 0; oc < filter_N; ++oc) {
        const int8_t* __restrict w_oc_base = filter + oc * OC_KSTRIDE;
        int32_t bias_val = bias[oc];
        uint8_t* out_ptr = output + oc * pooled_H * pooled_W;

        for (uint32_t oh = 0; oh < pooled_H; ++oh) {
            for (uint32_t ow = 0; ow < pooled_W; ++ow) {
                int32_t max_val = INT32_MIN;

                for (uint32_t ph = 0; ph < 2; ++ph) {
                    for (uint32_t pw = 0; pw < 2; ++pw) {
                        int32_t out_y = oh * 2 + ph;
                        int32_t out_x = ow * 2 + pw;

                        int32_t ih_base = (int32_t)out_y - (int32_t)padding;
                        int32_t iw_base = (int32_t)out_x - (int32_t)padding;
                        int32_t acc = bias_val;

                        uint32_t ic = 0;
                        for (; ic + 1 < input_C; ic += 2) {
                            const uint8_t* act_base0 = activation + ic * input_HW;
                            const uint8_t* act_base1 = activation + (ic + 1) * input_HW;
                            const int8_t* w_ic_base0 = w_oc_base + ic * filter_HW;
                            const int8_t* w_ic_base1 = w_oc_base + (ic + 1) * filter_HW;

                            for (uint32_t kh = 0; kh < filter_H; ++kh) {
                                int32_t ih = ih_base + kh;
                                if ((uint32_t)ih >= input_H) continue;

                                const uint8_t* row0 = act_base0 + ih * input_W;
                                const uint8_t* row1 = act_base1 + ih * input_W;
                                const int8_t* w_row0 = w_ic_base0 + kh * filter_W;
                                const int8_t* w_row1 = w_ic_base1 + kh * filter_W;

                                uint32_t kw = 0;
                                for (; kw + 2 < filter_W; kw += 3) {
                                    int iw0 = iw_base + kw;
                                    int iw1 = iw0 + 1;
                                    int iw2 = iw0 + 2;

                                    if ((uint32_t)iw0 < input_W) {
                                        acc += ((int32_t)row0[iw0] - 128) * w_row0[kw];
                                        acc += ((int32_t)row1[iw0] - 128) * w_row1[kw];
                                    }
                                    if ((uint32_t)iw1 < input_W) {
                                        acc += ((int32_t)row0[iw1] - 128) * w_row0[kw + 1];
                                        acc += ((int32_t)row1[iw1] - 128) * w_row1[kw + 1];
                                    }
                                    if ((uint32_t)iw2 < input_W) {
                                        acc += ((int32_t)row0[iw2] - 128) * w_row0[kw + 2];
                                        acc += ((int32_t)row1[iw2] - 128) * w_row1[kw + 2];
                                    }
                                }

                                for (; kw < filter_W; ++kw) {
                                    int iw = iw_base + kw;
                                    if ((uint32_t)iw < input_W) {
                                        acc += ((int32_t)row0[iw] - 128) * w_row0[kw];
                                        acc += ((int32_t)row1[iw] - 128) * w_row1[kw];
                                    }
                                }
                            }
                        }

                        if (ic < input_C) {
                            const uint8_t* act_base = activation + ic * input_HW;
                            const int8_t* w_ic_base = w_oc_base + ic * filter_HW;

                            for (uint32_t kh = 0; kh < filter_H; ++kh) {
                                int32_t ih = ih_base + kh;
                                if ((uint32_t)ih >= input_H) continue;

                                const uint8_t* act_row = act_base + ih * input_W;
                                const int8_t* weight_row = w_ic_base + kh * filter_W;

                                for (uint32_t kw = 0; kw < filter_W; ++kw) {
                                    int32_t iw = iw_base + kw;
                                    if ((uint32_t)iw < input_W)
                                        acc += ((int32_t)act_row[iw] - 128) * weight_row[kw];
                                }
                            }
                        }

                        // ReLU
                        if (acc < 0)
                            acc = 0;
                        if (acc > max_val)
                            max_val = acc;
                    }
                }

                out_ptr[oh * pooled_W + ow] = quant_clip(max_val, scale);
            }
        }
    }
};

void conv(uint32_t input_C, uint32_t input_H, uint32_t input_W,
          uint8_t* activation, uint32_t filter_N, uint32_t filter_C,
          uint32_t filter_H, uint32_t filter_W, int8_t* filter, int32_t* bias,
          uint32_t padding, uint8_t* output, uint32_t scale) {
    /*! <<<========= Implement here =========>>>*/
    const uint32_t out_H = input_H + 2 * padding - filter_H + 1;
    const uint32_t out_W = input_W + 2 * padding - filter_W + 1;
    const uint32_t input_HW = input_H * input_W;
    const uint32_t filter_HW = filter_H * filter_W;
    const uint32_t OC_KSTRIDE = filter_C * filter_HW;

    for (uint32_t oc = 0; oc < filter_N; ++oc) {
        const int8_t* __restrict w_oc_base = filter + oc * OC_KSTRIDE;
        int32_t bias_val = bias[oc];

        for (uint32_t oh = 0; oh < out_H; ++oh) {
            int32_t ih_base = (int32_t)oh - (int32_t)padding;

            for (uint32_t ow = 0; ow < out_W; ++ow) {
                int32_t iw_base = (int32_t)ow - (int32_t)padding;
                int32_t acc = bias_val;

                uint32_t ic = 0;
                for (; ic + 1 < input_C; ic += 2) {
                    const uint8_t* __restrict act_base0 = activation + ic * input_HW;
                    const uint8_t* __restrict act_base1 = activation + (ic + 1) * input_HW;
                    const int8_t* __restrict w_ic_base0 = w_oc_base + ic * filter_HW;
                    const int8_t* __restrict w_ic_base1 = w_oc_base + (ic + 1) * filter_HW;

                    for (uint32_t kh = 0; kh < filter_H; ++kh) {
                        int32_t ih = ih_base + kh;
                        if ((uint32_t)ih >= input_H) continue;

                        const uint8_t* row0 = act_base0 + ih * input_W;
                        const uint8_t* row1 = act_base1 + ih * input_W;
                        const int8_t* w_row0 = w_ic_base0 + kh * filter_W;
                        const int8_t* w_row1 = w_ic_base1 + kh * filter_W;

                        uint32_t kw = 0;
                        for (; kw + 2 < filter_W; kw += 3) {
                            int iw0 = iw_base + kw;
                            int iw1 = iw0 + 1;
                            int iw2 = iw0 + 2;

                            if ((uint32_t)iw0 < input_W) {
                                acc += ((int32_t)row0[iw0] - 128) * (int32_t)w_row0[kw];
                                acc += ((int32_t)row1[iw0] - 128) * (int32_t)w_row1[kw];
                            }
                            if ((uint32_t)iw1 < input_W) {
                                acc += ((int32_t)row0[iw1] - 128) * (int32_t)w_row0[kw + 1];
                                acc += ((int32_t)row1[iw1] - 128) * (int32_t)w_row1[kw + 1];
                            }
                            if ((uint32_t)iw2 < input_W) {
                                acc += ((int32_t)row0[iw2] - 128) * (int32_t)w_row0[kw + 2];
                                acc += ((int32_t)row1[iw2] - 128) * (int32_t)w_row1[kw + 2];
                            }
                        }

                        for (; kw < filter_W; ++kw) {
                            int iw = iw_base + kw;
                            if ((uint32_t)iw < input_W) {
                                acc += ((int32_t)row0[iw] - 128) * (int32_t)w_row0[kw];
                                acc += ((int32_t)row1[iw] - 128) * (int32_t)w_row1[kw];
                            }
                        }
                    }
                }

                // handle tail
                if (ic < input_C) {
                    const uint8_t* __restrict act_base = activation + ic * input_HW;
                    const int8_t* __restrict w_ic_base = w_oc_base + ic * filter_HW;

                    for (uint32_t kh = 0; kh < filter_H; ++kh) {
                        int32_t ih = ih_base + kh;
                        if ((uint32_t)ih >= input_H) continue;

                        const uint8_t* act_row = act_base + ih * input_W;
                        const int8_t* weight_row = w_ic_base + kh * filter_W;

                        for (uint32_t kw = 0; kw < filter_W; ++kw) {
                            int32_t iw = iw_base + kw;
                            if ((uint32_t)iw < input_W) {
                                acc += ((int32_t)act_row[iw] - 128) * (int32_t)weight_row[kw];
                            }
                        }
                    }
                }

                output[(oc * out_H + oh) * out_W + ow] = quant_clamp(acc, scale);
            }
        }
    }
};

void linear_relu(uint32_t input_size, uint32_t output_size, uint8_t* activation,
                 uint8_t* output, int8_t* filter, int32_t* bias,
                 uint32_t scale) {
    /*! <<<========= Implement here =========>>>*/
        /* iterate over output neurons by blocks of O_BLK -------------------- */
    for (uint32_t o0 = 0; o0 < output_size; o0 += O_BLK) {

        uint32_t o_bound = (o0 + O_BLK <= output_size) ? O_BLK
                                                       : (output_size - o0);

        /* -------- initialise partial sums with bias ------------------- */
        int32_t acc[O_BLK];
        for (uint32_t o = 0; o < o_bound; ++o)
            acc[o] = bias[o0 + o];

        /* -------- sweep through input dimension ----------------------- */
        for (uint32_t i0 = 0; i0 < input_size; i0 += I_BLK) {

            uint32_t i_bound = (i0 + I_BLK <= input_size) ? I_BLK
                                                          : (input_size - i0);

            uint8_t *act_ptr = activation + i0;

            /* ----- inner micro-kernel : accumulate I_BLK activations -- */
            for (uint32_t i = 0; i < i_bound; ++i) {

                /* de-quant */
                int32_t a = (int32_t)act_ptr[i] - 128;

                /* weights are stored row-major : (output × input)        */
                int8_t *w_ptr = filter + (o0 * input_size) + (i + i0);

                /* manual unrolling for ILP; fall-through switch handles
                   variable tail size without extra branches             */
                switch (o_bound) {
                default: acc[7] += a * (int32_t)w_ptr[7 * input_size];
                case 7:  acc[6] += a * (int32_t)w_ptr[6 * input_size];
                case 6:  acc[5] += a * (int32_t)w_ptr[5 * input_size];
                case 5:  acc[4] += a * (int32_t)w_ptr[4 * input_size];
                case 4:  acc[3] += a * (int32_t)w_ptr[3 * input_size];
                case 3:  acc[2] += a * (int32_t)w_ptr[2 * input_size];
                case 2:  acc[1] += a * (int32_t)w_ptr[1 * input_size];
                case 1:  acc[0] += a * (int32_t)w_ptr[0 * input_size];
                case 0:  break;
                }
            }
        }

        /* -------- ReLU + requantise + store --------------------------- */
        for (uint32_t o = 0; o < o_bound; ++o)
            output[o0 + o] = quant_clamp(acc[o], scale);
    }
};

void linear(uint32_t input_size, uint32_t output_size, uint8_t* activation,
            uint8_t* output, int8_t* filter, int32_t* bias, uint32_t scale) {
    /*! <<<========= Implement here =========>>>*/
    /* iterate over output dimension in blocks of O_BLK */
    for (uint32_t o0 = 0; o0 < output_size; o0 += O_BLK) {

        uint32_t o_bound = (o0 + O_BLK <= output_size) ? O_BLK : (output_size - o0);

        /* --- initialise partial sums with bias ---------------------- */
        int32_t acc[O_BLK] = {0};
        for (uint32_t o = 0; o < o_bound; ++o)
            acc[o] = bias[o0 + o];

        /* --- sweep through input dimension -------------------------- */
        for (uint32_t i0 = 0; i0 < input_size; i0 += I_BLK) {

            uint32_t i_bound = (i0 + I_BLK <= input_size) ? I_BLK : (input_size - i0);

            /* activations */
            uint8_t *act_ptr = activation + i0;

            /* inner micro-kernel: accumulate I_BLK activations */
            for (uint32_t i = 0; i < i_bound; ++i) {

                /* de-quant */
                int32_t a = (int32_t)act_ptr[i] - 128;

                /* weights are stored row-major : (output × input)        */
                int8_t *w_ptr = filter + (o0 * input_size) + (i + i0);

                /* manual unrolling for ILP; fall-through switch handles
                   variable tail size without extra branches             */
                switch (o_bound) {
                default: acc[7] += a * (int32_t)w_ptr[7 * input_size];
                case 7:  acc[6] += a * (int32_t)w_ptr[6 * input_size];
                case 6:  acc[5] += a * (int32_t)w_ptr[5 * input_size];
                case 5:  acc[4] += a * (int32_t)w_ptr[4 * input_size];
                case 4:  acc[3] += a * (int32_t)w_ptr[3 * input_size];
                case 3:  acc[2] += a * (int32_t)w_ptr[2 * input_size];
                case 2:  acc[1] += a * (int32_t)w_ptr[1 * input_size];
                case 1:  acc[0] += a * (int32_t)w_ptr[0 * input_size];
                case 0:  break;
                }
            }
        }

        /* --- requantise & store results ----------------------------- */
        for (uint32_t o = 0; o < o_bound; ++o)
            output[o0 + o] = quant_clip(acc[o], scale);
    }
};

void quantize(float* input_in_DRAM, uint8_t* output_in_DRAM, uint32_t size,
              uint32_t scale) {
    float fp_scale = 1;
    for (uint32_t i = 0; i < scale; i++) {
        fp_scale *= 2;
    }
    for (uint32_t i = 0; i < size; i++) {
        float t = input_in_DRAM[i] * fp_scale;
        int32_t temp = (int32_t)t + 128;
        // clamp to 0 ~ 255
        if (temp < 0) {
            output_in_DRAM[i] = 0;
        } else if (temp > 255)
            output_in_DRAM[i] = 255;
        else
            output_in_DRAM[i] = (uint8_t)temp;
    }
};

void dequantize(uint8_t* input_in_DRAM, float* output_in_DRAM, uint32_t size,
                uint32_t scale) {
    float fp_scale = 1;
    for (uint32_t i = 0; i < scale; i++) {
        fp_scale *= 2;
    }
    for (uint32_t i = 0; i < size; i++) {
        float temp = *(input_in_DRAM + i) - 128;
        *(output_in_DRAM + i) = temp / fp_scale;
    }
};