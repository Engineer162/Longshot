# Longshot

Longshot is a "legal" long range RF module that combines the **Silicon Labs SI4463** transceiver with the **Skyworks SKY66423-11** front-end module (FEM) to form a complete RF system. It is designed to operate within regulatory limits and can be used with omni-directional or directional antennas.

---

## Overview

| Component | Part | Role |
|-----------|------|------|
| Transceiver | Silicon Labs SI4463 | Sub-GHz radio modem (119–1050 MHz), SPI interface |
| Front-End Module | Skyworks SKY66423-11 | Integrated PA + LNA + T/R switch (169–960 MHz) |

Together, these ICs provide a complete, single-board RF path from baseband to antenna with extended range while staying within jurisdictional power limits (e.g., FCC Part 15, ETSI EN 300 220).

---

## Features

- **Long Range** – The integrated PA in the SKY66423-11 boosts the SI4463 output to the maximum legal EIRP for your region.
- **Legal Operation** – Output power is configurable and tested to comply with ISM-band regulations.
- **Flexible Antenna Support** – Standard SMA connector; supports omni-directional whip antennas and directional Yagi/patch antennas.
- **Low-Power Modes** – Hardware shutdown pin on the SKY66423-11 enables micro-amp standby current.
- **SPI Control** – The SI4463 is fully controlled over a 4-wire SPI bus.
- **Automatic T/R Switching** – The SKY66423-11 performs automatic TX/RX switching, driven directly by the SI4463 GPIO.

---

## Hardware

### Pin Connections

| Signal | SI4463 Pin | SKY66423-11 Pin | MCU / Host |
|--------|-----------|-----------------|------------|
| SPI SCK | SCLK | — | SPI CLK |
| SPI MOSI | SDI | — | SPI MOSI |
| SPI MISO | SDO | — | SPI MISO |
| SPI CS | NSEL | — | GPIO (active low) |
| Shutdown | — | CSD | GPIO (active high) |
| TX Enable | GPIO0 | CTX | Internal connection |
| RF Output | TX/RX | RFIN/RFOUT | — |
| Interrupt | NIRQ | — | GPIO (active low) |
| Reset | SDN | — | GPIO (active high) |

### Frequency Bands

The Longshot module supports the following ISM bands (hardware-dependent on antenna and filter selection):

| Band | Frequency | Common Regulations |
|------|-----------|-------------------|
| 169 MHz | 169.4–169.475 MHz | ETSI EN 300 220 |
| 433 MHz | 433.05–434.79 MHz | ITU Region 1 ISM |
| 868 MHz | 863–870 MHz | ETSI EN 300 220 |
| 915 MHz | 902–928 MHz | FCC Part 15.247 |

---

## Firmware

The `firmware/` directory contains a portable C library that drives both the SI4463 and SKY66423-11 and exposes a simple Longshot API.

### Directory Structure

```
firmware/
├── src/
│   ├── longshot.h        # Public Longshot API
│   ├── longshot.c        # Longshot module implementation
│   ├── si4463.h          # SI4463 driver header
│   ├── si4463.c          # SI4463 driver implementation
│   ├── si4463_defs.h     # SI4463 register and command definitions
│   ├── sky66423.h        # SKY66423-11 driver header
│   └── sky66423.c        # SKY66423-11 driver implementation
└── examples/
    ├── basic_transmit.c  # Simple packet transmit example
    └── basic_receive.c   # Simple packet receive example
```

### Quick Start

1. Copy `firmware/src/` into your project.
2. Implement the four HAL functions declared in `longshot.h` for your platform (SPI transfer, GPIO write, GPIO read, delay).
3. Call `longshot_init()`, then `longshot_transmit()` or `longshot_receive()`.

```c
#include "longshot.h"

/* Implement platform HAL (see longshot.h) */

int main(void)
{
    longshot_config_t cfg = LONGSHOT_DEFAULT_CONFIG;
    cfg.frequency_hz      = 915000000UL;  /* 915 MHz */
    cfg.tx_power_dbm      = 14;           /* Legal limit for your region */

    longshot_init(&cfg);

    uint8_t payload[] = "Hello, World!";
    longshot_transmit(payload, sizeof(payload));
}
```

---

## Regulatory Compliance

Longshot is designed to operate as an intentional radiator within the limits set by relevant authorities. Users are responsible for ensuring that their specific antenna gain and cabling losses result in a total EIRP that does not exceed the regulatory maximum for their jurisdiction and frequency band.

| Region | Band | Max EIRP |
|--------|------|---------|
| USA (FCC Part 15.247) | 915 MHz | 30 dBm (1 W) |
| Europe (ETSI EN 300 220) | 868 MHz | 14 dBm (25 mW) |
| Europe (ETSI EN 300 220) | 433 MHz | 10 dBm (10 mW) |

---

## License

This project is released under the MIT License. See [LICENSE](LICENSE) for details.