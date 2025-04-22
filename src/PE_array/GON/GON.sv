/* verilator lint_off MULTITOP */
`include "src/PE_array/GON/GON_Bus.sv"
`include "src/PE_array/GON/GON_MulticastController.sv"

module GON (
    input clk,
    input rst,

    /* Master GON <-> GLB */
    output logic GON_valid,
    input GON_ready,
    output logic [`DATA_BITS-1:0] GON_data,

    /* Controller <-> GON */
    input [`XID_BITS-1:0] tag_X,
    input [`YID_BITS-1:0] tag_Y,

    /* config */
    input set_XID,
    input [`XID_BITS - 1:0] XID_scan_in,
    input set_YID,
    input [`YID_BITS - 1:0] YID_scan_in,

    // Master PE <-> GON
    input [`NUMS_PE_ROW * `NUMS_PE_COL - 1:0] PE_valid,
    output logic [`NUMS_PE_ROW * `NUMS_PE_COL - 1:0] PE_ready,
    input [`DATA_BITS * `NUMS_PE_ROW * `NUMS_PE_COL - 1:0] PE_data

);

    logic [`YID_BITS-1:0] YID_scan_out;
    logic [`NUMS_PE_ROW - 1:0] X_BUS_READY;
    logic [`NUMS_PE_ROW - 1:0] Y_BUS_VALID;
    logic [`NUMS_PE_ROW * `DATA_BITS - 1:0] X_BUS_PE_DATA;
    logic [`XID_BITS - 1:0] X_BUS_ID_chain [0:`NUMS_PE_ROW];

    // Y-Bus
    GON_Bus #(
        .NUMS_MASTER(`NUMS_PE_COL),
        .ID_SIZE(`YID_BITS)
    ) GON_Y_BUS (
        .clk(clk),
        .rst(rst),

        // Master I/O
        .tag(tag_Y),
        .master_valid({2'b0,Y_BUS_VALID}),
        .master_data({64'b0,X_BUS_PE_DATA}),
        .master_ready(X_BUS_READY),

        
        // Slave I/O
        .slave_ready(GON_ready),
        .slave_valid(GON_valid),
        .slave_data(GON_data),

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
        for (i = 0; i < `NUMS_PE_ROW; i = i + 1) begin: GON_X_BUS
            GON_Bus #(
                .NUMS_MASTER(`NUMS_PE_COL),
                .ID_SIZE(`XID_BITS)
            ) GON_X_BUS (
                .clk(clk),
                .rst(rst),

                // Master I/O
                .tag(tag_X),
                .master_valid(PE_valid[(i+1)*`NUMS_PE_COL - 1:i*`NUMS_PE_COL]),
                .master_data(PE_data[(i+1)*`DATA_BITS*`NUMS_PE_COL - 1:i*`DATA_BITS*`NUMS_PE_COL]),
                .master_ready(PE_ready[(i+1)*`NUMS_PE_COL - 1:i*`NUMS_PE_COL]),

                // Slave I/O
                .slave_ready(X_BUS_READY[i]),
                .slave_valid(Y_BUS_VALID[i]),
                .slave_data(X_BUS_PE_DATA[(i+1) * `DATA_BITS - 1:i * `DATA_BITS]),

                // Config
               .set_id(set_XID),
               .ID_scan_in(X_BUS_ID_chain[i]),
               .ID_scan_out(X_BUS_ID_chain[i+1])
            );
        end
    endgenerate

endmodule
