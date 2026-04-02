/*
 * sky66423.c — Skyworks SKY66423-11 front-end module driver implementation.
 */

#include "sky66423.h"
#include <string.h>

/* Settling time after a mode change (µs) */
#define SKY66423_SETTLE_US  5u

sky66423_err_t sky66423_init(sky66423_t *dev, const sky66423_hal_t *hal)
{
    if (!dev || !hal) {
        return SKY66423_ERR_PARAM;
    }
    if (!hal->csd_write || !hal->delay_us) {
        return SKY66423_ERR_PARAM;
    }

    memcpy(&dev->hal, hal, sizeof(sky66423_hal_t));
    dev->mode = SKY66423_MODE_SHUTDOWN;

    /* Assert shutdown immediately */
    dev->hal.csd_write(0);
    if (dev->hal.cps_write) {
        dev->hal.cps_write(0);
    }
    if (dev->hal.ctx_write) {
        dev->hal.ctx_write(0);
    }

    return SKY66423_OK;
}

sky66423_err_t sky66423_set_mode(sky66423_t *dev, sky66423_mode_t mode)
{
    if (!dev) {
        return SKY66423_ERR_PARAM;
    }

    switch (mode) {
    case SKY66423_MODE_SHUTDOWN:
        /* CSD low: full shutdown */
        dev->hal.csd_write(0);
        if (dev->hal.cps_write) {
            dev->hal.cps_write(0);
        }
        if (dev->hal.ctx_write) {
            dev->hal.ctx_write(0);
        }
        break;

    case SKY66423_MODE_RX:
        /*
         * CSD high, CPS low, CTX low.
         * If CTX is wired to the SI4463 GPIO0 (TX_STATE), the radio will
         * hold CTX low while in RX mode automatically.
         */
        if (dev->hal.cps_write) {
            dev->hal.cps_write(0);
        }
        if (dev->hal.ctx_write) {
            dev->hal.ctx_write(0);
        }
        dev->hal.csd_write(1);
        break;

    case SKY66423_MODE_TX:
        /*
         * CSD high, CPS low, CTX high.
         * If CTX is wired to SI4463 GPIO0, the radio drives CTX high
         * automatically when transmitting. MCU control of CTX is optional.
         */
        if (dev->hal.cps_write) {
            dev->hal.cps_write(0);
        }
        dev->hal.csd_write(1);
        if (dev->hal.ctx_write) {
            dev->hal.ctx_write(1);
        }
        break;

    case SKY66423_MODE_BYPASS:
        /*
         * CSD high, CPS high: bypass PA and LNA.
         * Used for conducted testing or calibration.
         */
        dev->hal.csd_write(1);
        if (dev->hal.cps_write) {
            dev->hal.cps_write(1);
        }
        if (dev->hal.ctx_write) {
            dev->hal.ctx_write(0);
        }
        break;

    default:
        return SKY66423_ERR_PARAM;
    }

    dev->mode = mode;
    dev->hal.delay_us(SKY66423_SETTLE_US);
    return SKY66423_OK;
}

sky66423_mode_t sky66423_get_mode(const sky66423_t *dev)
{
    if (!dev) {
        return SKY66423_MODE_SHUTDOWN;
    }
    return dev->mode;
}
