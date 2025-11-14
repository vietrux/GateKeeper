# GateKeeper

An intelligent gate system that automatically recognizes license plates, combining a FastAPI image-processing service with an ESP32 controller.

## Introduction

GateKeeper functions as an automated access-control system:

* **Central processing (FastAPI)**: Captures images from a camera, detects license plates using YOLOv8 and PaddleOCR, then decides whether to allow or deny access.
* **Device control (ESP32)**: Uses an LM393 sensor to detect vehicles, calls the API for verification, controls a servo to open the gate, and displays results on an OLED screen.

## Key Features

### Intelligent image processing

* Detect license plates using YOLOv8
* Recognize characters using PaddleOCR, optimized for Vietnamese plates
* Processes images directly from the camera, no manual interaction needed

### Fully automated operation

* LM393 sensor automatically detects incoming vehicles
* Calls API and processes the returned results
* Controls the servo to open/close the gate
* Displays real-time status on the OLED screen

### Easy deployment

* Fully containerized with Docker
* Flexible configuration via environment variables
* Supports multiple camera types

## How it works

```
Vehicle arrives → Sensor detects → ESP32 calls API
                                    ↓
                         API captures & recognizes image
                                    ↓
                         Returns result to ESP32
                                    ↓
       Open gate (if valid) & display status on screen
```

## Project Structure

```
GateKeeper/
├── src/
│   ├── api/
│   │   └── main.py              # Starts FastAPI and defines endpoints
│   └── core/
│       ├── detector.py          # License plate detection using YOLOv8
│       ├── ocr_reader.py        # Character recognition with PaddleOCR
│       └── parser.py            # Processing and normalizing license plates
├── firmware/
│   └── GateKeeper/
│       └── src/
│           └── main.cpp         # ESP32 code (WiFi, HTTP, servo, OLED)
├── config/
│   └── settings.py              # System configuration
├── models/
│   └── best.pt                  # YOLOv8 model (download separately)
├── docker-compose.yml           # Run service with Docker
├── Dockerfile                   # Build Docker image
└── requirements.txt             # Python dependencies
```

## Hardware Requirements

### For the API

* USB camera or webcam
* Computer/server running Docker (or Python 3.11+)
* Optional GPU (CPU works but slower)

### For the ESP32

* ESP32 Wroom 32 DevKit v1
* LM393 obstacle sensor
* SG90 servo
* SSD1306 128×64 OLED display (I²C)

## Installation Guide

### Step 1: Prepare Python environment

```bash
# Create virtual environment
python -m venv .venv

# Activate environment
source .venv/bin/activate   # Linux/Mac
# or
.venv\Scripts\activate      # Windows

# Install dependencies
pip install --upgrade pip
pip install -r requirements.txt
```

### Step 2: Download YOLOv8 model

Place the trained `best.pt` file inside the `models/` directory.

### Step 3: Run the API

**Option 1: Run directly**

```bash
python -m src.api.main
```

The server will start at `http://localhost:8000`
Visit `http://localhost:8000/docs` to view API documentation.

**Option 2: Run with Docker**

```bash
docker compose up --build
```

### Step 4: Configure environment variables (optional)

Create a `.env` file or export variables:

```bash
# API configuration
export API_HOST=0.0.0.0
export API_PORT=8000

# Camera configuration
export CAMERA_ID=0           # 0 for default camera
export CAMERA_WIDTH=1280
export CAMERA_HEIGHT=720

# Detection confidence
export DETECTION_CONFIDENCE=0.3  # 0.0 - 1.0
```

### Step 5: Upload code to ESP32

1. Install [PlatformIO](https://platformio.org/)
2. Open `firmware/GateKeeper` in VS Code
3. Edit WiFi and API info in `src/main.cpp`:

   ```cpp
   const char* WIFI_SSID = "TenWiFi";
   const char* WIFI_PASSWORD = "MatKhauWiFi";
   const char* WEBHOOK_URL = "http://192.168.1.100:8000/lpr";
   ```
4. Build and upload to ESP32

## ESP32 Wiring Diagram

```
ESP32          |  LM393 Sensor
---------------|---------------
GPIO 4         →  DO (Digital Out)
GND            →  GND
3.3V           →  VCC

ESP32          |  Servo SG90
---------------|-----------------------------
GPIO 5         →  Signal
GND            →  GND
3.3V           →  VCC

ESP32          |  OLED SSD1306
---------------|---------------
GPIO 21 (SDA)  →  SDA
GPIO 22 (SCL)  →  SCL
GND            →  GND
3.3V           →  VCC
```

⚠️ **Important notes**:

* Do NOT power the servo directly from the ESP32 (this may damage the board)
* Use a separate 5V power supply for the servo
* Connect grounds (GND) between ESP32, servo, and external power

## API Endpoints

### `GET /lpr`

Captures an image from the camera, recognizes the license plate, and returns the result.

**Response:**

```json
{
  "status": true,
  "plate": "29A12345"
}
```

## Operation Flow

1. **Vehicle detection**: LM393 sensor detects an approaching vehicle
2. **Request sent**: ESP32 shows “Checking…” and calls the API
3. **Image processing**:

   * API captures an image
   * YOLOv8 detects the plate region
   * PaddleOCR reads characters
   * Cleanup and normalization of the results
4. **Result returned**: API sends the response to ESP32
5. **Gate control**:

   * If valid: Servo opens 90°, display shows “ACCEPT” and the plate
   * If invalid: Display shows “DENY”

