/*
 * si4463.h — SI4463 EZRadioPRO transceiver driver.
 *
 * This driver controls the Silicon Labs SI4463 over SPI. It exposes the
 * minimal API needed by the Longshot module. Platform-specific SPI and GPIO
 * primitives are supplied by the caller through the si4463_hal_t struct.
 */

#ifndef SI4463_H
#define SI4463_H

#include <stdint.h>
#include <stddef.h>
#include "si4463_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Return codes
 * ---------------------------------------------------------------------- */
typedef enum {
    SI4463_OK        =  0,
    SI4463_ERR_CTS   = -1,  /* Device did not become ready in time */
    SI4463_ERR_PART  = -2,  /* Part ID mismatch */
    SI4463_ERR_PARAM = -3,  /* Invalid parameter */
    SI4463_ERR_FIFO  = -4,  /* Payload too large for FIFO */
    SI4463_ERR_TX    = -5,  /* TX did not complete */
    SI4463_ERR_RX    = -6,  /* RX timeout or CRC error */
} si4463_err_t;

/* -------------------------------------------------------------------------
 * Hardware abstraction layer
 *
 * The caller must populate all function pointers before calling
 * si4463_init(). All functions return 0 on success, non-zero on error.
 * ---------------------------------------------------------------------- */
typedef struct {
    /*
     * SPI transfer: assert CS, clock `len` bytes out from `tx_buf` while
     * clocking `len` bytes into `rx_buf`, then deassert CS.
     * Either buffer may be NULL (fill/ignore with 0xFF).
     */
    int (*spi_transfer)(const uint8_t *tx_buf, uint8_t *rx_buf, size_t len);

    /*
     * Drive the SDN (shutdown) pin: 1 = asserted (chip off),
     *                                0 = deasserted (chip on).
     */
    void (*sdn_write)(int level);

    /*
     * Read the NIRQ pin: returns 1 when the IRQ line is asserted (low),
     * 0 when deasserted.
     */
    int (*nirq_read)(void);

    /* Delay in milliseconds. */
    void (*delay_ms)(uint32_t ms);
} si4463_hal_t;

/* -------------------------------------------------------------------------
 * Radio configuration
 * ---------------------------------------------------------------------- */
typedef struct {
    uint32_t frequency_hz;   /* Centre frequency in Hz (e.g. 915000000) */
    uint8_t  channel;        /* Logical channel offset                   */
    uint8_t  mod_type;       /* SI4463_MOD_TYPE_* from si4463_defs.h    */
    uint32_t data_rate_bps;  /* Air data rate in bits per second         */
    uint32_t freq_dev_hz;    /* Frequency deviation in Hz (FSK modes)    */
    uint8_t  tx_power_lvl;   /* Raw PA power level register (0–127)      */
    uint32_t xo_freq_hz;     /* Crystal frequency (typically 30000000)   */
} si4463_config_t;

/* -------------------------------------------------------------------------
 * Driver handle (opaque to callers)
 * ---------------------------------------------------------------------- */
typedef struct {
    si4463_hal_t    hal;
    si4463_config_t cfg;
} si4463_t;

/* -------------------------------------------------------------------------
 * API
 * ---------------------------------------------------------------------- */

/*
 * si4463_init — Reset and initialise the SI4463.
 *
 * Performs a hardware reset via SDN, waits for CTS, verifies the part ID,
 * then configures frequency, modulation, data rate, and PA power.
 *
 * Returns SI4463_OK on success.
 */
si4463_err_t si4463_init(si4463_t *dev, const si4463_hal_t *hal,
                         const si4463_config_t *cfg);

/*
 * si4463_transmit — Load `len` bytes from `data` into the TX FIFO and
 * start a fixed-length packet transmission on the configured channel.
 *
 * Blocks until the PACKET_SENT interrupt fires or a timeout occurs.
 * Returns SI4463_OK on success.
 */
si4463_err_t si4463_transmit(si4463_t *dev, const uint8_t *data, size_t len);

/*
 * si4463_receive — Switch to RX mode and wait for an incoming packet.
 *
 * Blocks until a packet is received, a CRC error occurs, or `timeout_ms`
 * milliseconds elapse (0 = wait forever).
 * On success `*rx_len` holds the number of bytes written to `buf`.
 * Returns SI4463_OK on success.
 */
si4463_err_t si4463_receive(si4463_t *dev, uint8_t *buf, size_t buf_len,
                            size_t *rx_len, uint32_t timeout_ms);

/*
 * si4463_get_rssi — Return the last RSSI reading in dBm (approximate).
 */
int8_t si4463_get_rssi(si4463_t *dev);

/*
 * si4463_sleep — Place the device in sleep/low-power state.
 */
si4463_err_t si4463_sleep(si4463_t *dev);

/*
 * si4463_wakeup — Return the device to READY state from sleep.
 */
si4463_err_t si4463_wakeup(si4463_t *dev);

/*
 * si4463_set_frequency — Change the centre frequency without full
 * reinitialisation.
 */
si4463_err_t si4463_set_frequency(si4463_t *dev, uint32_t frequency_hz);

/*
 * si4463_set_tx_power — Change the PA power level register.
 */
si4463_err_t si4463_set_tx_power(si4463_t *dev, uint8_t power_lvl);

#ifdef __cplusplus
}
#endif

#endif /* SI4463_H */
