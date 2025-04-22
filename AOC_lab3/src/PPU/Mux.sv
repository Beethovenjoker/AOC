`include "define.svh"

module MUX (
    input [7:0] data_A,
    input [7:0] data_B,
    input sel,
    output logic[7:0] mux_data_out
);
     always_comb begin
        mux_data_out = sel ? data_A : data_B;
    end

endmodule