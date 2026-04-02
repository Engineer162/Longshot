/*
 * sky66423.h — Skyworks SKY66423-11 front-end module (FEM) driver.
 *
 * The SKY66423-11 integrates a power amplifier (PA), a low-noise amplifier
 * (LNA), and a T/R switch into a single package. It operates from
 * 169 MHz to 960 MHz and is controlled by three digital GPIO lines:
 *
 *   CSD — Chip Shutdown (active high = enabled, low = off)
 *   CTX — TX/RX select  (high = TX mode, low = RX mode)
 *   CPS — Bypass mode   (high = bypass PA/LNA, low = normal)
 *
 * In a Longshot system the CTX pin is driven directly by the SI4463 GPIO0
 * configured as TX_STATE, so the FEM automatically follows the radio state.
 * CSD and CPS are driven by the host MCU.
 */

#ifndef SKY66423_H
#define SKY66423_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Return codes
 * ---------------------------------------------------------------------- */
typedef enum {
    SKY66423_OK        =  0,
    SKY66423_ERR_PARAM = -1,
} sky66423_err_t;

/* -------------------------------------------------------------------------
 * Hardware abstraction layer
 * ---------------------------------------------------------------------- */
typedef struct {
    /* Drive the CSD (chip shutdown) pin: 1 = enabled, 0 = shutdown */
    void (*csd_write)(int level);

    /* Drive the CPS (bypass) pin: 1 = bypass mode, 0 = normal mode */
    void (*cps_write)(int level);

    /*
     * Optional: drive the CTX pin explicitly.
     * If NULL the CTX pin is assumed to be wired directly to the SI4463
     * GPIO0 (TX_STATE) and is managed automatically by the radio hardware.
     */
    void (*ctx_write)(int level);

    /* Delay in microseconds (used for mode-transition settling). */
    void (*delay_us)(uint32_t us);
} sky66423_hal_t;

/* -------------------------------------------------------------------------
 * Operating modes
 * ---------------------------------------------------------------------- */
typedef enum {
    SKY66423_MODE_SHUTDOWN = 0, /* Full shutdown, minimum current        */
    SKY66423_MODE_RX,           /* RX mode (LNA active, T/R in RX path)  */
    SKY66423_MODE_TX,           /* TX mode (PA active, T/R in TX path)   */
    SKY66423_MODE_BYPASS,       /* Bypass PA and LNA (test/calibration)  */
} sky66423_mode_t;

/* -------------------------------------------------------------------------
 * Driver handle
 * ---------------------------------------------------------------------- */
typedef struct {
    sky66423_hal_t  hal;
    sky66423_mode_t mode;
} sky66423_t;

/* -------------------------------------------------------------------------
 * API
 * ---------------------------------------------------------------------- */

/*
 * sky66423_init — Initialise the driver handle and assert shutdown.
 *
 * Call this before any other sky66423_* function. The FEM will be held in
 * shutdown until sky66423_set_mode() is called.
 */
sky66423_err_t sky66423_init(sky66423_t *dev, const sky66423_hal_t *hal);

/*
 * sky66423_set_mode — Set the operating mode of the SKY66423-11.
 *
 * Mode transitions complete within ~3 µs (typical). The function inserts a
 * short settling delay after changing mode.
 */
sky66423_err_t sky66423_set_mode(sky66423_t *dev, sky66423_mode_t mode);

/*
 * sky66423_get_mode — Return the current operating mode.
 */
sky66423_mode_t sky66423_get_mode(const sky66423_t *dev);

/*
 * sky66423_shutdown — Convenience wrapper to assert full shutdown.
 */
static inline sky66423_err_t sky66423_shutdown(sky66423_t *dev)
{
    return sky66423_set_mode(dev, SKY66423_MODE_SHUTDOWN);
}

/*
 * sky66423_enable_rx — Convenience wrapper to switch to RX mode.
 */
static inline sky66423_err_t sky66423_enable_rx(sky66423_t *dev)
{
    return sky66423_set_mode(dev, SKY66423_MODE_RX);
}

/*
 * sky66423_enable_tx — Convenience wrapper to switch to TX mode.
 *
 * Note: When CTX is wired to the SI4463 GPIO0 (TX_STATE), the hardware
 * automatically manages TX/RX switching. Calling this function is only
 * necessary when CTX is controlled by the MCU (hal.ctx_write != NULL).
 */
static inline sky66423_err_t sky66423_enable_tx(sky66423_t *dev)
{
    return sky66423_set_mode(dev, SKY66423_MODE_TX);
}

#ifdef __cplusplus
}
#endif

#endif /* SKY66423_H */
