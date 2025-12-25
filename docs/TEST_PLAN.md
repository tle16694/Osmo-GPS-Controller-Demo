# TEST PLAN – Production / End‑of‑Line Checklist

Use this checklist for production validation (EOL / QA).

## 1) Smoke Test
1. Flash succeeds; device boots without reset loops.
2. LED shows BOOT: ON ~800 ms, OFF ~200 ms (one time).
3. When not connected: LED shows READY (ON 120 ms / OFF 880 ms).

## 2) Pair / Connect
4. Long press (>= 2.0 s): LED shows CONNECTING (80/120 ms).
5. Camera accepts the connection and LED becomes CONNECTED (solid ON).

## 3) Command Verification
6. Single click: start recording; single click again: stop recording (LED must show RECORDING while recording).
7. Double click: MODE_NEXT (camera switches Quick Switch / mode cycle).
8. Triple click: TAKE_PHOTO (camera captures a photo).

## 4) Reconnect & Reset
9. Reboot the remote: it must auto-reconnect to the last camera (with the camera powered on nearby).
10. Very long press (>= 7.0 s): clears bonding info and forces re-pairing (LED returns to CONNECTING).

## 5) Power Behavior
- Leave it idle (not connected) for 5 minutes: remote must enter light sleep and wake on button press.
