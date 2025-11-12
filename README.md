# GateKeeper - Smart Car Park System

A comprehensive smart car park system with license plate recognition using YOLOv8 and PaddleOCR, automated barrier control, and web-based management interface.

## 🏗️ Project Structure

```
GateKeeper/
├── src/                          # Main application code
│   ├── api/                      # FastAPI application
│   │   └── main.py              # License plate recognition API
│   ├── web/                      # Flask web application
│   │   ├── app.py               # Web interface
│   │   ├── templates/           # HTML templates
│   │   └── static/              # CSS, JS, images
│   ├── hardware/                 # Hardware integration
│   │   └── smart_car_park.py    # Raspberry Pi integration
│   └── core/                     # Core business logic
│       ├── detector.py          # YOLOv8 license plate detection
│       ├── ocr_reader.py        # PaddleOCR text recognition
│       └── parser.py            # Plate text parsing
├── firmware/                     # STM32 firmware
│   └── stm32_main.c             # STM32 control code
├── models/                       # ML models
│   └── best.pt                  # YOLOv8 trained model
├── data/                         # Data storage
│   ├── databases/               # SQLite databases
│   └── logs/                    # Application logs
├── tests/                        # Test suite
│   └── test_images/            # Test images
├── config/                       # Configuration
│   └── settings.py             # Centralized configuration
├── scripts/                      # Utility scripts
│   └── setup_db.py             # Database setup
├── docs/                         # Documentation
│   └── README.md               # Detailed documentation
├── requirements.txt             # Python dependencies
├── Dockerfile                   # Docker container definition
└── docker-compose.yml          # Multi-container setup
```

## 🚀 Quick Start

### Prerequisites

- Python 3.9+
- Docker and Docker Compose (optional)
- Raspberry Pi 4/5 (for hardware integration)
- STM32 microcontroller (for barrier control)
- USB Webcam

### Installation

1. **Clone the repository**
   ```bash
   git clone <repository-url>
   cd GateKeeper
   ```

2. **Install Python dependencies**
   ```bash
   pip install -r requirements.txt
   ```

3. **Setup database**
   ```bash
   python scripts/setup_db.py
   ```

4. **Run the API server**
   ```bash
   # From project root
   python -m src.api.main
   ```

5. **Run the web interface (in separate terminal)**
   ```bash
   # From project root
   python -m src.web.app
   ```

### Using Docker

1. **Build and run with Docker Compose**
   ```bash
   docker-compose up -d
   ```

2. **Access the services**
   - API: http://localhost:8000
   - Web Interface: http://localhost:5000

## 📖 Usage

### Web Interface

1. Navigate to `http://localhost:5000`
2. Login with credentials (default: admin/admin)
3. Add authorized license plates
4. View access logs and system status

### API

**Recognize License Plate**
```bash
curl -X POST "http://localhost:8000/lpr" \
  -H "Content-Type: multipart/form-data" \
  -F "file=@/path/to/image.jpg"
```

Response:
```json
{
  "plate_text": "29A12345"
}
```

### Hardware Integration

For Raspberry Pi with STM32:

```bash
# From project root
python -m src.hardware.smart_car_park
```

## ⚙️ Configuration

Edit `config/settings.py` or use environment variables:

```bash
# API Configuration
export API_HOST=0.0.0.0
export API_PORT=8000

# Web Configuration
export WEB_HOST=0.0.0.0
export WEB_PORT=5000
export WEB_SECRET_KEY=your-secret-key

# Hardware Configuration
export UART_PORT=/dev/ttyAMA0
export UART_BAUDRATE=115200
export CAMERA_ID=0

# Detection Configuration
export DETECTION_CONFIDENCE=0.3
```

## 🔧 Development

### Project Organization

- **src/api/** - FastAPI endpoints for license plate recognition
- **src/web/** - Flask web interface for management
- **src/hardware/** - Raspberry Pi integration with STM32
- **src/core/** - Core detection and OCR logic
- **config/** - Centralized configuration
- **scripts/** - Utility scripts for setup and maintenance

### Adding New Features

1. Core logic goes in `src/core/`
2. API endpoints in `src/api/`
3. Web pages in `src/web/templates/`
4. Configuration in `config/settings.py`

### Running Tests

```bash
# Run all tests
python -m pytest tests/

# With test images
python -m pytest tests/ -v
```

## 🔐 Security Notes

⚠️ **Important**: This is a development setup. For production:

1. Change default admin credentials
2. Set strong `WEB_SECRET_KEY`
3. Use HTTPS for web interface
4. Implement proper authentication
5. Secure database with proper permissions
6. Use environment variables for sensitive data

## 📝 Hardware Requirements

### Components
- **STM32F4 Microcontroller**
- **Raspberry Pi 4/5**
- **LM393 IR Sensor** (car detection)
- **SG90 Servo Motor** (barrier control)
- **SSD1306 OLED Display** (status display)
- **USB Webcam** (license plate capture)

### Wiring

See `docs/README.md` for detailed wiring diagrams and hardware setup instructions.

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Submit a pull request

## 📄 License

This project is for educational purposes. Use in production requires proper licensing and security measures.

## 🙏 Acknowledgements

- YOLOv8 by Ultralytics
- PaddleOCR by PaddlePaddle
- Flask web framework
- FastAPI framework

## 📞 Support

For issues and questions, please open an issue on GitHub.
