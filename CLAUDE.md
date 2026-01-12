# Parking Pass Display - ESP32 E-Ink

## Project Overview
ESP32-based e-ink display for showing Toronto parking permit barcode. Syncs via BLE from Android app.

## Build Commands

### Heltec Vision Master E290 (production)
```bash
pio run -e vision_e290 --target upload
pio device monitor
```

### Standard ESP32 (testing, no display)
```bash
pio run -e esp32dev --target upload
pio device monitor
```

## Hardware
- **Board**: Heltec Vision Master E290 (ESP32-S3 + 2.9" e-ink)
- **Button**: GPIO 21 (short press = BLE sync, long press 3s = force refresh)
- **LED**: GPIO 45
- **Display**: 296x128 e-ink

## Key Files
- `src/main.cpp` - Main firmware with BLE client and display logic
- `src/Code39Generator.h` - Barcode rendering for Code39 format
- `src/bluetooth_helper.h` - BLE connection and data transfer
- `src/permit_config.h` - Permit data structures

## BLE Protocol
- ESP32 acts as BLE client, phone as server
- Service UUID: `12345678-1234-5678-1234-56789abcdef0`
- Permit Characteristic: `12345678-1234-5678-1234-56789abcdef1`
- Sync Type Characteristic: `12345678-1234-5678-1234-56789abcdef2`

## Branches
- `main` / `ble-only` - Current BLE-only version
- `wifi-only` - Legacy WiFi version (pulls from GitHub)
- `permit` - Permit JSON data branch

## Notes
- Requires 3s USB CDC delay on ESP32-S3 for serial output
- Permit data persists in flash (Preferences library)
- Display can be flipped 180 via app setting
- E-ink retains image when powered off
