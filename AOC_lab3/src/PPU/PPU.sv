`include "src/PPU/post_quant.sv"
`include "src/PPU/Comparator_Qint8.sv"
`include "src/PPU/ReLU_Qint8.sv"
`include "src/PPU/Mux.sv"
`include "define.svh"

module PPU (
    input clk,
    input rst,
    input [`DATA_BITS-1:0] data_in,
    input [5:0] scaling_factor,
    input maxpool_en,
    input maxpool_init,
    input relu_sel,
    input relu_en,
    output logic[7:0] data_out
);

    post_quant pq (
        .post_quant_data_in(data_in),
        .scaling_factor(scaling_factor),
        .post_quant_data_out()
    );

    Comparator_Qint8 cq(
        .clk(clk),
        .rst(rst),
        .maxpool_en(maxpool_en),
        .maxpool_init(maxpool_init),
        .maxpool_data_in(pq.post_quant_data_out),
        .maxpool_data_out()
    );

    MUX m(
        .data_A(cq.maxpool_data_out),
        .data_B(pq.post_quant_data_out),
        .sel(relu_sel),
        .mux_data_out()
    );

    ReLU_Qint8 rq(
        .relu_data_in(m.mux_data_out),
        .relu_en(relu_en),
        .relu_data_out()
    );

    assign data_out = rq.relu_data_out;

endmodule