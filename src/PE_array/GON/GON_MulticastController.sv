/* verilator lint_off MULTITOP */
module GON_MulticastController #(
    parameter ID_SIZE = `XID_BITS
)(
    input clk,
    input rst,

    // config id
    input set_id,
    input [ID_SIZE - 1:0] id_in,
    output logic [ID_SIZE - 1:0] id,

    // tag
    input [ID_SIZE - 1:0] tag,

    input valid_in,
    output logic valid_out,
    input ready_in,
    output logic ready_out
);

    logic [ID_SIZE - 1:0] id_buf;
    logic id_equal;
 
    // ID buffer
    always_ff @(posedge clk) begin
        if (rst)
            id_buf <= 0;
        else if (set_id)
            id_buf <= id_in;
        else
            id_buf <= id_buf;
    end

    // comparator
    always_comb begin
        id_equal = (id_buf == tag);
    end

    // combinational logic
    always_comb begin
        ready_out = id_equal? ready_in : 1'b0;
        valid_out = (valid_in && ready_in && id_equal);
        id = id_buf;
    end

endmodule
