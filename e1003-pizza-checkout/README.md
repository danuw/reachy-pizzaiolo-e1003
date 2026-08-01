# E1003 Pizza Checkout Firmware

Firmware project for Seeed Studio reTerminal E1003 that renders a pizza checkout UI and exposes a local HTTP JSON API for backend-driven updates.

## Hardware

- Seeed Studio reTerminal E1003
- ESP32-S3
- 10.3 inch ePaper display
- GT911 touch controller (expected)
- Wi-Fi network access

## Software Requirements

- Python 3.10+
- PlatformIO Core
- USB serial driver for ESP32-S3

## PlatformIO Setup (Virtual Environment)

Windows PowerShell:

```powershell
python -m venv .venv
.\.venv\Scripts\Activate.ps1
python -m pip install --upgrade pip
python -m pip install platformio
```

macOS/Linux:

```bash
python3 -m venv .venv
source .venv/bin/activate
python -m pip install --upgrade pip
python -m pip install platformio
```

## Wi-Fi Configuration

1. Copy `src/secrets.example.h` to `src/secrets.h`.
2. Set your values:
   - `WIFI_SSID`
   - `WIFI_PASSWORD`
   - `API_TOKEN`

`src/secrets.h` is ignored by git.

## Build and Upload

```bash
pio run
pio run -t upload
pio device monitor -b 115200
```

## API Token

- Write endpoints require: `X-Api-Token: <API_TOKEN>`
- Read endpoints do not require a token in this MVP.

## API Endpoints

- `GET /api/health`
- `GET /api/state`
- `GET /api/events/latest`
- `POST /api/config`
- `POST /api/basket`
- `POST /api/right-panel`
- `POST /api/customisation`
- `POST /api/screen`
- `POST /api/refresh`
- `POST /api/clear`

## Sample curl Commands

```bash
curl http://pizza-display.local/api/health
```

```bash
curl -X POST http://pizza-display.local/api/basket \
  -H "Content-Type: application/json" \
  -H "X-Api-Token: dev-secret" \
  -d @samples/sample-basket.json
```

```bash
curl -X POST http://pizza-display.local/api/right-panel \
  -H "Content-Type: application/json" \
  -H "X-Api-Token: dev-secret" \
  -d @samples/sample-right-panel.json
```

```bash
curl -X POST http://pizza-display.local/api/screen \
  -H "Content-Type: application/json" \
  -H "X-Api-Token: dev-secret" \
  -d @samples/sample-screen.json
```

```bash
curl http://pizza-display.local/api/events/latest
```

## Touch Callback

- Configure callback URL with `POST /api/config`.
- On button press, firmware posts:
  - `deviceId`
  - `event=button_pressed`
  - `screenId`
  - `buttonId`
  - `label`
  - `timestampMs`
- If callback fails, latest local event remains available via `GET /api/events/latest`.

## Troubleshooting

- If Wi-Fi drops, firmware retries connection in the main loop.
- If touch is not active yet, use serial simulation:
  - send: `t 1200 900`
- If display init fails with grayscale mode, remove/adjust `initGrayMode` for your installed Seeed_GFX version.

## Known Limitations

- GT911 polling is a placeholder and should be replaced with Seeed E1003 touch example integration.
- No payment, speech, or menu logic in firmware.
- No external image downloads in MVP.
