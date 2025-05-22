`include "define.svh"

module Comparator_Qint8 (
    input clk,
    input rst,
    input maxpool_en,
    input maxpool_init,
    input [7:0] maxpool_data_in,
    output logic[7:0] maxpool_data_out
);

    // counter
    logic [7:0] max;
    logic [1:0] counter;

    // current state
    logic [1:0] cur_state;
    logic [1:0] next_state;

    // state
    parameter [1:0] IDLE                = 3'b00;
    parameter [1:0] LOAD_DATA           = 3'b01;
    parameter [1:0] OUTPUT              = 3'b10;

    // state register
    always_ff @(posedge clk) begin
        if (rst)
            cur_state <= IDLE;
        else
            cur_state <= next_state;
    end

    // next state logic
    always_comb begin
        case (cur_state)
            IDLE: begin
                if (maxpool_en)
                    next_state = LOAD_DATA;
                else
                    next_state = IDLE;
            end

            LOAD_DATA: begin
                if (counter == 2'b10)
                    next_state = OUTPUT;
                else
                    next_state = LOAD_DATA;
            end

            OUTPUT: begin
                next_state = IDLE;
            end

            default: next_state = IDLE;
        endcase
    end

    // counter
    always_ff @(posedge clk) begin
        if (rst)
            counter <= 0;
        else if (cur_state == LOAD_DATA)
            counter <= counter + 1;
        else if (cur_state == IDLE)
            counter <= 0;
        else
            counter <= counter;
    end

    // max
    always_ff @(posedge clk) begin
        if (rst) begin
            max <= 0;
        end else if (cur_state == OUTPUT) begin
            max <= 0;
        end else if (cur_state == IDLE || cur_state == LOAD_DATA) begin
            if (maxpool_data_in > max)
                max <= maxpool_data_in;
            else
                max <= max;
        end else begin
            max <= max;
        end
    end

    // output
    always_comb begin
        maxpool_data_out = max;
    end

endmodule
