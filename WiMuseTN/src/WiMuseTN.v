// MSX WiMuseTN unit - TangNano @Harumakkin
`default_nettype none

module WiMuseTN (
    input wire msx_req,
    input wire [7:0] msx_ad,
    input wire [7:0] msx_dt,
    input wire esp_req,
    output reg [2:0] esp_ct,    // update counter
    output reg [2:0] esp_ad,    // decoded address
    output reg [7:0] esp_dt     // data
);

parameter BUFFSIZE = 32;
reg [4:0] buff_wp;
reg [4:0] buff_rp;

reg [2:0] buff_ct[BUFFSIZE];
reg [2:0] buff_ad[BUFFSIZE];
reg [7:0] buff_dt[BUFFSIZE];
reg [2:0] tmp_ct;

initial begin
    buff_ct[0] = 3'b000;
    tmp_ct = 3'b000;
    buff_wp = 5'b0;
    buff_rp = 5'b0;
end

function [3:0] getAddress(input [7:0] ad);
    case(ad)
        8'h00 :  getAddress = 4'b1_000;  // (for test)
        8'h01 :  getAddress = 4'b1_001;  // (for test) 
        8'h7C :  getAddress = 4'b1_010;  // OPLL ADDRESS REG.
        8'h7D :  getAddress = 4'b1_011;  // OPLL DATA REG.
        8'hA0 :  getAddress = 4'b1_100;  // PSG  ADDRESS REG.
        8'hA1 :  getAddress = 4'b1_101;  // PSG  DATA REG.
        8'hC0 :  getAddress = 4'b1_110;  // MSX-AUDIO ADDRESS REG.
        8'hC1 :  getAddress = 4'b1_111;  // MSX-AUDIO DATA REG.
        default: getAddress = 4'b0_000;  // nothing
    endcase
endfunction

assign esp_ct = buff_ct[buff_rp];
assign esp_ad = buff_ad[buff_rp];
assign esp_dt = buff_dt[buff_rp];

reg [3:0] state;
assign state = getAddress(msx_ad[7:0]);

always @(negedge msx_req) begin
    if( state[3] ) begin
        tmp_ct = tmp_ct + 4'b1;
        buff_ad[buff_wp + 5'b1] = state[2:0];
        buff_dt[buff_wp + 5'b1] = msx_dt;
        buff_ct[buff_wp + 5'b1] = tmp_ct;
        buff_ad[buff_wp] = state[2:0];
        buff_dt[buff_wp] = msx_dt;
        buff_ct[buff_wp] = tmp_ct;
        buff_wp = buff_wp + 5'b1;
    end
end

always @(negedge esp_req) begin
    buff_rp = buff_rp + ((buff_wp != buff_rp) ? 5'b1 : 5'b0);
end

endmodule
