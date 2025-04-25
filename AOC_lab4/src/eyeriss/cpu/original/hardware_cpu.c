#include "hardware_cpu.h"

static inline uint32_t align4(uint32_t x)
{
    return (x + 3u) & ~3u;
}

static inline uint8_t quant_clip(int32_t x, uint32_t s)
{
    int32_t y = (x >> s) + 128;      /* add zero-point */
    if (y <   0)
        return 0;
    if (y > 255)
        return 255;
    return (uint8_t)y;
}

void conv_maxpooling(uint32_t input_C, uint32_t input_H, uint32_t input_W,
                     uint8_t* activation, uint32_t filter_N, uint32_t filter_C,
                     uint32_t filter_H, uint32_t filter_W, int8_t* filter,
                     int32_t* bias, uint32_t padding, uint8_t* output,
                     uint32_t scale) {

    /*! <<<========= Implement here =========>>>*/
    /* ------------ (1) calculate output size of conv --------------- */
    uint32_t E = input_H + 2 * padding - filter_H + 1;   /* out-H */
    uint32_t F = input_W + 2 * padding - filter_W + 1;   /* out-W */

    /* conv result buffer, used for 2×2 / stride2 max-pool */
    uint32_t conv_area = filter_N * E * F;
    int32_t* conv_buf = (int32_t*)malloc(sizeof(int32_t) * conv_area);

    for (uint32_t k = 0; k < filter_N; ++k)                /* out-channel */
        for (uint32_t oh = 0; oh < E; ++oh)                /* out-H        */
            for (uint32_t ow = 0; ow < F; ++ow) {          /* out-W        */
                int32_t acc = bias[k];
                for (uint32_t c = 0; c < filter_C; ++c)          /* in-C */
                    for (uint32_t r = 0; r < filter_H; ++r)      /* ker-H */
                        for (uint32_t s = 0; s < filter_W; ++s){ /* ker-W */
                            int32_t ih = (int32_t)oh + (int32_t)r - (int32_t)padding;
                            int32_t iw = (int32_t)ow + (int32_t)s - (int32_t)padding;
                            if (ih < 0 || ih >= (int32_t)input_H ||
                                iw < 0 || iw >= (int32_t)input_W)
                                continue;
                            uint8_t a  = activation[(c*input_H + ih)*input_W + iw];
                            int8_t  w  = filter[(((k*filter_C)+c)*filter_H + r)*filter_W + s];
                            acc += ((int32_t)a - 128) * (int32_t)w;
                        }
                if (acc < 0) acc = 0;                     /* ReLU */
                conv_buf[(k*E + oh)*F + ow] = acc;        /* save int32 for pooling */
            }
        /* ------------ (3) 2×2 / stride 2 max-pool & quantized -------------- */
        uint32_t P = E / 2;           /* pooled H */
        uint32_t Q = F / 2;           /* pooled W */
    
    for (uint32_t k = 0; k < filter_N; ++k)
        for (uint32_t ph = 0; ph < P; ++ph)
            for (uint32_t pw = 0; pw < Q; ++pw) {
                /* find 2×2 maximun */
                int32_t m0 = conv_buf[(k*E + (ph*2  ))*F + (pw*2  )];
                int32_t m1 = conv_buf[(k*E + (ph*2  ))*F + (pw*2+1)];
                int32_t m2 = conv_buf[(k*E + (ph*2+1))*F + (pw*2  )];
                int32_t m3 = conv_buf[(k*E + (ph*2+1))*F + (pw*2+1)];
                int32_t m  = m0;
                if (m1 > m)
                    m = m1;
                if (m2 > m)
                    m = m2;
                if (m3 > m)
                    m = m3;
                output[(k*P + ph)*Q + pw] = quant_clip(m, scale);
            }
    
     free(conv_buf);
};

void conv(uint32_t input_C, uint32_t input_H, uint32_t input_W,
          uint8_t* activation, uint32_t filter_N, uint32_t filter_C,
          uint32_t filter_H, uint32_t filter_W, int8_t* filter, int32_t* bias,
          uint32_t padding, uint8_t* output, uint32_t scale) {

    /*! <<<========= Implement here =========>>>*/
    uint32_t E = input_H + 2 * padding - filter_H + 1;
    uint32_t F = input_W + 2 * padding - filter_W + 1;

    for (uint32_t k = 0; k < filter_N; ++k)
        for (uint32_t oh = 0; oh < E; ++oh)
            for (uint32_t ow = 0; ow < F; ++ow) {
                int32_t acc = bias[k];
                for (uint32_t c = 0; c < filter_C; ++c)
                    for (uint32_t r = 0; r < filter_H; ++r)
                        for (uint32_t s = 0; s < filter_W; ++s) {
                            int32_t ih = (int32_t)oh + (int32_t)r - (int32_t)padding;
                            int32_t iw = (int32_t)ow + (int32_t)s - (int32_t)padding;
                            if (ih < 0 || ih >= (int32_t)input_H ||
                                iw < 0 || iw >= (int32_t)input_W)
                                continue;
                            uint8_t a = activation[(c*input_H + ih)*input_W + iw];
                            int8_t  w = filter[(((k*filter_C)+c)*filter_H + r)*filter_W + s];
                            acc += ((int32_t)a - 128) * (int32_t)w;
                        }
                if (acc < 0)
                    acc = 0;
                output[(k*E + oh)*F + ow] = quant_clip(acc, scale);
            }
};

void linear_relu(uint32_t input_size, uint32_t output_size, uint8_t* activation,
                 uint8_t* output, int8_t* filter, int32_t* bias,
                 uint32_t scale) {
    /*! <<<========= Implement here =========>>>*/
    for (uint32_t o = 0; o < output_size; ++o) {
        int32_t acc = bias[o];
        for (uint32_t i = 0; i < input_size; ++i) {
            uint8_t  a = activation[i];
            int8_t   w = filter[o*input_size + i];
            acc += ((int32_t)a - 128) * (int32_t)w;
        }
        if (acc < 0)
            acc = 0;                   /* ReLU */
        output[o] = quant_clip(acc, scale);
    }
};

void linear(uint32_t input_size, uint32_t output_size, uint8_t* activation,
            uint8_t* output, int8_t* filter, int32_t* bias, uint32_t scale) {
    /*! <<<========= Implement here =========>>>*/
    for (uint32_t o = 0; o < output_size; ++o) {
        int32_t acc = bias[o];
        for (uint32_t i = 0; i < input_size; ++i) {
            uint8_t a = activation[i];
            int8_t  w = filter[o*input_size + i];
            acc += ((int32_t)a - 128) * (int32_t)w;
        }
        output[o] = quant_clip(acc, scale);
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
