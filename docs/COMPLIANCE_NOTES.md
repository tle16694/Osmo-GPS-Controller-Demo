# COMPLIANCE NOTES (Bluetooth / Regulatory / Trademark)

This document is a high-level productization checklist and is not legal advice.

## Bluetooth / RF Regulatory
- If you sell a product containing a radio (ESP32), you must comply with the destination country's requirements (e.g., FCC (US), CE/RED (EU), UKCA (UK), MIC (JP), NCC (TW), etc.).
- Even when using an ESP‑WROOM‑32 module with modular approval, the final product (enclosure, antenna environment, PCB/layout, power supply, shielding) may still require additional testing/certification.
- Maintain required user documentation/labels/warnings and keep EMI/EMC test records as applicable.
- If you want to use the Bluetooth® name/logo in marketing, follow Bluetooth SIG requirements (e.g., qualification/listing) appropriate for your product category.

## Data Security / Pairing
- The camera-side confirmation/verification flow (verify_mode) is part of the pairing workflow; make sure your user guide explains it clearly.
- Document a clear factory-reset policy (how to clear pairing/bonding data, and what the user should expect).

## Trademark / Branding (DJI)
- This product is not an official DJI product and is not endorsed/certified by DJI.
- Avoid marketing language that implies "Official DJI" or "DJI‑certified".
- If you reference "DJI", "Osmo", or "Osmo Action", do so only for compatibility purposes and include appropriate trademark disclaimers in documentation/packaging.
