 /* verilator lint_off MULTITOP */
module GIN_Bus #(
    parameter NUMS_SLAVE = `NUMS_PE_COL,
    parameter ID_SIZE = `XID_BITS
) (
    input clk,
    input rst,

   // Master I/O
    input [ID_SIZE-1:0] tag,
    input master_valid,
    input [`DATA_BITS-1:0] master_data,
    output logic master_ready,

   // Slave I/O
    input [NUMS_SLAVE-1:0] slave_ready,
    output logic [NUMS_SLAVE-1:0] slave_valid,
    output logic [`DATA_BITS-1:0] slave_data,

    // Config
    input set_id,
    input [ID_SIZE-1:0] ID_scan_in,
    output logic [ID_SIZE-1:0] ID_scan_out
 );

    logic [NUMS_SLAVE-1:0] MC_ready;
    logic [ID_SIZE-1:0] ID_chain [NUMS_SLAVE:0];
    // MC
    genvar i;

    generate
        for (i = 0; i < NUMS_SLAVE; i = i + 1) begin: GIN_BUS_MC
            GIN_MulticastController #(
                .ID_SIZE(ID_SIZE)
            ) MC_GIN (
                .clk(clk),
                .rst(rst),

                // config id
                .set_id(set_id),
                .id_in(ID_chain[i]),
                .id(ID_chain[i+1]),

                // tag
                .tag(tag),

                .valid_in(master_valid),
                .valid_out(slave_valid[i]),
                .ready_in(slave_ready[i]),
                .ready_out(MC_ready[i])
            );
        end
    endgenerate

    always_comb begin
        master_ready = | MC_ready;
        slave_data = master_data;
        ID_chain [0] = ID_scan_in;
        ID_scan_out = ID_chain[NUMS_SLAVE];
    end

endmodule
