#ifndef __ST7789_H
#define __ST7789_H

#include <string.h>
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"

static const uint ST7789_NO_CONNECT = ~0x0;

static const uint8_t ST7789_CMD_NOP            = 0x00;
static const uint8_t ST7789_CMD_SWRESET        = 0x01;
static const uint8_t ST7789_CMD_SLPIN          = 0x10;
static const uint8_t ST7789_CMD_SLPOUT         = 0x11;
static const uint8_t ST7789_CMD_NORON          = 0x13;
static const uint8_t ST7789_CMD_INVON          = 0x21;
static const uint8_t ST7789_CMD_DISPOFF        = 0x28;
static const uint8_t ST7789_CMD_DISPON         = 0x29;
static const uint8_t ST7789_CMD_CASET          = 0x2a;
static const uint8_t ST7789_CMD_RASET          = 0x2b;
static const uint8_t ST7789_CMD_RAMWR          = 0x2c;
static const uint8_t ST7789_CMD_MADCTL         = 0x36;
static const uint8_t ST7789_CMD_COLMOD         = 0x3a;

static const uint8_t ST7789_ORIENT_NORMAL      = 0x00;
static const uint8_t ST7789_ORIENT_MIRROR_Y    = 0x90;
static const uint8_t ST7789_ORIENT_MIRROR_X    = 0x44;
static const uint8_t ST7789_ORIENT_ROT_180     = 0xd4;

static const uint8_t ST7789_ORIENT_SWAP_XY     = 0x20;
static const uint8_t ST7789_ORIENT_ROT_270     = 0xb4;
static const uint8_t ST7789_ORIENT_ROT_90      = 0x64;
static const uint8_t ST7789_ORIENT_MIRROR_SWAP_XY = 0xf4;

static const uint8_t ST7789_COLMOD_65K = 0x55;
static const uint8_t ST7789_COLMOD_262K = 0x66;

typedef struct {
    spi_inst_t* spi;
    uint pin_din;
    uint pin_clk;
} st7789_serial_intf_t;

typedef struct {
    PIO pio;
    uint sm;
    uint num_bits;
    uint pin_wr_rd_base;
    uint pin_data_base;
} st7789_parallel_intf_t;

typedef union {
    st7789_serial_intf_t si;
    st7789_parallel_intf_t pi;
} st7789_intf_t;

typedef struct {
    bool serial;
    uint dma_chan;
    uint pin_rst;
    uint pin_bl;
    uint pin_cs;
    uint pin_dc;
    st7789_intf_t intf;
} st7789_cfg_t;

// Initialize the display controller
// cfg indicates whether a serial (SPI) or parallel (PIO) intrface is used, which controller to use, and certains pins
//      a NULL value means a default SPI, with default pins are used
// width, height are screen dimensions of the attached display; when x/y axes aree flipped (ie, rot_90 or rot_270), the with and height shoudl also be exchanged
// orient indicates the orientation of the data being transferred to the display controller
void st7789_init(const st7789_cfg_t* cfg, uint width, uint height, uint8_t orient);
// The window in which rendering will occur
// xs, ys are the upper-left corner of the window (inclusive)
// xe, ye are the lwer-right corner of the window (inclusive)
void st7789_set_window(uint16_t xs, uint16_t ys, uint16_t xe, uint16_t ye);
// Writes data to the framebuffer at 16 bits per pixel (565 format)
void st7789_write(const void* data, size_t len);
// returns once the prior write transfer is completed
// must be called before the next one can be issued
// will return immediately if no writes have been issued
void st7789_wait_for_write();
// fills the framebuffer with a single color in 565 format
// this will also reset the window to cover the entire screen
void st7789_fill(uint16_t pixel);

#endif