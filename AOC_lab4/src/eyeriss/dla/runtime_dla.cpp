#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "hardware_dla.h"
#include "runtime.h"

/*  //////////      NOTICE      //////////
    all parameter used to set DLA are send in by function argument
*/
static inline uint32_t align4(uint32_t x) 
{
    return (x + 3u) & ~3u;
}

static uint32_t adjust_m(uint32_t m_orig, uint32_t M, uint32_t E, uint32_t F, uint32_t ifmap_bytes, uint32_t filt_bytes)
{
    const uint32_t GLB_CAP  = 64 * 1024; // 64-KB on-chip GLB capacity
    const uint32_t BYTES_B  = 4;         // bias element size (int32)
    const uint32_t BYTES_P  = 4;         // psum / ofmap element size (int32)

    uint32_t m = m_orig;
    while (m > 0) {
        uint32_t total =
            ifmap_bytes +               // input feature-map tiles
            filt_bytes  +               // filter tiles
            m * BYTES_B +               // bias for m output channels
            m * E * F * BYTES_P;        // psum / ofmap for m channels

        if (total <= GLB_CAP)
            break;                      // fits in GLB, stop shrinking

        m >>= 1;                        // otherwise try the next lower power-of-two
    }

    if (m == 0)
        m = 1;                          // guard: at least one channel
    if (m > M)
        m = M;                          // never exceed the true output-channel count

    return m;
}

void dla_stop() {
    // set disable
    reg_write(DLA_ENABLE_OFFSET, 0);
}

void dla_init() {
#ifdef DLA_INFO
    fprintf(stdout, "DLA runtime info logging enabled.\n");
    dla_reset_runtime_info();
    create_dla_info_to_csv(DLA_INFO_CSV);
#endif
    hal_init();
}

void dla_final() {
#ifdef DLA_INFO
    fprintf(stdout, "Creating dla info file: %s\n", DLA_INFO_CSV);
#endif
    hal_final();
}

void dla_reset_runtime_info() { reset_runtime_info(); }

void create_dla_info_to_csv(const char *filename) {
    fprintf(stdout, "Creating dla info file: %s\n", filename);
    FILE *file = fopen(filename, "w");
    if (!file) {
        fprintf(stderr, "Create DLA info file failed.\n");
        return;
    }
    fprintf(file,
            "Operation,Cycles,Time(ns),Memory read,Memory "
            "write,m,e,p,q,r,t,PAD,U,R,S,C,M,W,H\n");
    fclose(file);
}

void dump_dla_info_to_csv(const char *filename, const char *operation_name,
                          // mapping parameter
                          uint32_t m, uint32_t e, uint32_t p, uint32_t q,
                          uint32_t r, uint32_t t,
                          // shape parameter
                          uint32_t PAD, uint32_t U, uint32_t R, uint32_t S,
                          uint32_t C, uint32_t M, uint32_t W, uint32_t H) {
    FILE *file = fopen(filename, "a");
    struct runtime_info info = get_runtime_info();
    fprintf(file, "%s,", operation_name);        // Operation
    fprintf(file, "%10d,", info.elapsed_cycle);  // Cycles
    fprintf(file, "%10d,", info.elapsed_time);   // Time (ns)
    fprintf(file, "%10d,", info.memory_read);    // Memory read
    fprintf(file, "%10d,", info.memory_write);   // Memory write
    fprintf(file, "%d,%d,%d,%d,%d,%d,", m, e, p, q, r, t);
    fprintf(file, "%d,%d,%d,%d,%d,%d,%d,%d\n", PAD, U, R, S, C, M, W, H);
    fclose(file);
}

int qconv2d_relu_maxpool(
    uint8_t *input_in_DRAM, int8_t *filter_in_DRAM, uint8_t *opsum_in_DRAM,
    int32_t *bias, uint32_t ofmap_len, uint32_t ifmap_len, uint32_t filter_len,
    // mapping parameter
    uint32_t m, uint32_t e, uint32_t p, uint32_t q, uint32_t r, uint32_t t,
    // shape parameter
    uint32_t PAD, uint32_t U, uint32_t R, uint32_t S, uint32_t C, uint32_t M,
    uint32_t W, uint32_t H,
    uint32_t scale) {  // int32_t scale_factor: merge ifmap and weight and ofmap
    // scale bit-shift

#ifdef DLA_INFO
    dla_reset_runtime_info();
#endif
    // Calculate m for GLB memory allocation
    /*! <<<========= Implement here =========>>>*/
    const uint32_t GLB_CAP = 64 * 1024;        // 64 KB
    const uint32_t BYTES_I = 1;                // ifmap / weight 8-bit
    const uint32_t BYTES_B = 4;                // bias 32-bit

    
    uint32_t F = (W + 2 * PAD - S) / U + 1;
    uint32_t E = (H + 2 * PAD - R) / U + 1;

    uint32_t C_pad      = align4(C);       /* filter channel 4-alignment */
    uint32_t ifmap_bytes= (q * r) * (e + R - 1) * (F + S - 1) * BYTES_I;
    uint32_t filt_bytes = (p * t) * (q * r) * R * S * C_pad * BYTES_I;

    m = adjust_m(m, M, E, F, ifmap_bytes, filt_bytes);
    
    /* layout offsets (bytes) */
    uint32_t filter_base = align4(ifmap_bytes);                 // start of weight
    uint32_t bias_base   = align4(filter_base + filt_bytes);    // start of bias
    uint32_t ofmap_base  = align4(bias_base + m * BYTES_B);     // start of psum/ofmap

    // call lower setting functions
    /*! <<<========= Implement here =========>>>*/
    // Set memory buffer address
    set_ifmap_addr(input_in_DRAM);
    set_filter_addr(filter_in_DRAM);
    set_bias_addr(bias);
    set_opsum_addr(opsum_in_DRAM);
    
    // Set memory buffer length
    set_input_activation_len(ifmap_len);
    set_output_activation_len(ofmap_len);
    
    // Set GLB address offset
    set_glb_filter_addr(filter_base);
    set_glb_bias_addr(bias_base);
    set_glb_ofmap_addr(ofmap_base);

    // Set DLA computation configuration
    set_mapping_param(m, e, p, q, r, t);
    set_shape_param1(PAD, U, R, S, C, M);
    set_shape_param2(W, H, PAD);

    // Enable DLA (with maxpool and relu)
    set_enable(scale, true, true, 0);

    wait_for_interrupt();
    dla_stop();
#ifdef DLA_INFO
    dump_dla_info_to_csv(DLA_INFO_CSV, "qconv2d_relu_maxpool", m, e, p, q, r, t,
                         PAD, U, R, S, C, M, W, H);
#endif
    return 0;
};

int qconv2d_relu(uint8_t *input_in_DRAM, int8_t *filter_in_DRAM,
                 uint8_t *opsum_in_DRAM, int32_t *bias, uint32_t ofmap_len,
                 uint32_t ifmap_len, uint32_t filter_len,
                 // mapping parameter
                 uint32_t m, uint32_t e, uint32_t p, uint32_t q, uint32_t r,
                 uint32_t t,
                 // shape parameter
                 uint32_t PAD, uint32_t U, uint32_t R, uint32_t S, uint32_t C,
                 uint32_t M, uint32_t W, uint32_t H,
                 uint32_t scale) {  // int32_t scale_factor: merge ifmap and
                                    // ofmap scale bit-shift
#ifdef DLA_INFO
    dla_reset_runtime_info();
#endif
    // Calculate m for GLB memory allocation
    /*! <<<========= Implement here =========>>>*/
    const uint32_t GLB_CAP = 64 * 1024;        // 64 KB
    const uint32_t BYTES_I = 1;                // ifmap / weight 8-bit
    const uint32_t BYTES_B = 4;                // bias 32-bit
    
    uint32_t F = (W + 2 * PAD - S) / U + 1;
    uint32_t E = (H + 2 * PAD - R) / U + 1;

    uint32_t C_pad      = align4(C);       /* filter channel 4-alignment */
    uint32_t ifmap_bytes= (q * r) * (E + R - 1) * (F + S - 1) * BYTES_I;
    uint32_t filt_bytes = (p * t) * (q * r) * R * S * C_pad * BYTES_I;

    m = adjust_m(m, M, E, F, ifmap_bytes, filt_bytes);
    
    /* layout offsets (bytes) */
    uint32_t filter_base = align4(ifmap_bytes);
    uint32_t bias_base   = align4(filter_base + filt_bytes);
    uint32_t ofmap_base  = align4(bias_base   + m * BYTES_B);

    // call lower setting functions
    /*! <<<========= Implement here =========>>>*/
    // Set memory buffer address
    set_ifmap_addr(input_in_DRAM);
    set_filter_addr(filter_in_DRAM);
    set_bias_addr(bias);
    set_opsum_addr(opsum_in_DRAM);
    
    // Set memory buffer length
    set_input_activation_len(ifmap_len);
    set_output_activation_len(ofmap_len);
    
    // Set GLB address offset
    set_glb_filter_addr(filter_base);
    set_glb_bias_addr(bias_base);
    set_glb_ofmap_addr(ofmap_base);
    
    // Set activation length
    set_input_activation_len(ifmap_len);
    set_output_activation_len(ofmap_len);
    
    // Set DLA computation configuration
    set_mapping_param(m, e, p, q, r, t);
    set_shape_param1(PAD, U, R, S, C, M);
    set_shape_param2(W, H, PAD);

    // Enable DLA (with maxpool and relu)
    set_enable(scale, false, true, 0);

    wait_for_interrupt();
    dla_stop();
#ifdef DLA_INFO
    dump_dla_info_to_csv(DLA_INFO_CSV, "qconv2d_relu", m, e, p, q, r, t, PAD, U,
                         R, S, C, M, W, H);
#endif
    return 0;
};
