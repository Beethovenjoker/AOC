`include "define.svh"

module ReLU_Qint8 (
    input [7:0] relu_data_in,
    input relu_en,
    output logic[7:0] relu_data_out
);
    always_comb begin
        if (relu_en)
            relu_data_out = (relu_data_in < 8'd128) ? 8'd128 : relu_data_in;
        else
            relu_data_out = relu_data_in;
    end

endmodule