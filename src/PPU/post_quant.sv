`include "define.svh"

module post_quant (
    input  logic [`DATA_BITS-1:0] post_quant_data_in,
    input  logic [5:0] scaling_factor,
    output logic [7:0] post_quant_data_out
);

    logic signed [`DATA_BITS-1:0] scaled_data;
    logic [`DATA_BITS-1:0] biased_data;

    always_comb begin
        // 1. Scale
        scaled_data = post_quant_data_in >>> $signed(scaling_factor);

        // 2. Add zero-point (XOR 128)
        biased_data = scaled_data ^ 32'b0000_0000_0000_0000_0000_0000_1000_0000;

        // 3. clamp
        post_quant_data_out = (| biased_data[30:8] ? 8'd128 : biased_data[7]  == 1'b0 ? 8'd255 : biased_data[7:0]);
    end

endmodule

