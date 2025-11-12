# GateKeeper — Smart Car Park Automation

GateKeeper blends edge computing, computer vision, and embedded control to manage vehicle access automatically. A Raspberry Pi orchestrates license-plate capture and recognition, an STM32 microcontroller drives the physical barrier, and operators manage authorized vehicles through a web dashboard and REST API.

## Key Capabilities

- **Real-time detection and OCR** powered by a YOLOv8 plate detector and PaddleOCR reader with CPU-friendly defaults.@src/core/detector.py#1-75@src/core/ocr_reader.py#1-119
- **Edge-to-cloud workflow** where the Raspberry Pi listens for STM32 events, captures frames, and decides barrier actions over UART.@src/hardware/smart_car_park.py#1-389
- **Operator tooling** including a Flask dashboard for managing plate records and viewing recent movements.@src/web/app.py#55-247
- **Self-contained recognition API** exposing camera-triggered and file-upload endpoints for integration or testing.@src/api/main.py#1-140

## System Architecture

| Layer | Responsibility | Primary Artifacts |
| --- | --- | --- |
| Embedded control | Detect vehicles, position barrier, display status | `firmware/stm32_main.c`, UART protocol (see `docs/README.md`) |
| Edge orchestration | Capture frames, run ML pipeline, query SQLite, answer STM32 | `src/hardware/smart_car_park.py`@src/hardware/smart_car_park.py#62-389 |
| Vision core | YOLOv8 detection, PaddleOCR recognition, text parsing utilities | `src/core/detector.py`, `src/core/ocr_reader.py`, `src/core/parser.py` |
| Services | FastAPI recognition API, Flask management UI | `src/api/main.py`, `src/web/app.py` |
| Data & config | Models, SQLite DB, logging, settings | `models/`, `data/`, `config/settings.py`@config/settings.py#11-49 |

## Repository Layout

```
GateKeeper/
├── src/
│   ├── api/                # FastAPI service
│   ├── web/                # Flask dashboard (templates & static assets)
│   ├── hardware/           # Raspberry Pi orchestration script
│   └── core/               # Detection, OCR, parsing logic
├── firmware/               # STM32 firmware sources
├── models/                 # YOLO weights and supporting assets
├── data/                   # Databases and log output
├── scripts/                # Utilities (e.g., DB initialization)
├── docs/                   # Expanded hardware + protocol docs
├── tests/                  # Sample images and test harnesses
├── Dockerfile
└── docker-compose.yml
```

## Getting Started

### Prerequisites

- Python 3.9+
- libopencv bindings (`sudo apt install python3-opencv` on Debian/Ubuntu)
- (Optional) Docker + Docker Compose
- Hardware: Raspberry Pi 4/5, STM32F4, USB/UVC camera, LM393 sensor, SG90 servo

### Local Installation

```bash
# clone and enter repo
git clone <repository-url>
cd GateKeeper

# create virtual environment (recommended)
python -m venv .venv
source .venv/bin/activate

# install dependencies
pip install -r requirements.txt

# initialize SQLite schema
python scripts/setup_db.py
```

### Run Services

```bash
# launch FastAPI recognition service
python -m src.api.main

# in another terminal: launch Flask dashboard
python -m src.web.app
```

Services listen on `http://localhost:8000` (API) and `http://localhost:5000` (web UI) by default.@src/api/main.py#13-17@src/web/app.py#27-60

### Docker Deployment

```bash
docker-compose up -d
```

The compose stack builds a unified image, mounts the shared `data/` and `models/` directories, and exposes the API on port `8000` and dashboard on `5000` for local access.@docker-compose.yml#3-38

## Configuration

Environment variables override defaults defined in `config/settings.py`.@config/settings.py#11-45

| Variable | Default | Purpose |
| --- | --- | --- |
| `API_HOST` / `API_PORT` | `0.0.0.0` / `8000` | FastAPI bind address and port |
| `WEB_HOST` / `WEB_PORT` | `0.0.0.0` / `5000` | Flask dashboard bind address and port |
| `WEB_SECRET_KEY` | _unset_ | Session secret (must change for production) |
| `ADMIN_USERNAME` / `ADMIN_PASSWORD` | `admin` / `admin` | Dashboard credentials (change immediately) |
| `MODEL_PATH` | `models/best.pt` | YOLOv8 weights path |
| `UART_PORT` / `UART_BAUDRATE` | `/dev/ttyAMA0` / `115200` | STM32 UART link |
| `CAMERA_ID` / `CAMERA_WIDTH` / `CAMERA_HEIGHT` | `0` / `1280` / `720` | Capture device selection |
| `DETECTION_CONFIDENCE` | `0.3` | YOLO confidence threshold |

## Usage

### Web Dashboard

1. Open `http://localhost:5000`.
2. Authenticate with the configured admin credentials.
3. Manage plates (add/remove) and review recent movement logs from the dashboard views.@src/web/app.py#89-247

### Recognition API

- **Trigger camera capture** (requires a camera attached to the host running the API):
  ```bash
  curl http://localhost:8000/lpr
  ```
  Returns `{"plate": "<text>", "status": true}` when a plate is recognized.@src/api/main.py#62-110

- **Upload an image** (legacy endpoint retained for testing):
  ```bash
  curl -X POST http://localhost:8000/lpr/upload \
    -H "Content-Type: multipart/form-data" \
    -F "file=@/path/to/image.jpg"
  ```
  Response mirrors the camera endpoint on success.@src/api/main.py#112-138

### Edge Automation (Raspberry Pi)

Run the orchestration script on the Pi to bridge STM32 events, recognition, and barrier control:

```bash
python -m src.hardware.smart_car_park
```

The script establishes UART to the STM32, maintains a live camera feed, passes detections through YOLO/PaddleOCR, and writes decisions back to the controller.@src/hardware/smart_car_park.py#93-364

### Data & Logs

- SQLite database: `data/databases/car_park.db`
- Web logs: `data/logs/web_app.log`
- Edge runtime logs: timestamped files in `data/logs/`

## Hardware Quick Reference

| Component | Notes |
| --- | --- |
| STM32F4 MCU | Handles sensors, servo PWM, OLED feedback |
| Raspberry Pi 4/5 | Runs Python orchestration and services |
| LM393 IR sensor | Detects vehicle presence |
| SG90 servo | Drives entry barrier |
| SSD1306 OLED | Displays status messages |
| UVC-compatible camera | Captures license plates |

See `docs/README.md` for wiring diagrams, UART protocol details, and deployment checklists.

## Testing

```bash
python -m pytest tests/
```

Sample images in `tests/test_images/` help validate the detector/OCR pipeline.

## Troubleshooting

- No camera feed? Confirm `/dev/video*` permissions and the configured `CAMERA_ID`.
- UART silent? Verify `enable_uart=1` on the Pi and wiring to STM32.
- Recognition failures? Enable `DEBUG_IMAGES=true` before running the hardware script to inspect crops.@src/core/detector.py#67-74

## Security Checklist

- Rotate the default admin credentials and Flask secret key.
- Place services behind HTTPS and restrict network exposure.
- Harden the Pi/STM32 deployment (physical security, VLAN isolation, limited user accounts).

## Contributing

1. Fork the repository
2. Create a feature branch
3. Commit changes with context-rich messages
4. Submit a pull request with test results

## License & Acknowledgements

Educational use only—obtain appropriate approvals before deploying in production. Built atop Ultralytics YOLOv8, PaddleOCR, Flask, FastAPI, and community tooling.
