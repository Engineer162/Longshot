/*
 * si4463.c — SI4463 EZRadioPRO transceiver driver implementation.
 */

#include "si4463.h"
#include <string.h>

/* -------------------------------------------------------------------------
 * Internal helpers
 * ---------------------------------------------------------------------- */

/* Maximum number of polls waiting for CTS */
#define CTS_POLL_MAX    2000

/*
 * Wait for the SI4463 to assert CTS by reading the command buffer
 * (READ_CMD_BUFF). Returns SI4463_OK once CTS == 0xFF.
 */
static si4463_err_t wait_cts(si4463_t *dev)
{
    uint8_t tx[2] = { SI4463_CMD_READ_CMD_BUFF, 0xFF };
    uint8_t rx[2];
    int     retries = CTS_POLL_MAX;

    while (retries-- > 0) {
        if (dev->hal.spi_transfer(tx, rx, 2) != 0) {
            continue;
        }
        if (rx[1] == SI4463_CTS_READY) {
            return SI4463_OK;
        }
    }
    return SI4463_ERR_CTS;
}

/*
 * Send a command and wait for CTS. `cmd_buf` is the full command including
 * the command byte. The device must already have CTS before entry.
 */
static si4463_err_t send_command(si4463_t *dev, const uint8_t *cmd_buf,
                                 size_t cmd_len)
{
    int ret = dev->hal.spi_transfer(cmd_buf, NULL, cmd_len);
    if (ret != 0) {
        return SI4463_ERR_CTS;
    }
    return wait_cts(dev);
}

/*
 * Send a command then read `resp_len` bytes of response into `resp`.
 */
static si4463_err_t send_command_read(si4463_t *dev,
                                      const uint8_t *cmd_buf, size_t cmd_len,
                                      uint8_t *resp, size_t resp_len)
{
    si4463_err_t err = send_command(dev, cmd_buf, cmd_len);
    if (err != SI4463_OK) {
        return err;
    }
    /* After wait_cts the response data is already in the RX buffer of the
     * last READ_CMD_BUFF transfer. Re-issue READ_CMD_BUFF to collect more
     * response bytes if needed. */
    uint8_t tx[1 + 64];
    uint8_t rx[1 + 64];
    size_t  total = 1 + resp_len;

    if (total > sizeof(tx)) {
        return SI4463_ERR_PARAM;
    }
    memset(tx, 0xFF, total);
    tx[0] = SI4463_CMD_READ_CMD_BUFF;

    if (dev->hal.spi_transfer(tx, rx, total) != 0) {
        return SI4463_ERR_CTS;
    }
    /* rx[0] is CTS (should be 0xFF), response starts at rx[1] */
    memcpy(resp, &rx[1], resp_len);
    return SI4463_OK;
}

/*
 * Set a single property (1 byte value).
 */
static si4463_err_t set_property(si4463_t *dev, uint8_t group, uint8_t index,
                                 uint8_t value)
{
    uint8_t cmd[5] = {
        SI4463_CMD_SET_PROPERTY,
        group,
        1,      /* num_props */
        index,
        value
    };
    return send_command(dev, cmd, sizeof(cmd));
}

/*
 * Set multiple consecutive properties.
 */
static si4463_err_t set_properties(si4463_t *dev, uint8_t group,
                                   uint8_t start_index, const uint8_t *values,
                                   uint8_t count)
{
    uint8_t cmd[4 + 12];  /* max 12 props per command per SI4463 spec */
    if (count > 12) {
        return SI4463_ERR_PARAM;
    }
    cmd[0] = SI4463_CMD_SET_PROPERTY;
    cmd[1] = group;
    cmd[2] = count;
    cmd[3] = start_index;
    memcpy(&cmd[4], values, count);
    return send_command(dev, cmd, 4 + count);
}

/* -------------------------------------------------------------------------
 * Frequency calculation helpers
 *
 * The SI4463 uses: F_rf = (inte + 1 + frac/2^19) * xo_freq * outdiv_factor
 * For simplicity this driver targets the 850–1050 MHz band (outdiv = 4).
 * Users targeting other bands should adjust outdiv accordingly.
 * ---------------------------------------------------------------------- */

#define OUTDIV_850_1050  4
#define OUTDIV_284_350   8

static void calc_frequency_regs(uint32_t freq_hz, uint32_t xo_hz,
                                 uint8_t *inte, uint32_t *frac,
                                 uint8_t *band)
{
    uint8_t  outdiv;
    uint64_t f;

    if (freq_hz >= 850000000UL) {
        outdiv = OUTDIV_850_1050;
        *band  = 0;
    } else if (freq_hz >= 420000000UL) {
        outdiv = 8;
        *band  = 2;
    } else if (freq_hz >= 284000000UL) {
        outdiv = 12;
        *band  = 3;
    } else if (freq_hz >= 142000000UL) {
        outdiv = 24;
        *band  = 5;
    } else {
        outdiv = 24;
        *band  = 5;
    }

    /* F_rf = (INTE + 1 + FRAC/2^19) * xo_hz / outdiv
     * => INTE + 1 + FRAC/2^19 = F_rf * outdiv / xo_hz
     * Multiply by 2^19 to keep integer arithmetic.
     */
    f       = (uint64_t)freq_hz * outdiv;
    f      *= (1u << 19);
    f      /= xo_hz;

    *inte   = (uint8_t)((f >> 19) - 1u);
    *frac   = (uint32_t)(f & 0x7FFFFu);

    (void)outdiv; /* suppress unused-variable warning in some compilers */
}

/* -------------------------------------------------------------------------
 * Public API
 * ---------------------------------------------------------------------- */

si4463_err_t si4463_init(si4463_t *dev, const si4463_hal_t *hal,
                         const si4463_config_t *cfg)
{
    if (!dev || !hal || !cfg) {
        return SI4463_ERR_PARAM;
    }
    memcpy(&dev->hal, hal, sizeof(si4463_hal_t));
    memcpy(&dev->cfg, cfg, sizeof(si4463_config_t));

    /* Hardware reset */
    dev->hal.sdn_write(1);
    dev->hal.delay_ms(10);
    dev->hal.sdn_write(0);
    dev->hal.delay_ms(20);

    /* POWER_UP command */
    uint8_t xo_bytes[4] = {
        (uint8_t)(cfg->xo_freq_hz >> 24),
        (uint8_t)(cfg->xo_freq_hz >> 16),
        (uint8_t)(cfg->xo_freq_hz >>  8),
        (uint8_t)(cfg->xo_freq_hz >>  0),
    };
    uint8_t power_up[7] = {
        SI4463_CMD_POWER_UP,
        SI4463_POWER_UP_FUNC_PRO,
        SI4463_POWER_UP_XTAL,
        xo_bytes[0], xo_bytes[1], xo_bytes[2], xo_bytes[3]
    };
    si4463_err_t err = send_command(dev, power_up, sizeof(power_up));
    if (err != SI4463_OK) {
        return err;
    }

    /* Verify part ID */
    uint8_t part_info_cmd = SI4463_CMD_PART_INFO;
    uint8_t part_info[8];
    err = send_command_read(dev, &part_info_cmd, 1, part_info,
                            sizeof(part_info));
    if (err != SI4463_OK) {
        return err;
    }
    uint16_t part_id = ((uint16_t)part_info[1] << 8) | part_info[2];
    if (part_id != 0x4463) {
        return SI4463_ERR_PART;
    }

    /* Configure GPIO0 as TX_STATE (high during TX) to drive SKY66423-11 CTX */
    uint8_t gpio_cfg[8] = {
        SI4463_CMD_GPIO_PIN_CFG,
        SI4463_GPIO_MODE_TX_STATE, /* GPIO0 -> TX_STATE */
        SI4463_GPIO_MODE_RX_STATE, /* GPIO1 -> RX_STATE (diagnostic) */
        SI4463_GPIO_MODE_DONOTHING,
        SI4463_GPIO_MODE_DONOTHING,
        SI4463_GPIO_MODE_DONOTHING, /* NIRQ */
        SI4463_GPIO_MODE_DONOTHING, /* SDO  */
        0x00                         /* drive strength: low */
    };
    err = send_command(dev, gpio_cfg, sizeof(gpio_cfg));
    if (err != SI4463_OK) {
        return err;
    }

    /* Set frequency */
    err = si4463_set_frequency(dev, cfg->frequency_hz);
    if (err != SI4463_OK) {
        return err;
    }

    /* Modulation type and data rate */
    uint32_t data_rate = cfg->data_rate_bps;
    uint8_t  modem_props[10] = {
        cfg->mod_type,
        0x00,  /* MAP_CONTROL */
        0x07,  /* DSM_CTRL   */
        (uint8_t)(data_rate >> 16),
        (uint8_t)(data_rate >>  8),
        (uint8_t)(data_rate >>  0),
        /* TX NCO: 0x04 << 26 | xo_freq_hz */
        (uint8_t)((0x04u << 2) | ((cfg->xo_freq_hz >> 24) & 0x03)),
        (uint8_t)(cfg->xo_freq_hz >> 16),
        (uint8_t)(cfg->xo_freq_hz >>  8),
        (uint8_t)(cfg->xo_freq_hz >>  0),
    };
    err = set_properties(dev, SI4463_PROP_MODEM, 0x00, modem_props,
                         sizeof(modem_props));
    if (err != SI4463_OK) {
        return err;
    }

    /* Frequency deviation (FSK/GFSK) */
    uint32_t dev_hz = cfg->freq_dev_hz;
    uint8_t  dev_regs[3] = {
        (uint8_t)(dev_hz >> 16),
        (uint8_t)(dev_hz >>  8),
        (uint8_t)(dev_hz >>  0),
    };
    err = set_properties(dev, SI4463_PROP_MODEM, 0x0A, dev_regs,
                         sizeof(dev_regs));
    if (err != SI4463_OK) {
        return err;
    }

    /* PA power level */
    err = si4463_set_tx_power(dev, cfg->tx_power_lvl);
    if (err != SI4463_OK) {
        return err;
    }

    /* Enable packet-handler interrupts: PACKET_SENT, PACKET_RX, CRC_ERROR */
    err = set_property(dev, SI4463_PROP_INT_CTL,
                       SI4463_PROP_INT_CTL_ENABLE & 0xFF, 0x01);
    if (err != SI4463_OK) {
        return err;
    }
    err = set_property(dev, SI4463_PROP_INT_CTL,
                       SI4463_PROP_INT_CTL_PH_ENABLE & 0xFF,
                       SI4463_PH_STATUS_PACKET_SENT |
                       SI4463_PH_STATUS_PACKET_RX   |
                       SI4463_PH_STATUS_CRC_ERROR);
    return err;
}

si4463_err_t si4463_transmit(si4463_t *dev, const uint8_t *data, size_t len)
{
    if (!dev || !data || len == 0 || len > SI4463_TX_FIFO_SIZE) {
        return SI4463_ERR_PARAM;
    }

    /* Reset TX FIFO */
    uint8_t fifo_reset[2] = { SI4463_CMD_FIFO_INFO, 0x01 };
    si4463_err_t err = send_command(dev, fifo_reset, sizeof(fifo_reset));
    if (err != SI4463_OK) {
        return err;
    }

    /* Write payload into TX FIFO */
    uint8_t write_buf[1 + SI4463_TX_FIFO_SIZE];
    write_buf[0] = SI4463_CMD_WRITE_TX_FIFO;
    memcpy(&write_buf[1], data, len);
    if (dev->hal.spi_transfer(write_buf, NULL, 1 + len) != 0) {
        return SI4463_ERR_TX;
    }

    /* START_TX: channel, condition=TX_COMPLETE→READY, length */
    uint8_t start_tx[5] = {
        SI4463_CMD_START_TX,
        dev->cfg.channel,
        0x30,                /* condition: go to READY after TX */
        (uint8_t)(len >> 8),
        (uint8_t)(len >> 0),
    };
    err = send_command(dev, start_tx, sizeof(start_tx));
    if (err != SI4463_OK) {
        return err;
    }

    /* Wait for NIRQ (PACKET_SENT) */
    uint32_t timeout = 5000;
    while (timeout-- > 0) {
        if (dev->hal.nirq_read()) {
            /* Clear interrupt status */
            uint8_t clr_int[4] = { SI4463_CMD_GET_INT_STATUS, 0, 0, 0 };
            uint8_t int_resp[8];
            send_command_read(dev, clr_int, sizeof(clr_int), int_resp,
                              sizeof(int_resp));
            if (int_resp[3] & SI4463_PH_STATUS_PACKET_SENT) {
                return SI4463_OK;
            }
        }
        dev->hal.delay_ms(1);
    }
    return SI4463_ERR_TX;
}

si4463_err_t si4463_receive(si4463_t *dev, uint8_t *buf, size_t buf_len,
                            size_t *rx_len, uint32_t timeout_ms)
{
    if (!dev || !buf || buf_len == 0 || !rx_len) {
        return SI4463_ERR_PARAM;
    }

    /* Reset RX FIFO */
    uint8_t fifo_reset[2] = { SI4463_CMD_FIFO_INFO, 0x02 };
    si4463_err_t err = send_command(dev, fifo_reset, sizeof(fifo_reset));
    if (err != SI4463_OK) {
        return err;
    }

    /* START_RX */
    uint8_t start_rx[7] = {
        SI4463_CMD_START_RX,
        dev->cfg.channel,
        0x00,
        0x00, 0x00,     /* packet length = 0 (use field length) */
        SI4463_STATE_NOCHANGE,
        SI4463_STATE_READY,  /* go to READY on valid packet */
        SI4463_STATE_READY,  /* go to READY on bad packet  */
    };
    err = send_command(dev, start_rx, sizeof(start_rx));
    if (err != SI4463_OK) {
        return err;
    }

    uint32_t elapsed = 0;
    while (timeout_ms == 0 || elapsed < timeout_ms) {
        if (dev->hal.nirq_read()) {
            uint8_t clr_int[4] = { SI4463_CMD_GET_INT_STATUS, 0, 0, 0 };
            uint8_t int_resp[8];
            send_command_read(dev, clr_int, sizeof(clr_int), int_resp,
                              sizeof(int_resp));

            if (int_resp[3] & SI4463_PH_STATUS_CRC_ERROR) {
                return SI4463_ERR_RX;
            }

            if (int_resp[3] & SI4463_PH_STATUS_PACKET_RX) {
                /* Query FIFO byte count */
                uint8_t fifo_info_cmd[2] = { SI4463_CMD_FIFO_INFO, 0x00 };
                uint8_t fifo_resp[2];
                err = send_command_read(dev, fifo_info_cmd,
                                        sizeof(fifo_info_cmd),
                                        fifo_resp, sizeof(fifo_resp));
                if (err != SI4463_OK) {
                    return err;
                }
                uint8_t avail = fifo_resp[1]; /* RX FIFO byte count */
                if (avail > buf_len) {
                    avail = (uint8_t)buf_len;
                }

                /* Read RX FIFO */
                uint8_t read_cmd[1 + SI4463_RX_FIFO_SIZE];
                uint8_t read_rsp[1 + SI4463_RX_FIFO_SIZE];
                read_cmd[0] = SI4463_CMD_READ_RX_FIFO;
                memset(&read_cmd[1], 0xFF, avail);
                if (dev->hal.spi_transfer(read_cmd, read_rsp,
                                          1 + avail) != 0) {
                    return SI4463_ERR_RX;
                }
                memcpy(buf, &read_rsp[1], avail);
                *rx_len = avail;
                return SI4463_OK;
            }
        }
        dev->hal.delay_ms(1);
        elapsed++;
    }
    return SI4463_ERR_RX;
}

int8_t si4463_get_rssi(si4463_t *dev)
{
    if (!dev) {
        return 0;
    }
    uint8_t cmd = SI4463_CMD_GET_MODEM_STATUS;
    uint8_t resp[8];
    if (send_command_read(dev, &cmd, 1, resp, sizeof(resp)) != SI4463_OK) {
        return 0;
    }
    /* resp[2] = CURR_RSSI (raw, dBm = raw/2 - 130 for SI4463) */
    return (int8_t)((int16_t)resp[2] / 2 - 130);
}

si4463_err_t si4463_sleep(si4463_t *dev)
{
    uint8_t cmd[2] = { SI4463_CMD_CHANGE_STATE, SI4463_STATE_SLEEP };
    return send_command(dev, cmd, sizeof(cmd));
}

si4463_err_t si4463_wakeup(si4463_t *dev)
{
    uint8_t cmd[2] = { SI4463_CMD_CHANGE_STATE, SI4463_STATE_READY };
    return send_command(dev, cmd, sizeof(cmd));
}

si4463_err_t si4463_set_frequency(si4463_t *dev, uint32_t frequency_hz)
{
    if (!dev) {
        return SI4463_ERR_PARAM;
    }
    dev->cfg.frequency_hz = frequency_hz;

    uint8_t  inte;
    uint32_t frac;
    uint8_t  band;
    calc_frequency_regs(frequency_hz, dev->cfg.xo_freq_hz, &inte, &frac,
                        &band);

    uint8_t freq_regs[4] = {
        inte,
        (uint8_t)(frac >> 16),
        (uint8_t)(frac >>  8),
        (uint8_t)(frac >>  0),
    };
    return set_properties(dev, SI4463_PROP_FREQ_CONTROL, 0x00, freq_regs,
                          sizeof(freq_regs));
}

si4463_err_t si4463_set_tx_power(si4463_t *dev, uint8_t power_lvl)
{
    if (!dev) {
        return SI4463_ERR_PARAM;
    }
    dev->cfg.tx_power_lvl = power_lvl;
    uint8_t pa_props[2] = {
        0x08,        /* PA_MODE: class-E */
        power_lvl,
    };
    return set_properties(dev, SI4463_PROP_PA, 0x00, pa_props,
                          sizeof(pa_props));
}
