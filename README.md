# NarBot

NarBot is an experimental SMARS-style robot workspace. It currently contains
PlatformIO firmware for ESP32 camera/control nodes plus a KiCad PCB project.

The active firmware work is focused on getting reliable ESP32 camera streams,
basic Wi-Fi setup, and simple robot-control building blocks working before
adding a larger autonomous control stack.

## Projects

### `NarbotESP`

Minimal dual-camera firmware for two Seeed Studio XIAO ESP32S3 Sense boards.
The same codebase builds in two modes:

- `main_esp`: starts the `SMARS_MAIN` access point at `192.168.4.1`, serves the
  dashboard, and streams its local camera.
- `front_esp`: joins `SMARS_MAIN` as a station, tries to use static IP
  `192.168.4.10`, and serves its own camera stream.

Main endpoints:

- `http://192.168.4.1/` - dashboard hosted by the main ESP
- `/status` - JSON status
- `/jpg` - single JPEG frame
- `/stream` - MJPEG stream

Default Wi-Fi credentials used by the prototype:

- SSID: `SMARS_MAIN`
- Password: `smars1234`

Build and upload:

```sh
cd NarbotESP
make compile
make upload_main
make upload_front
make monitor
```

Equivalent PlatformIO commands:

```sh
pio run -e main_esp -t upload
pio run -e front_esp -t upload
pio device monitor
```

### `CameraDriver`

Experimental all-in-one XIAO ESP32S3 Sense firmware. This project currently
contains motor-control, camera capture/streaming, setup AP, simple web UI,
serial console, headlight/status LED behavior, and color-sensor code.

Hardware assumptions in the current firmware include:

- Seeed Studio XIAO ESP32S3 Sense with camera
- DFRobot TB6612FNG motor driver
- TCS34725-compatible color sensor at I2C address `0x29`

Setup AP used by the prototype:

- SSID: `CameraDriver-Setup`
- Password: `smars1234`

Build and upload:

```sh
cd CameraDriver
make compile
make upload
make monitor
```

### `NarbotPCB`

KiCad PCB project for the robot electronics.

Commit these files:

- `*.kicad_pro`
- `*.kicad_sch`
- `*.kicad_pcb`

Local KiCad backups and project-local state are ignored by
`NarbotPCB/.gitignore`.

### `ColorCalibration`

Placeholder/calibration workspace. At the moment only generated PlatformIO
output was present, so `.pio/` is ignored there.

## Repository Layout

```text
.
├── CameraDriver/      # Experimental ESP32 camera + motor driver firmware
├── ColorCalibration/  # Calibration workspace
├── NarbotESP/         # Two-ESP camera streaming firmware
├── NarbotPCB/         # KiCad board project
└── README.md
```

## Requirements

- PlatformIO CLI (`pio`)
- USB serial access to the ESP32 boards
- KiCad for the PCB project

The firmware targets ESP32 boards through PlatformIO's `espressif32` platform
and uses the Arduino framework.

## Git Notes

Each project has its own `.gitignore` for generated files. Build output,
downloaded PlatformIO dependencies, clangd caches, notebook scratch files,
macOS metadata, and KiCad backups should stay uncommitted.

Useful status check:

```sh
git status --short --untracked-files=all
```

## Current Milestone

The main firmware milestone is:

1. Main ESP starts the `SMARS_MAIN` access point.
2. Main dashboard opens at `http://192.168.4.1/`.
3. Main ESP camera stream is visible.
4. Front ESP connects to the main AP at `192.168.4.10`.
5. Front ESP stream appears in the main dashboard.
