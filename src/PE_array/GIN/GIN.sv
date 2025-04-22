/* verilator lint_off MULTITOP */
`include "src/PE_array/GIN/GIN_Bus.sv"
`include "src/PE_array/GIN/GIN_MulticastController.sv"

module GIN (
    input clk,
    input rst,

    // Slave SRAM <-> GIN
    input GIN_valid,
    output logic GIN_ready,
    input [`DATA_BITS - 1:0] GIN_data,

    /* Controller <-> GIN */
    input [`XID_BITS - 1:0] tag_X,
    input [`YID_BITS - 1:0] tag_Y,

    /* config */
    input set_XID,
    input [`XID_BITS - 1:0] XID_scan_in,
    input set_YID,
    input [`YID_BITS - 1:0] YID_scan_in,

    // Master GIN <-> PE
    input [`NUMS_PE_ROW * `NUMS_PE_COL - 1:0] PE_ready,
    output logic [`NUMS_PE_ROW * `NUMS_PE_COL - 1:0] PE_valid,
    output logic [`DATA_BITS - 1:0] PE_data
);

    logic [`YID_BITS-1:0] YID_scan_out;
    logic [`NUMS_PE_ROW-1:0] X_BUS_READY;
    logic [`NUMS_PE_ROW-1:0] Y_BUS_VALID;
    logic [`DATA_BITS - 1:0] X_BUS_PE_DATA;
    logic [`XID_BITS - 1:0] X_BUS_ID_chain [0:`NUMS_PE_ROW];
    
    // Y-Bus
    GIN_Bus #(
        .NUMS_SLAVE(`NUMS_PE_ROW),
        .ID_SIZE(`YID_BITS)
    ) GIN_Y_BUS (
        .clk(clk),
        .rst(rst),

        // Master I/O
        .tag(tag_Y),
        .master_valid(GIN_valid),
        .master_data(GIN_data),
        .master_ready(GIN_ready),

        // Slave I/O
        .slave_ready(X_BUS_READY),

        .slave_valid(Y_BUS_VALID),
        .slave_data(X_BUS_PE_DATA),

        // Config
        .set_id(set_YID),
        .ID_scan_in(YID_scan_in),
        .ID_scan_out(YID_scan_out)
    );

    // XID chain
    always_comb begin
        X_BUS_ID_chain[0] = XID_scan_in;
    end

    // X-Bus
    genvar i;

    generate
        for (i = 0; i < `NUMS_PE_ROW; i = i + 1) begin: GIN_X_BUS
            GIN_Bus #(
                .NUMS_SLAVE(`NUMS_PE_COL),
                .ID_SIZE(`XID_BITS)
            ) GIN_X_BUS (
                .clk(clk),
                .rst(rst),

                // Master I/O
                .tag(tag_X),
                .master_valid(Y_BUS_VALID[i]),
                .master_data(X_BUS_PE_DATA),
                .master_ready(X_BUS_READY[i]),

                // Slave I/O
                .slave_ready(PE_ready[(i+1)*`NUMS_PE_COL-1:i*`NUMS_PE_COL]),
                .slave_valid(PE_valid[(i+1)*`NUMS_PE_COL-1:i*`NUMS_PE_COL]),
                .slave_data(PE_data),

                // Config
               .set_id(set_XID),
               .ID_scan_in(X_BUS_ID_chain[i]),
               .ID_scan_out(X_BUS_ID_chain[i+1])
            );
        end
    endgenerate

endmodule
