/*
 * si4463_defs.h — SI4463 register map, command codes, and field definitions.
 *
 * References:
 *   Silicon Labs SI4463 API Description Rev 0.5
 *   Silicon Labs AN633 — Programming Guide for EZRadioPRO
 */

#ifndef SI4463_DEFS_H
#define SI4463_DEFS_H

#include <stdint.h>

/* -------------------------------------------------------------------------
 * API command codes
 * ---------------------------------------------------------------------- */
#define SI4463_CMD_NOP                  0x00
#define SI4463_CMD_PART_INFO            0x01
#define SI4463_CMD_POWER_UP             0x02
#define SI4463_CMD_PATCH_IMAGE          0x04
#define SI4463_CMD_FUNC_INFO            0x10
#define SI4463_CMD_SET_PROPERTY         0x11
#define SI4463_CMD_GET_PROPERTY         0x12
#define SI4463_CMD_GPIO_PIN_CFG         0x13
#define SI4463_CMD_GET_ADC_READING      0x14
#define SI4463_CMD_FIFO_INFO            0x15
#define SI4463_CMD_GET_INT_STATUS       0x20
#define SI4463_CMD_GET_PH_STATUS        0x21
#define SI4463_CMD_GET_MODEM_STATUS     0x22
#define SI4463_CMD_GET_CHIP_STATUS      0x23
#define SI4463_CMD_START_TX             0x31
#define SI4463_CMD_START_RX             0x32
#define SI4463_CMD_REQUEST_DEVICE_STATE 0x33
#define SI4463_CMD_CHANGE_STATE         0x34
#define SI4463_CMD_READ_CMD_BUFF        0x44
#define SI4463_CMD_WRITE_TX_FIFO        0x66
#define SI4463_CMD_READ_RX_FIFO         0x77
#define SI4463_CMD_IRCAL                0x17
#define SI4463_CMD_PROTOCOL_CFG         0x18
#define SI4463_CMD_START_TX_TUNE        0x35
#define SI4463_CMD_START_RX_TUNE        0x36
#define SI4463_CMD_RX_HOP               0x37

/* CTS (clear to send) value returned in the command buffer */
#define SI4463_CTS_READY                0xFF

/* -------------------------------------------------------------------------
 * Property group identifiers
 * ---------------------------------------------------------------------- */
#define SI4463_PROP_GLOBAL              0x00
#define SI4463_PROP_INT_CTL             0x01
#define SI4463_PROP_FRR_CTL             0x02
#define SI4463_PROP_PREAMBLE            0x10
#define SI4463_PROP_SYNC                0x11
#define SI4463_PROP_PKT                 0x12
#define SI4463_PROP_MODEM               0x20
#define SI4463_PROP_MODEM_CHFLT         0x21
#define SI4463_PROP_PA                  0x22
#define SI4463_PROP_SYNTH               0x23
#define SI4463_PROP_MATCH               0x30
#define SI4463_PROP_FREQ_CONTROL        0x40
#define SI4463_PROP_RX_HOP              0x50

/* -------------------------------------------------------------------------
 * Selected property addresses (group | offset)
 * ---------------------------------------------------------------------- */

/* Global */
#define SI4463_PROP_GLOBAL_XO_TUNE          0x0000
#define SI4463_PROP_GLOBAL_CLK_CFG          0x0001
#define SI4463_PROP_GLOBAL_LOW_BATT_THRESH  0x0002
#define SI4463_PROP_GLOBAL_CONFIG           0x0003
#define SI4463_PROP_GLOBAL_WUT_CONFIG       0x0004

/* Interrupt control */
#define SI4463_PROP_INT_CTL_ENABLE          0x0100
#define SI4463_PROP_INT_CTL_PH_ENABLE       0x0101
#define SI4463_PROP_INT_CTL_MODEM_ENABLE    0x0102
#define SI4463_PROP_INT_CTL_CHIP_ENABLE     0x0103

/* Preamble */
#define SI4463_PROP_PREAMBLE_TX_LENGTH      0x1000
#define SI4463_PROP_PREAMBLE_CONFIG_STD_1   0x1001
#define SI4463_PROP_PREAMBLE_CONFIG_NSTD    0x1002
#define SI4463_PROP_PREAMBLE_CONFIG_STD_2   0x1003
#define SI4463_PROP_PREAMBLE_CONFIG         0x1004

/* Sync word */
#define SI4463_PROP_SYNC_CONFIG             0x1100
#define SI4463_PROP_SYNC_BITS_31_24         0x1101
#define SI4463_PROP_SYNC_BITS_23_16         0x1102
#define SI4463_PROP_SYNC_BITS_15_8          0x1103
#define SI4463_PROP_SYNC_BITS_7_0           0x1104

/* PA */
#define SI4463_PROP_PA_MODE                 0x2200
#define SI4463_PROP_PA_PWR_LVL              0x2201
#define SI4463_PROP_PA_BIAS_CLKDUTY         0x2202
#define SI4463_PROP_PA_TC                   0x2203

/* Frequency control */
#define SI4463_PROP_FREQ_CONTROL_INTE       0x4000
#define SI4463_PROP_FREQ_CONTROL_FRAC_2     0x4001
#define SI4463_PROP_FREQ_CONTROL_FRAC_1     0x4002
#define SI4463_PROP_FREQ_CONTROL_FRAC_0     0x4003
#define SI4463_PROP_FREQ_CONTROL_CHANNEL_STEP_SIZE_1 0x4004
#define SI4463_PROP_FREQ_CONTROL_CHANNEL_STEP_SIZE_0 0x4005
#define SI4463_PROP_FREQ_CONTROL_W_SIZE     0x4006
#define SI4463_PROP_FREQ_CONTROL_VCOCNT_RX_ADJ 0x4007

/* Modem */
#define SI4463_PROP_MODEM_MOD_TYPE          0x2000
#define SI4463_PROP_MODEM_MAP_CONTROL       0x2001
#define SI4463_PROP_MODEM_DSM_CTRL          0x2002
#define SI4463_PROP_MODEM_DATA_RATE_2       0x2003
#define SI4463_PROP_MODEM_DATA_RATE_1       0x2004
#define SI4463_PROP_MODEM_DATA_RATE_0       0x2005
#define SI4463_PROP_MODEM_TX_NCO_MODE_3     0x2006
#define SI4463_PROP_MODEM_TX_NCO_MODE_2     0x2007
#define SI4463_PROP_MODEM_TX_NCO_MODE_1     0x2008
#define SI4463_PROP_MODEM_TX_NCO_MODE_0     0x2009
#define SI4463_PROP_MODEM_FREQ_DEV_2        0x200A
#define SI4463_PROP_MODEM_FREQ_DEV_1        0x200B
#define SI4463_PROP_MODEM_FREQ_DEV_0        0x200C

/* -------------------------------------------------------------------------
 * POWER_UP boot options
 * ---------------------------------------------------------------------- */
#define SI4463_POWER_UP_FUNC_PRO        0x01  /* EZRadioPRO */
#define SI4463_POWER_UP_XTAL            0x00  /* XTAL oscillator */
#define SI4463_POWER_UP_TCXO            0x01  /* TCXO input */

/* -------------------------------------------------------------------------
 * Device states (REQUEST_DEVICE_STATE / CHANGE_STATE)
 * ---------------------------------------------------------------------- */
#define SI4463_STATE_NOCHANGE           0x00
#define SI4463_STATE_SLEEP              0x01
#define SI4463_STATE_SPI_ACTIVE         0x02
#define SI4463_STATE_READY              0x03
#define SI4463_STATE_READY2             0x04
#define SI4463_STATE_TX_TUNE            0x05
#define SI4463_STATE_RX_TUNE            0x06
#define SI4463_STATE_TX                 0x07
#define SI4463_STATE_RX                 0x08

/* -------------------------------------------------------------------------
 * GPIO pin function codes (used with GPIO_PIN_CFG command)
 * ---------------------------------------------------------------------- */
#define SI4463_GPIO_MODE_DONOTHING      0x00
#define SI4463_GPIO_MODE_TRISTATE       0x01
#define SI4463_GPIO_MODE_DRIVE0         0x02
#define SI4463_GPIO_MODE_DRIVE1         0x03
#define SI4463_GPIO_MODE_INPUT          0x04
#define SI4463_GPIO_MODE_32K_CLK        0x05
#define SI4463_GPIO_MODE_TX_STATE       0x11  /* High during TX */
#define SI4463_GPIO_MODE_RX_STATE       0x12  /* High during RX */
#define SI4463_GPIO_MODE_RX_FIFO_FULL   0x21
#define SI4463_GPIO_MODE_TX_FIFO_EMPTY  0x22
#define SI4463_GPIO_MODE_CCA            0x29

/* -------------------------------------------------------------------------
 * Modem modulation types
 * ---------------------------------------------------------------------- */
#define SI4463_MOD_TYPE_CW              0x00
#define SI4463_MOD_TYPE_OOK             0x01
#define SI4463_MOD_TYPE_2FSK            0x02
#define SI4463_MOD_TYPE_2GFSK           0x03
#define SI4463_MOD_TYPE_4FSK            0x04
#define SI4463_MOD_TYPE_4GFSK           0x05

/* -------------------------------------------------------------------------
 * Interrupt status bit masks (GET_INT_STATUS reply byte 0)
 * ---------------------------------------------------------------------- */
#define SI4463_INT_STATUS_CHIP_INT      (1u << 2)
#define SI4463_INT_STATUS_MODEM_INT     (1u << 1)
#define SI4463_INT_STATUS_PH_INT        (1u << 0)

/* Packet handler interrupt pending bits */
#define SI4463_PH_STATUS_FILTER_MATCH   (1u << 7)
#define SI4463_PH_STATUS_FILTER_MISS    (1u << 6)
#define SI4463_PH_STATUS_PACKET_SENT    (1u << 5)
#define SI4463_PH_STATUS_PACKET_RX      (1u << 4)
#define SI4463_PH_STATUS_CRC_ERROR      (1u << 3)
#define SI4463_PH_STATUS_TX_FIFO_ALMOST_EMPTY (1u << 1)
#define SI4463_PH_STATUS_RX_FIFO_ALMOST_FULL  (1u << 0)

/* -------------------------------------------------------------------------
 * FIFO sizes
 * ---------------------------------------------------------------------- */
#define SI4463_TX_FIFO_SIZE             64
#define SI4463_RX_FIFO_SIZE             64

#endif /* SI4463_DEFS_H */
