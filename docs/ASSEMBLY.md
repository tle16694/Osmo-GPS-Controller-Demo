# ASSEMBLY – ESP32 DevKit (ESP‑WROOM‑32) One‑Button Remote

This document describes wiring, basic hardware precautions, and enclosure notes for a sellable single-button BLE remote build.

## Recommended Parts
- Board: ESP32 DevKit (ESP‑WROOM‑32)
- Momentary push button (NO)
- 1x LED (any color) + 330Ω series resistor (220–470Ω depending on desired brightness)
- 5V via USB, or a battery system with a suitable regulator (per your DevKit)

## Wiring (Firmware Pinout)
### Button (Active‑Low)
- `GPIO27` ↔ button ↔ `GND`
- Firmware enables the internal pull‑up (no external pull‑up resistor required)

### Status LED (Active‑High)
- `GPIO33` → 330Ω resistor → LED(+)  
- LED(−) → `GND`

## Hardware Notes
- Do not use the DevKit BOOT button for UI; keep `BOOT/IO0` for flashing only.
- Avoid wiring that can pull ESP32 strapping pins to the wrong level at boot. (`GPIO27`/`GPIO33` are typically safe on common DevKits.)
- If the button wire is long (>20–30 cm), consider adding a 100 nF capacitor close to `GPIO27`–`GND` to reduce noise. (Firmware already includes debounce.)

## Enclosure Notes
- Provide access to the USB port for flashing/firmware updates.
- Use a light pipe, small window, or diffuser if you need to reduce LED glare.
- Secure the board mechanically to prevent flexing that could stress wires on `GPIO27`/`GPIO33`.
