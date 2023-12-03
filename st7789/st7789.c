#include "st7789.h"
#include "st7789.pio.h"

static st7789_cfg_t st7789_cfg;
static dma_channel_config st7789_dma_cfg;
static volatile void* st7789_dma_write_addr;
static uint st7789_width;
static uint st7789_height;
static bool st7789_ram_wr = false;

#define ST7789_PIO_HZ 4608000
#define MAX_DMA 12

static void st7789_cmd(uint8_t cmd, const uint8_t* data, size_t len)
{
    if (st7789_cfg.serial) {
        if (st7789_cfg.pin_cs != ST7789_NO_CONNECT) {
            spi_set_format(st7789_cfg.intf.si.spi, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
        } else {
            spi_set_format(st7789_cfg.intf.si.spi, 8, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);
        }
    }
    st7789_ram_wr = false;

    sleep_us(1);
    if (st7789_cfg.pin_cs != ST7789_NO_CONNECT) {
        gpio_put(st7789_cfg.pin_cs, 0);
    }
    gpio_put(st7789_cfg.pin_dc, 0);
    sleep_us(1);

    if (st7789_cfg.serial) {
        spi_write_blocking(st7789_cfg.intf.si.spi, &cmd, 1);
    } else {
        pio_sm_put_blocking(st7789_cfg.intf.pi.pio, st7789_cfg.intf.pi.sm, cmd);
    }

    if (len) {
        sleep_us(1);
        gpio_put(st7789_cfg.pin_dc, 1);
        sleep_us(1);

        if (st7789_cfg.serial) {
            spi_write_blocking(st7789_cfg.intf.si.spi, data, len);
        } else {
            uint i;
            for (i = 0; i < len; i++) {
                pio_sm_put_blocking(st7789_cfg.intf.pi.pio, st7789_cfg.intf.pi.sm, data[i]);
            }
        }
    }

    sleep_us(1);
    if (st7789_cfg.pin_cs != ST7789_NO_CONNECT) {
        gpio_put(st7789_cfg.pin_cs, 1);
    }
    gpio_put(st7789_cfg.pin_dc, 1);
    sleep_us(1);
}

void st7789_set_window(uint16_t xs, uint16_t ys, uint16_t xe, uint16_t ye)
{
    uint8_t dx[] = {xs >> 8, xs & 0xff, xe >> 8, xe & 0xff};
    uint8_t dy[] = {ys >> 8, ys & 0xff, ye >> 8, ye & 0xff};
    st7789_cmd(ST7789_CMD_CASET, dx, sizeof(dx));
    st7789_cmd(ST7789_CMD_RASET, dy, sizeof(dy));
}

void st7789_init(const st7789_cfg_t* cfg, uint width, uint height, uint8_t orient)
{
    if (cfg) {
        st7789_cfg = *cfg;
    } else {
        st7789_cfg.serial = true;
        st7789_cfg.intf.si.spi = PICO_DEFAULT_SPI_INSTANCE;
        st7789_cfg.pin_cs = PICO_DEFAULT_SPI_CSN_PIN;
        st7789_cfg.intf.si.pin_din = PICO_DEFAULT_SPI_TX_PIN;
        st7789_cfg.intf.si.pin_clk = PICO_DEFAULT_SPI_SCK_PIN;
        st7789_cfg.pin_dc = 20;
        st7789_cfg.pin_rst = 21;
        st7789_cfg.pin_bl = 22;
        st7789_cfg.dma_chan = 1;
    }
    st7789_width = width;
    st7789_height = height;

    if (st7789_cfg.serial) {
        // setup SPI
        spi_init(st7789_cfg.intf.si.spi, 62500000);
        st7789_dma_write_addr = &spi_get_hw(st7789_cfg.intf.si.spi)->dr;
        if (st7789_cfg.pin_cs != ST7789_NO_CONNECT) {
            spi_set_format(st7789_cfg.intf.si.spi, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
            gpio_init(st7789_cfg.pin_cs);
            gpio_set_dir(st7789_cfg.pin_cs, GPIO_OUT);
        } else {
            spi_set_format(st7789_cfg.intf.si.spi, 8, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);
        }

        gpio_set_function(st7789_cfg.intf.si.pin_din, GPIO_FUNC_SPI);
        gpio_set_function(st7789_cfg.intf.si.pin_clk, GPIO_FUNC_SPI);
    } else {
        // setup PIO
        uint st7789_16b_offset = pio_add_program(st7789_cfg.intf.pi.pio, &st7789_16b_program);
        // calculate PIO clock divider
        float div = (float)clock_get_hz(clk_sys) / ST7789_PIO_HZ;
        st7789_16b_program_init(&st7789_cfg, st7789_16b_offset, div);

        // start running the sm
        pio_sm_set_enabled(st7789_cfg.intf.pi.pio, st7789_cfg.intf.pi.sm, true);
    }

    gpio_init(st7789_cfg.pin_dc);
    gpio_init(st7789_cfg.pin_rst);
    gpio_init(st7789_cfg.pin_bl);

    gpio_set_dir(st7789_cfg.pin_dc, GPIO_OUT);
    gpio_set_dir(st7789_cfg.pin_rst, GPIO_OUT);
    gpio_set_dir(st7789_cfg.pin_bl, GPIO_OUT);

    // Hard reset
    if (st7789_cfg.pin_cs != ST7789_NO_CONNECT) {
        gpio_put(st7789_cfg.pin_cs, 0);
    }
    gpio_put(st7789_cfg.pin_dc, 0);
    gpio_put(st7789_cfg.pin_rst, 0);
    sleep_ms(100);

    if (st7789_cfg.pin_cs != ST7789_NO_CONNECT) {
        gpio_put(st7789_cfg.pin_cs, 1);
    }
    gpio_put(st7789_cfg.pin_dc, 1);
    gpio_put(st7789_cfg.pin_rst, 1);
    sleep_ms(100);

    st7789_cmd(ST7789_CMD_SWRESET, NULL, 0);
    sleep_ms(150);

    st7789_cmd(ST7789_CMD_SLPOUT, NULL, 0);
    sleep_ms(50);

    st7789_cmd(ST7789_CMD_COLMOD, &ST7789_COLMOD_65K, 1);
    sleep_ms(10);

    st7789_cmd(ST7789_CMD_MADCTL, &orient, 1);
    st7789_set_window(0, 0, st7789_width, st7789_height);

    st7789_cmd(ST7789_CMD_INVON, NULL, 0);
    sleep_ms(10);

    st7789_cmd(ST7789_CMD_NORON, NULL, 0);
    sleep_ms(10);

    st7789_cmd(ST7789_CMD_DISPON, NULL, 0);
    sleep_ms(10);
    gpio_put(st7789_cfg.pin_bl, 1);

    if (st7789_cfg.dma_chan < MAX_DMA) {
        st7789_dma_cfg = dma_channel_get_default_config(st7789_cfg.dma_chan);
        channel_config_set_transfer_data_size(&st7789_dma_cfg, DMA_SIZE_16);
        channel_config_set_read_increment(&st7789_dma_cfg, true);
        channel_config_set_write_increment(&st7789_dma_cfg, false);
        if (st7789_cfg.serial) {
            channel_config_set_dreq(&st7789_dma_cfg, spi_get_dreq(st7789_cfg.intf.si.spi, true));
        } else {
            channel_config_set_dreq(&st7789_dma_cfg, pio_get_dreq(st7789_cfg.intf.pi.pio, st7789_cfg.intf.pi.sm, true));
        }
    }
}

void st7789_ramwr()
{
    sleep_us(1);
    if (st7789_cfg.pin_cs != ST7789_NO_CONNECT) {
        gpio_put(st7789_cfg.pin_cs, 0);
    }
    gpio_put(st7789_cfg.pin_dc, 0);
    sleep_us(1);

    if (st7789_cfg.serial) {
        spi_write_blocking(st7789_cfg.intf.si.spi, &ST7789_CMD_RAMWR, 1);
    } else {
        pio_sm_put_blocking(st7789_cfg.intf.pi.pio, st7789_cfg.intf.pi.sm, ST7789_CMD_RAMWR);
    }
    sleep_us(1);

    if (st7789_cfg.pin_cs != ST7789_NO_CONNECT) {
        gpio_put(st7789_cfg.pin_cs, 0);
    }
    gpio_put(st7789_cfg.pin_dc, 1);
    sleep_us(1);
}

void st7789_write(const void* data, size_t len)
{
    if (!st7789_ram_wr) {
        st7789_ramwr();

        if(st7789_cfg.serial) spi_set_format(st7789_cfg.intf.si.spi, 16, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);
        st7789_ram_wr = true;
    }
    if (st7789_cfg.dma_chan < MAX_DMA) {
        dma_channel_configure(
            st7789_cfg.dma_chan,
            &st7789_dma_cfg,
            st7789_dma_write_addr,
            data,
            len / 2,
            true
        );
    } else {
        if (st7789_cfg.serial) {
            spi_write16_blocking(st7789_cfg.intf.si.spi, data, len / 2);
        } else {
            uint i;
            uint16_t* d = (uint16_t*)data;
            for (i = 0; i < len / 2; i++) {
                pio_sm_put_blocking(st7789_cfg.intf.pi.pio, st7789_cfg.intf.pi.sm, d[i]);
            }
        }
    }
}

void st7789_wait_for_write()
{
    if (st7789_cfg.dma_chan < MAX_DMA) {
        dma_channel_wait_for_finish_blocking(st7789_cfg.dma_chan);
    }
}

void st7789_fill(uint16_t pixel)
{
    if (st7789_cfg.dma_chan < MAX_DMA) {
        uint num_pix = st7789_width * st7789_height;
        channel_config_set_read_increment(&st7789_dma_cfg, false);
        st7789_set_window(0, 0, st7789_width, st7789_height);
        st7789_write(&pixel, num_pix * 2);
        channel_config_set_read_increment(&st7789_dma_cfg, true);
    } else {
        static const uint BUFFER_SIZE = 64;
        uint16_t data[BUFFER_SIZE];
        uint i;
        uint num_pix = st7789_width * st7789_height;
        uint len = (num_pix < BUFFER_SIZE) ? num_pix : BUFFER_SIZE;
        for (i = 0; i < len; i++) {
            data[i] = pixel;
        }

        st7789_set_window(0, 0, st7789_width, st7789_height);
        while (num_pix > 0) {
            st7789_write(data, len * 2);
            num_pix -= len;
            len = (num_pix < BUFFER_SIZE) ? num_pix : BUFFER_SIZE;
        }
    }
}

