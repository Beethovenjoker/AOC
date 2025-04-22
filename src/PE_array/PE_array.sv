`include "src/PE_array/PE.sv"
`include "src/PE_array/GIN/GIN.sv"
`include "src/PE_array/GON/GON.sv"

module PE_array #(
    parameter NUMS_PE_ROW = `NUMS_PE_ROW,
    parameter NUMS_PE_COL = `NUMS_PE_COL,
    parameter XID_BITS = `XID_BITS,
    parameter YID_BITS = `YID_BITS,
    parameter DATA_SIZE = `DATA_BITS,
    parameter CONFIG_SIZE = `CONFIG_SIZE
)(
    input clk,
    input rst,

    /* Scan Chain */
    input set_XID,
    input [`XID_BITS-1:0] ifmap_XID_scan_in,
    input [`XID_BITS-1:0] filter_XID_scan_in,
    input [`XID_BITS-1:0] ipsum_XID_scan_in,
    input [`XID_BITS-1:0] opsum_XID_scan_in,

    input set_YID,
    input [`YID_BITS-1:0] ifmap_YID_scan_in,
    input [`YID_BITS-1:0] filter_YID_scan_in,
    input [`YID_BITS-1:0] ipsum_YID_scan_in,
    input [`YID_BITS-1:0] opsum_YID_scan_in,

    input set_LN,
    input [`NUMS_PE_ROW-2:0] LN_config_in,

    /* Controller */
    input [`NUMS_PE_ROW*`NUMS_PE_COL-1:0] PE_en,
    input [`CONFIG_SIZE-1:0] PE_config,
    input [`XID_BITS-1:0] ifmap_tag_X,
    input [`YID_BITS-1:0] ifmap_tag_Y,
    input [`XID_BITS-1:0] filter_tag_X,
    input [`YID_BITS-1:0] filter_tag_Y,
    input [`XID_BITS-1:0] ipsum_tag_X,
    input [`YID_BITS-1:0] ipsum_tag_Y,
    input [`XID_BITS-1:0] opsum_tag_X,
    input [`YID_BITS-1:0] opsum_tag_Y,

    /* GLB */
    input GLB_ifmap_valid,
    output logic GLB_ifmap_ready,
    input GLB_filter_valid,
    output logic GLB_filter_ready,
    input GLB_ipsum_valid,
    output logic GLB_ipsum_ready,
    input [DATA_SIZE-1:0] GLB_data_in,
    output logic GLB_opsum_valid,
    input GLB_opsum_ready,
    output logic [DATA_SIZE-1:0] GLB_data_out

);

    //GIN_ifmap
    logic [`NUMS_PE_ROW*`NUMS_PE_COL-1:0] GIN_ifmap_pe_vaild;
    logic [`NUMS_PE_ROW*`NUMS_PE_COL-1:0] GIN_ifmap_pe_ready;
    logic [DATA_SIZE-1:0] GIN_ifmap_pe_data;

    //GIN_filter
    logic [`NUMS_PE_ROW*`NUMS_PE_COL-1:0] GIN_filter_pe_vaild;
    logic [`NUMS_PE_ROW*`NUMS_PE_COL-1:0] GIN_filter_pe_ready;
    logic [DATA_SIZE-1:0] GIN_filter_pe_data;

    //GIN_ipsum
    logic [`NUMS_PE_ROW*`NUMS_PE_COL-1:0] GIN_ipsum_pe_vaild;
    logic [`NUMS_PE_ROW*`NUMS_PE_COL-1:0] GIN_ipsum_pe_ready;
    logic [DATA_SIZE-1:0] GIN_ipsum_pe_data;

    //GON_opsum
    logic [`NUMS_PE_ROW*`NUMS_PE_COL-1:0] GON_opsum_pe_vaild;
    logic [`NUMS_PE_ROW*`NUMS_PE_COL-1:0] GON_opsum_pe_ready;
    logic [`NUMS_PE_ROW*`NUMS_PE_COL*DATA_SIZE-1:0] GON_opsum_pe_data;

    //local network
    logic [(`NUMS_PE_ROW-1)*`NUMS_PE_COL-1:0] LN;
    logic [`DATA_BITS * `NUMS_PE_ROW * `NUMS_PE_COL - 1:0] opsum;
    logic [`DATA_BITS * `NUMS_PE_ROW * `NUMS_PE_COL - 1:0] ipsum;
    logic [`NUMS_PE_ROW*`NUMS_PE_COL-1:0] ipsum_valid;
    logic [`NUMS_PE_ROW*`NUMS_PE_COL-1:0] ipsum_ready;
    logic [`NUMS_PE_ROW*`NUMS_PE_COL-1:0] opsum_valid;
    logic [`NUMS_PE_ROW*`NUMS_PE_COL-1:0] opsum_ready;

    GIN GIN_ifmap(
        .clk(clk),
        .rst(rst),

        // Slave SRAM <-> GIN
        .GIN_valid(GLB_ifmap_valid),
        .GIN_ready(GLB_ifmap_ready),
        .GIN_data(GLB_data_in),

        /* Controller <-> GIN */
        .tag_X(ifmap_tag_X),
        .tag_Y(ifmap_tag_Y),

        /* config */
        .set_XID(set_XID),
        .XID_scan_in(ifmap_XID_scan_in),
        .set_YID(set_YID),   
        .YID_scan_in(ifmap_YID_scan_in),

        // Master GIN <-> PE
        .PE_ready(GIN_ifmap_pe_ready),
        .PE_valid(GIN_ifmap_pe_vaild),
        .PE_data(GIN_ifmap_pe_data)
    );

    GIN GIN_filter(
        .clk(clk),
        .rst(rst),

        // Slave SRAM <-> GIN
        .GIN_valid(GLB_filter_valid),
        .GIN_ready(GLB_filter_ready),
        .GIN_data(GLB_data_in),

        /* Controller <-> GIN */
        .tag_X(filter_tag_X),
        .tag_Y(filter_tag_Y),

        /* config */
        .set_XID(set_XID),
        .XID_scan_in(filter_XID_scan_in),
        .set_YID(set_YID),   
        .YID_scan_in(filter_YID_scan_in),

        // Master GIN <-> PE
        .PE_ready(GIN_filter_pe_ready),
        .PE_valid(GIN_filter_pe_vaild),
        .PE_data(GIN_filter_pe_data)
    );

    GIN GIN_ipsum(
        .clk(clk),
        .rst(rst),

        // Slave SRAM <-> GIN
        .GIN_valid(GLB_ipsum_valid),
        .GIN_ready(GLB_ipsum_ready),
        .GIN_data(GLB_data_in),

        /* Controller <-> GIN */
        .tag_X(ipsum_tag_X),
        .tag_Y(ipsum_tag_Y),

        /* config */
        .set_XID(set_XID),
        .XID_scan_in(ipsum_XID_scan_in),
        .set_YID(set_YID),   
        .YID_scan_in(ipsum_YID_scan_in),

        // Master GIN <-> PE
        .PE_ready(GIN_ipsum_pe_ready),
        .PE_valid(GIN_ipsum_pe_vaild),
        .PE_data(GIN_ipsum_pe_data)
    );

    GON GON_opsum(
        .clk(clk),
        .rst(rst),

        // Slave SRAM <-> GIN
        .GON_valid(GLB_opsum_valid),
        .GON_ready(GLB_opsum_ready),
        .GON_data(GLB_data_out),

        /* Controller <-> GIN */
        .tag_X(opsum_tag_X),
        .tag_Y(opsum_tag_Y),

        /* config */
        .set_XID(set_XID),
        .XID_scan_in(opsum_XID_scan_in),
        .set_YID(set_YID),
        .YID_scan_in(opsum_YID_scan_in),

        // Master GIN <-> PE
        .PE_valid(GON_opsum_pe_vaild),
        .PE_ready(GON_opsum_pe_ready),
        .PE_data(GON_opsum_pe_data)
    );


    generate
        for (genvar i = 0; i < `NUMS_PE_ROW * `NUMS_PE_COL; i = i + 1) begin : PEs
            PE pe(
                .clk(clk),
                .rst(rst),
                .PE_en(PE_en[i]),
                .i_config(PE_config),

                .ifmap(GIN_ifmap_pe_data),
                .filter(GIN_filter_pe_data),
                .ipsum(ipsum[i*`DATA_BITS +: `DATA_BITS]),

                .ifmap_valid(GIN_ifmap_pe_vaild[i]),
                .filter_valid(GIN_filter_pe_vaild[i]),
                .ipsum_valid(ipsum_valid[i]),
                .opsum_ready(opsum_ready[i]),

                .opsum(opsum[i*`DATA_BITS +: `DATA_BITS]),
                .ifmap_ready(GIN_ifmap_pe_ready[i]),
                .filter_ready(GIN_filter_pe_ready[i]),
                .ipsum_ready(ipsum_ready[i]),
                .opsum_valid(opsum_valid[i])
            );
        end
    endgenerate

    always_ff @(posedge clk) begin
        if(rst)
            LN <= 40'd0;
        else if(set_LN) begin
            LN[7:0]   <= {8{LN_config_in[0]}};
            LN[15:8]  <= {8{LN_config_in[1]}};
            LN[23:16] <= {8{LN_config_in[2]}};
            LN[31:24] <= {8{LN_config_in[3]}};
            LN[39:32] <= {8{LN_config_in[4]}};
        end else
            LN <= LN;
    end

    //PE input
    always_comb begin
        for (int i = 0; i < `NUMS_PE_ROW * `NUMS_PE_COL; i=i+1) begin
            if (i < 40) begin
                ipsum[i*`DATA_BITS +: `DATA_BITS] = (LN[i]) ? opsum[(i+8)*`DATA_BITS +: `DATA_BITS] : GIN_ipsum_pe_data;
                ipsum_valid[i] = (LN[i]) ? opsum_valid[i+`NUMS_PE_COL] : GIN_ipsum_pe_vaild[i];
                GIN_ipsum_pe_ready[i] = (LN[i]) ? 1'b0 : ipsum_ready[i];
            end else begin
                ipsum[i*`DATA_BITS +: `DATA_BITS] = GIN_ipsum_pe_data;
                ipsum_valid[i] = GIN_ipsum_pe_vaild[i];
                GIN_ipsum_pe_ready[i] = ipsum_ready[i];
            end
        end
        for (int i = 0; i < `NUMS_PE_ROW * `NUMS_PE_COL; i=i+1) begin
            if (i > 7) begin
                opsum_ready[i] = (LN[i-8]) ? ipsum_ready[i-`NUMS_PE_COL] : GON_opsum_pe_ready[i];
                GON_opsum_pe_data[i*`DATA_BITS +: `DATA_BITS] = (LN[i-8]) ? 32'd0 : opsum[i*`DATA_BITS +: `DATA_BITS];
                GON_opsum_pe_vaild[i] = (LN[i-8]) ? 1'b0 : opsum_valid[i];
            end else begin
                opsum_ready[i] = GON_opsum_pe_ready[i];
                GON_opsum_pe_data[i*`DATA_BITS +: `DATA_BITS] = opsum[i*`DATA_BITS +: `DATA_BITS];
                GON_opsum_pe_vaild[i] = opsum_valid[i];
            end
        end
    end

endmodule
