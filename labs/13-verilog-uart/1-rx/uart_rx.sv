module uart_rx
    (
        input logic clk, rst,
        input logic rx,            // serial data
        input logic tick,          // baud rate oversampled tick
        output logic rx_done_tick, // pulse one tick when done
        output logic [7:0] dout    // output data
    );

<<<<<<< HEAD
    typedef enum {idle, start, data, stop} state_t;
    state_t state_reg = idle;
    state_t state_next;
=======
    /* verilator public_module */

    parameter idle = 2'b00;
    parameter start = 2'b01;
    parameter data = 2'b10;
    parameter stop = 2'b11;
>>>>>>> 5b46d5074052e3418b03a56efa421503e6f54125

    // typedef enum {idle, start, data, stop} state_t;
    reg[1:0] state_reg = idle;
    reg[1:0] state_next;

    assign dout = 1;
    // TODO
endmodule
