`include "define.svh"

module PE (
    input clk,
    input rst,
    input PE_en,
    input [`CONFIG_SIZE-1:0] i_config,

    input [`DATA_BITS-1:0] ifmap,
    input [`DATA_BITS-1:0] filter,
    input [`DATA_BITS-1:0] ipsum,

    input ifmap_valid,
    input filter_valid,
    input ipsum_valid,
    input opsum_ready,

    output logic [`DATA_BITS-1:0] opsum,
    output logic ifmap_ready,
    output logic filter_ready,
    output logic ipsum_ready,
    output logic opsum_valid
);

    //================================================================
    //  Parameters
    //================================================================

    // cofig decode
    logic mode;
    logic [2:0] p;
    logic [4:0] f;
    logic [2:0] q;

    // spad
    logic [`IFMAP_INDEX_BIT-1:0]  ifmap_index;
    logic [`FILTER_INDEX_BIT-1:0] filter_index;
    logic [`OFMAP_INDEX_BIT-1:0]  ipsum_index;
    logic [`IFMAP_SIZE-1:0]  ifmap_spad  [`IFMAP_SPAD_LEN-1:0];
    logic [`FILTER_SIZE-1:0] filter_spad [`FILTER_SPAD_LEN-1:0];
    logic [`PSUM_SIZE-1:0]   ipsum_spad  [`OFMAP_SPAD_LEN-1:0];

    // counter
    logic [1:0] ifmap_counter;  // count to 3
    logic [3:0] sliding_counter; // count to 1
    logic [3:0] sliding_load_counter; // count to 1
    logic [3:0] filter_counter; // count to 12
    logic [2:0] ipsum_counter;  // count to 3
    logic [2:0] output_counter;
    logic [`IFMAP_INDEX_BIT-1:0]  compute_ifmap_counter;
    logic [`FILTER_INDEX_BIT-1:0] compute_filter_counter;
    logic [`OFMAP_INDEX_BIT-1:0]  compute_ipsum_counter;

    // sliding
    logic sliding_on;

    //================================================================
    //  Decoding
    //================================================================

    // config decoding
    always_ff @(posedge clk) begin
        if (rst) begin
            mode <= 1'd0;
            p    <= 3'd0;
            f    <= 5'd0;
            q    <= 3'd0;
        end else if (PE_en) begin
            mode <= i_config[9];
            p    <= {1'b0,i_config[8:7]} + 3'd1;
            f    <= i_config[6:2];
            q    <= {1'b0,i_config[1:0]} + 3'd1;
        end else begin
            mode <= mode;
            p    <= p;
            f    <= f;
            q    <= q;
        end
    end

    //================================================================
    //  FSM
    //================================================================

    // current state
    logic [3:0] cur_state;
    logic [3:0] next_state;

    // state
    parameter [3:0] IDLE                = 4'b0000;
    parameter [3:0] LOAD_IDLE           = 4'b0001;
    parameter [3:0] LOAD_FILT           = 4'b0010;
    parameter [3:0] LOAD_IFMAP          = 4'b0011;
    parameter [3:0] SLIDING             = 4'b0100;
    parameter [3:0] SLIDING_LOAD        = 4'b0101;
    parameter [3:0] LOAD_IPSUM          = 4'b0110;
    parameter [3:0] COMPUTE             = 4'b0111;
    parameter [3:0] SWITCH_FILTER_IPSUM = 4'b1000;
    parameter [3:0] SWITCH_IFMAP        = 4'b1001;
    parameter [3:0] OUTPUT              = 4'b1010;
    
    // changing state
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
                if (PE_en)
                    next_state = LOAD_FILT;
                else
                    next_state = IDLE;
            end

            LOAD_IDLE: begin
                if (sliding_on)
                    next_state = SLIDING;
                else
                    next_state = LOAD_IDLE;
            end

            LOAD_FILT: begin
                if (filter_counter == 3*p)
                    next_state = LOAD_IFMAP;
                else
                    next_state = LOAD_FILT;
            end

            LOAD_IFMAP: begin
                if (ifmap_counter == 3)
                    next_state = COMPUTE;
                else
                    next_state = LOAD_IFMAP;
            end

            SLIDING: begin
                if (sliding_counter == 1)
                    next_state = SLIDING_LOAD;
                else
                    next_state = SLIDING;
            end

            SLIDING_LOAD: begin
                if (sliding_load_counter == 1)
                    next_state = COMPUTE;
                else
                    next_state = SLIDING_LOAD;
            end

            COMPUTE: begin
                if (compute_filter_counter != 0 && (compute_filter_counter + 1) % (3*p*q) == 0)
                    next_state = LOAD_IPSUM;
                else if (compute_filter_counter != 0 && (compute_filter_counter + 1) % (3*p) == 0)
                    next_state = SWITCH_IFMAP;
                else if (compute_filter_counter != 0 && (compute_filter_counter + 1) % 3 == 0)
                    next_state = SWITCH_FILTER_IPSUM;
                else
                    next_state = COMPUTE;
                end

            SWITCH_FILTER_IPSUM: begin
                next_state = COMPUTE;
            end

            SWITCH_IFMAP: begin
                next_state = COMPUTE;
            end

            LOAD_IPSUM: begin
                if (ipsum_counter == 4)
                    next_state = OUTPUT;
                else
                    next_state = LOAD_IPSUM;
            end

            OUTPUT: begin
                if (output_counter == 4) begin
                    next_state = LOAD_IDLE;
                end else begin
                    next_state = OUTPUT;
                end
            end

            default: next_state = IDLE;
        endcase
    end

    //================================================================
    //  Load Counter
    //================================================================

    // load counter
    always_comb begin
        filter_index = {2'b0, filter_counter};;
        ifmap_index = {2'b0, ifmap_counter};
        ipsum_index = ipsum_counter[1:0];
    end

    // filter counter
    always_ff @(posedge clk) begin
        if (rst)
            filter_counter <= 0;
        else if (filter_valid && filter_ready)
            filter_counter <= filter_counter + 1;
        else if (cur_state == LOAD_IDLE)
            filter_counter <= 0;
        else
            filter_counter <= filter_counter;
    end

    // ifmap counter
    always_ff @(posedge clk) begin
        if (rst)
            ifmap_counter <= 0;
        else if (ifmap_valid && ifmap_ready)
            ifmap_counter <= ifmap_counter + 1;
        else if (cur_state == LOAD_IDLE)
            ifmap_counter <= 0;
        else
            ifmap_counter <= ifmap_counter;
    end

    // ipsum counter
    always_ff @(posedge clk) begin
        if (rst)
            ipsum_counter <= 0;
        else if (ipsum_valid && ipsum_ready)
            ipsum_counter <= ipsum_counter + 1;
        else if (cur_state == LOAD_IDLE)
            ipsum_counter <= 0;
        else
            ipsum_counter <= ipsum_counter;
    end

    // sliding counter
    always_ff @(posedge clk) begin
        if (rst)
            sliding_counter <= 0;
        else if (cur_state == SLIDING)
            sliding_counter <= sliding_counter + 1;
        else if (cur_state == LOAD_IDLE)
            sliding_counter <= 0;
        else
            sliding_counter <= sliding_counter;
    end

    // sliding load counter
    always_ff @(posedge clk) begin
        if (rst)
            sliding_load_counter <= 0;
        else if (cur_state == SLIDING_LOAD && ifmap_valid && ifmap_ready)
            sliding_load_counter <= sliding_load_counter + 1;
        else if (cur_state == LOAD_IDLE)
            sliding_load_counter <= 0;
        else
            sliding_load_counter <= sliding_load_counter;
    end

    //================================================================
    //  Compute Counter
    //================================================================

    // compute filter counter
    always_ff @(posedge clk) begin
        if (rst)
           compute_filter_counter <= 0;
        else if (cur_state == COMPUTE)
           compute_filter_counter <= compute_filter_counter + 1;
        else if (cur_state == LOAD_IDLE)
           compute_filter_counter <= 0;
        else
           compute_filter_counter <= compute_filter_counter;
    end
    // compute ifmap counter
    always_ff @(posedge clk) begin
        if (rst)
            compute_ifmap_counter <= 0;
        else if (cur_state == COMPUTE)
            compute_ifmap_counter <= compute_ifmap_counter + 1;
        else if (cur_state == SWITCH_FILTER_IPSUM)
            compute_ifmap_counter <= compute_ifmap_counter - 3;
        else if (cur_state == LOAD_IDLE)
            compute_ifmap_counter <= 0;
        else
            compute_ifmap_counter <= compute_ifmap_counter;
    end

    // compute ipsum counter
    always_ff @(posedge clk) begin
        if (rst)
           compute_ipsum_counter <= 0;
        else if (cur_state == SWITCH_FILTER_IPSUM)
           compute_ipsum_counter <= compute_ipsum_counter + 1;
        else if (cur_state == SWITCH_IFMAP)
           compute_ipsum_counter <= 0;
        else if (cur_state == LOAD_IDLE)
            compute_ipsum_counter <= 0;
        else
           compute_ipsum_counter <= compute_ipsum_counter;
    end

    //================================================================
    //  Calculate
    //================================================================

    // load filter
    always_ff @(posedge clk) begin
        if (rst) begin
            for (int i = 0 ; i < 48 ; i++)                         
                filter_spad[i]  <= 8'd0;
        end else if (filter_valid && filter_ready) begin
            filter_spad[filter_index] <= filter[7:0];
            filter_spad[filter_index + 12] <= filter[15:8];
            filter_spad[filter_index + 24] <= filter[23:16];
            filter_spad[filter_index + 36] <= filter[31:24];
        end else begin
            filter_spad <= filter_spad;
        end   
    end

    // load ifmap
    always_ff @(posedge clk) begin
        if (rst)begin
            // clear ifmap_spad
            for (int i = 0 ; i < 12 ; i++)                         
                ifmap_spad[i]  <= 8'd0;
        end else if (cur_state == SLIDING) begin
            ifmap_spad[sliding_counter]     <= ifmap_spad[sliding_counter + 1];
            ifmap_spad[sliding_counter + 3] <= ifmap_spad[sliding_counter + 4];
            ifmap_spad[sliding_counter + 6] <= ifmap_spad[sliding_counter + 7];
            ifmap_spad[sliding_counter + 9] <= ifmap_spad[sliding_counter + 10];
        end else if (cur_state == SLIDING_LOAD && ifmap_valid && ifmap_ready) begin
            ifmap_spad[sliding_load_counter + 2]  <= ifmap[7:0] ^ 8'b10000000;
            ifmap_spad[sliding_load_counter + 5]  <= ifmap[15:8] ^ 8'b10000000;
            ifmap_spad[sliding_load_counter + 8]  <= ifmap[23:16] ^ 8'b10000000;
            ifmap_spad[sliding_load_counter + 11] <= ifmap[31:24] ^ 8'b10000000;
        end else if (ifmap_valid && ifmap_ready) begin
            ifmap_spad[ifmap_index]     <= ifmap[7:0] ^ 8'b10000000;
            ifmap_spad[ifmap_index + 3] <= ifmap[15:8] ^ 8'b10000000;
            ifmap_spad[ifmap_index + 6] <= ifmap[23:16] ^ 8'b10000000;
            ifmap_spad[ifmap_index + 9] <= ifmap[31:24] ^ 8'b10000000;
        end else begin
            ifmap_spad <= ifmap_spad;
        end
    end

    // ipsum compute
    always_ff @(posedge clk) begin
        if (rst) begin
            for (int i = 0 ; i < 4 ; i++)                               
                ipsum_spad[i] <= 32'd0;
        end else if (cur_state == COMPUTE) begin
            ipsum_spad[compute_ipsum_counter] <= $signed(ipsum_spad[compute_ipsum_counter]) + ($signed(ifmap_spad[compute_ifmap_counter]) * $signed(filter_spad[compute_filter_counter]));
        end else if (cur_state == LOAD_IPSUM && ipsum_valid && ipsum_ready) begin
            ipsum_spad[ipsum_index] <= ipsum_spad[ipsum_index] + ipsum;
        end else if (cur_state == SLIDING) begin
            for (int i = 0 ; i < 4 ; i++)                               
                ipsum_spad[i] <= 32'd0;
        end else begin
            ipsum_spad <= ipsum_spad;
        end
    end

    //================================================================
    //  Control Signals
    //================================================================

    // sliding on
    always_ff @(posedge clk) begin
        if (rst)
            sliding_on <= 0;
        else if (cur_state == OUTPUT)
            sliding_on <= 1;
        else if (cur_state == SLIDING_LOAD)
            sliding_on <= 0;
        else
            sliding_on <= sliding_on;
    end

    // hand shaking
    always_comb begin
        ifmap_ready  = (cur_state == LOAD_IFMAP) || (cur_state == SLIDING_LOAD);
        filter_ready = (cur_state == LOAD_FILT);
        ipsum_ready  = (cur_state == LOAD_IPSUM);
        opsum_valid  = (cur_state == OUTPUT && output_counter < 4);
    end

    //================================================================
    //  Output
    //================================================================

    // output counter
    always_ff @(posedge clk) begin
        if (rst)
            output_counter <= 0;
        else if (opsum_valid && opsum_ready)
            output_counter <= output_counter + 1;
        else if (cur_state == LOAD_IDLE)
            output_counter <= 0;
        else
           output_counter <= output_counter;
    end

    // output
    always_comb begin
        opsum = ipsum_spad[output_counter];
    end

endmodule
