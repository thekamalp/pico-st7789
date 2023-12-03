#include "st7789.h"
#include "font.h"
#include <stdio.h>

// This clock rate is divided in order to get a baudrate on the SPI 
// such that a 256x240x16bpp image can be displayed at 60 fps
#define SYS_CLK_KHZ 236000

// Flip XY causes image to be addressed in column major order
// When sent to the display controller, we set the orientation
// to mirror in y direction
// without flip xy, the image is addressed in row major order,
// and the image is rotated by 270 degrees in the display controller
// Both modes display the same image, but Flip XY, in conjunction with
// double buffering removes the diagonal tear artifiact
// Tearng ay still occur on a scanline, but it is much less obvious
// This tearing may also be removed if the display controller exponses
// the tearing effect signal (TE).  But this logic is not yet implemented
#define FB_FLIP_XY 1
#if FB_FLIP_XY
#define FB_ADDRESS FB_ADDRESS_FLIP
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320
#define SCREEN_ORIENT ST7789_ORIENT_MIRROR_Y
#else
#define FB_ADDRESS FB_ADDRESS_NORMAL
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define SCREEN_ORIENT ST7789_ORIENT_ROT_270
#endif

// framebuffer width and height is the size of the image to be rendered
// if not the full screen, then the rest should be cleared before any rendering occurs
#define FB_WIDTH 256
#define FB_HEIGHT 240
#define SCREEN_WIN_WIDTH ((FB_FLIP_XY) ? FB_HEIGHT : FB_WIDTH)
#define SCREEN_WIN_HEIGHT ((FB_FLIP_XY) ? FB_WIDTH : FB_HEIGHT)
#define SCREEN_WIN_X ((SCREEN_WIDTH - SCREEN_WIN_WIDTH) >> 1)
#define SCREEN_WIN_Y ((SCREEN_HEIGHT - SCREEN_WIN_HEIGHT) >> 1)
#define FB_PIXELS (FB_WIDTH * FB_HEIGHT)
#define FB_SIZE (FB_PIXELS * sizeof(uint16_t))
#define FB_ADDRESS_NORMAL(x, y) ((y) * FB_WIDTH + (x))
#define FB_ADDRESS_FLIP(x, y) ((x) * FB_HEIGHT + (y))
// Single or double buffering
#define FB_BUFFERS 2

uint16_t framebuffer[FB_BUFFERS * FB_PIXELS];

uint16_t* draw_frame = framebuffer;
uint16_t* disp_frame = framebuffer + (FB_BUFFERS-1) * FB_PIXELS;

void flip_framebuffer()
{
    uint16_t* temp = draw_frame;
    draw_frame = disp_frame;
    disp_frame = temp;
}

void lcd_init(bool serial)
{
    st7789_cfg_t cfg;
    cfg.serial = serial;
    cfg.pin_cs = PICO_DEFAULT_SPI_CSN_PIN;
    cfg.pin_dc = 20;
    cfg.pin_rst = 21;
    cfg.pin_bl = 22;
    cfg.dma_chan = 1;
    if (serial) {
        cfg.intf.si.spi = PICO_DEFAULT_SPI_INSTANCE;
        cfg.intf.si.pin_din = PICO_DEFAULT_SPI_TX_PIN;
        cfg.intf.si.pin_clk = PICO_DEFAULT_SPI_SCK_PIN;
    } else {
        cfg.intf.pi.pio = pio0;
        cfg.intf.pi.sm = pio_claim_unused_sm(cfg.intf.pi.pio, true);
        cfg.intf.pi.num_bits = 16;
        cfg.intf.pi.pin_wr_rd_base = 18;
        cfg.intf.pi.pin_data_base = 0;
    }
    st7789_init(&cfg, SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_ORIENT);
}

void main()
{
    //uint vco_freq, postdiv1, postdiv2;
    //check_sys_clock_khz(125000, &vco_freq, &postdiv1, &postdiv2);
    set_sys_clock_khz(SYS_CLK_KHZ, false);
    clock_configure(clk_peri,
        0, // Only AUX mux on ADC
        CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS,
        SYS_CLK_KHZ * 1000,
        SYS_CLK_KHZ * 1000);
    stdio_init_all();
    lcd_init(true);

    uint frame = 0;
    static const uint frame_check = 1;  // check fps every frame_check frames; must be power of 2
    uint16_t color = 0x0000;

    const FONT_T* text_font = font[FONT_ID_AIXOID9_F16];
    uint16_t clear_color = 0x07e0;
    uint16_t text_color = 0xf800;
    char text_str[64];
    int text_start_x = 0, text_start_y = 0;
    int text_end_x = text_start_x;
    int text_end_y = text_start_y + text_font->height;
    bool in_text = false;
    uint32_t cur_time, last_time = 0;
    float fps;
    int x, y;
    uint32_t clk = clock_get_hz(clk_peri);
    uint32_t baud = spi_get_baudrate(spi0);

    st7789_fill(clear_color);
    st7789_wait_for_write();
    st7789_set_window(SCREEN_WIN_X, SCREEN_WIN_Y, SCREEN_WIN_X + SCREEN_WIN_WIDTH - 1, SCREEN_WIN_Y + SCREEN_WIN_HEIGHT - 1);
    while (1) {
        if ((frame & (frame_check - 1)) == 0x0) {
            color = ~color;
            cur_time = time_us_32();

            // Wait for 16.667 ms, to render at roughly 60fps
            // May still cause tearing artifiact, since this is not synchronized
            // to display controller
            while (cur_time - last_time < 16667) {
                cur_time = time_us_32();
            }

            // Calculate and report out the frames per second
            fps = cur_time - last_time;
            fps = (frame_check * 1000000) / fps;
            sprintf(text_str, "%0.2f %d", fps, baud);
            text_end_x = text_start_x + strlen(text_str) * text_font->width;
            last_time = cur_time;

            int text_offset_x = 0;
            int text_offset_y = 0;
            int text_char_index = 0;
            int text_index = 0;

            for (y = 0; y < FB_HEIGHT; y++) {
                for (x = 0; x < FB_WIDTH; x++) {
                    in_text = (x >= text_start_x && x < text_end_x && y >= text_start_y && y < text_end_y);
                    if (in_text) {
                        // We're in the text bounding box
                        // Now check if we hit the glyph
                        text_index = text_str[text_char_index] * text_font->stride;
                        uint8_t glyph_row = text_font->glyphs[text_index + text_offset_y];
                        in_text = (glyph_row << text_offset_x) & 0x80;
                        if (x == text_end_x - 1) {
                            // end of row
                            text_offset_x = 0;
                            text_char_index = 0;
                            text_offset_y++;
                        } else {
                            text_offset_x++;
                            if (text_offset_x >= text_font->width) {
                                // end of a character
                                text_offset_x = 0;
                                text_char_index++;
                            }
                        }
                    }
                    draw_frame[FB_ADDRESS(x,y)] = (in_text) ? text_color : color;
                }
            }
            flip_framebuffer();
        }
        // Wait for prior DMA before issuing the next frame's
        st7789_wait_for_write();
        st7789_write(disp_frame, FB_SIZE);
        frame++;
    }
}
