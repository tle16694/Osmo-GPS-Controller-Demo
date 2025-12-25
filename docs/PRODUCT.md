# DJI Osmo Action 5 Pro - Single-Button BLE Remote (ESP32 DevKit)

This firmware is a hardened port of DJI's official demo into a sellable single-button BLE remote for ESP32 DevKit (ESP-WROOM-32): first-run pairing, automatic reconnect, full single-button command mapping, and power-friendly behavior.

## Key Features
- BLE + DJI R SDK protocol control
- Single-button UX: record toggle, mode next (quick switch), take photo, pair/reconnect, factory reset link
- Stores last bonded camera info in NVS and auto-reconnects on boot
- No GNSS hardware required (GNSS disabled by default on ESP32)
- Single status LED (GPIO33) with explicit patterns
- If not connected and no user input for 5 minutes, enters light sleep and wakes on the button

## Single-Button UX Mapping
Multi-click finalize window: 380ms after the last button release.

- Single click: RECORD_TOGGLE
- Double click: MODE_NEXT (Quick Switch / cycle)
- Triple click: TAKE_PHOTO
- Long press >= 2.0s: PAIR_OR_RECONNECT
- Very long press >= 7.0s: FACTORY_RESET_LINK (clear bond + force re-pair)

## Status LED Patterns (GPIO33, single LED)
- BOOT: 800ms ON then 200ms OFF once
- READY (not connected): 120ms ON / 880ms OFF
- CONNECTING: 80ms ON / 120ms OFF
- CONNECTED (protocol ready): solid ON
- RECORDING: 180ms ON / 820ms OFF
- ERROR: 70/70ms triple blink then 700ms pause

## First-Time Pairing
1) Power on the camera and enable its BLE/remote-control feature.
2) Press and hold the remote button for >= 2.0s.
3) LED enters CONNECTING; the camera may prompt for confirmation depending on pairing state.
4) On success, LED becomes solid ON (CONNECTED).

## Auto-Reconnect
- If previously paired: the remote attempts to reconnect to the last bonded camera on boot.
- If reconnect fails: it falls back to scanning and connecting to the nearest compatible camera.

## Factory Reset Link
Press and hold the button for >= 7.0s to:
- Clear bonded camera info in NVS
- Force a fresh pairing flow (CONNECTING)

## Notes
- The ESP32 DevKit BOOT button is not used for UI (BOOT remains for flashing only).
- If RECORD_TOGGLE is pressed while in photo mode, firmware attempts to switch to video then start recording.
- TAKE_PHOTO: if a direct shutter report fails, firmware switches to photo mode and retries.
