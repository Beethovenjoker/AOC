/* verilator lint_off MULTITOP */
module GON_Bus #(
    parameter NUMS_MASTER = `NUMS_PE_COL,
    parameter ID_SIZE = `XID_BITS
) (
    input clk,
    input rst,

    // Master I/O
    input [ID_SIZE - 1:0] tag,
    input [NUMS_MASTER - 1:0] master_valid,
    input [NUMS_MASTER * `DATA_BITS - 1:0] master_data,
    output logic [NUMS_MASTER - 1:0] master_ready,
    
    // Slave I/O
    output logic slave_valid,
    input slave_ready,
    output logic [`DATA_BITS - 1:0] slave_data,

    // Config
    input set_id,
    input [ID_SIZE - 1:0] ID_scan_in,
    output logic [ID_SIZE - 1 :0] ID_scan_out
 );

    logic [7:0] MC_valid;
    logic [ID_SIZE-1:0] ID_chain [NUMS_MASTER:0];

    // MC
    genvar i;

    generate
        for (i = 0; i < NUMS_MASTER; i = i + 1) begin: GON_BUS_MC
            GON_MulticastController #(
                .ID_SIZE(ID_SIZE)
            ) MC_GON (
                .clk(clk),
                .rst(rst),

                // config id
                .set_id(set_id),
                .id_in(ID_chain[i]),
                .id(ID_chain[i+1]),

                // tag
                .tag(tag),

                .valid_in(master_valid[i]),
                .valid_out(MC_valid[i]),
                .ready_in(slave_ready),
                .ready_out(master_ready[i])
            );
        end
    endgenerate
    
    always_comb begin
        slave_valid = | MC_valid;
        slave_data = (MC_valid[0]) ? master_data[ 0*`DATA_BITS +: `DATA_BITS] :
                     (MC_valid[1]) ? master_data[ 1*`DATA_BITS +: `DATA_BITS] :
                     (MC_valid[2]) ? master_data[ 2*`DATA_BITS +: `DATA_BITS] :
                     (MC_valid[3]) ? master_data[ 3*`DATA_BITS +: `DATA_BITS] :
                     (MC_valid[4]) ? master_data[ 4*`DATA_BITS +: `DATA_BITS] :
                     (MC_valid[5]) ? master_data[ 5*`DATA_BITS +: `DATA_BITS] :
                     (MC_valid[6]) ? master_data[ 6*`DATA_BITS +: `DATA_BITS] :
                     (MC_valid[7]) ? master_data[ 7*`DATA_BITS +: `DATA_BITS] : '0;
        ID_chain[0] = ID_scan_in;
        ID_scan_out = ID_chain[NUMS_MASTER];
    end

endmodule