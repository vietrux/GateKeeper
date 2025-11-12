# GateKeeper - Quick Reference Card

## 📁 Project Structure at a Glance

```
GateKeeper/
├── src/                    # Source code
│   ├── api/               # REST API
│   ├── web/               # Web interface
│   ├── hardware/          # Raspberry Pi/STM32
│   └── core/              # Detection & OCR
├── models/                # ML models (best.pt)
├── data/                  # Databases & logs
├── tests/                 # Test images
├── config/                # Settings
├── scripts/               # Utilities
├── firmware/              # STM32 code
└── docs/                  # Documentation
```

## 🚀 Quick Commands

### Setup
```bash
# Install dependencies
pip install -r requirements.txt

# Initialize database
python scripts/setup_db.py
```

### Run Services
```bash
# API Server (port 8000)
python -m src.api.main

# Web Interface (port 5000)
python -m src.web.app

# Hardware Integration
python -m src.hardware.smart_car_park

# All services with Docker
docker-compose up -d
```

### Test
```bash
# Test API
curl -X POST "http://localhost:8000/lpr" \
  -F "file=@tests/test_images/CarLongPlateGen1624_jpg.rf.0b3825b996f8aa91567271e09a8c6040.jpg"

# Web interface
open http://localhost:5000
# Login: admin/admin
```

## 📝 Key Files

| File | Purpose |
|------|---------|
| `src/api/main.py` | FastAPI endpoints |
| `src/web/app.py` | Flask web app |
| `src/hardware/smart_car_park.py` | Hardware integration |
| `src/core/detector.py` | YOLOv8 detection |
| `src/core/ocr_reader.py` | PaddleOCR recognition |
| `config/settings.py` | Configuration |
| `scripts/setup_db.py` | Database setup |

## 🔧 Configuration

Set environment variables:
```bash
export API_PORT=8000
export WEB_PORT=5000
export DATABASE_PATH=data/databases/car_park.db
export MODEL_PATH=models/best.pt
```

Or edit `config/settings.py` directly.

## 📊 Database

Location: `data/databases/car_park.db`

Tables:
- `plates` - Authorized license plates
- `movement_log` - Entry/exit logs

## 🔐 Default Credentials

**Web Interface:**
- Username: `admin`
- Password: `admin`

⚠️ **Change in production!**

## 📂 Important Paths

```python
# In code, use:
from config.settings import (
    DATABASE_PATH,
    MODEL_PATH,
    LOG_DIR
)
```

## 🐳 Docker

```bash
# Build
docker-compose build

# Run
docker-compose up -d

# Stop
docker-compose down

# Logs
docker-compose logs -f
```

## 🔍 Troubleshooting

### Import errors
```bash
# Always run from project root
cd /path/to/GateKeeper
python -m src.api.main
```

### Database not found
```bash
python scripts/setup_db.py
```

### Model not found
```bash
# Ensure models/best.pt exists
ls models/best.pt
```

## 📖 More Info

- **Quick Start**: `README.md`
- **Migration**: `MIGRATION_GUIDE.md`
- **Full Docs**: `docs/README.md`
- **Summary**: `REORGANIZATION_SUMMARY.md`

## 🎯 Common Tasks

### Add a license plate
1. Go to http://localhost:5000
2. Login
3. Navigate to "Plates" → "Add New"
4. Enter plate number
5. Submit

### View logs
1. Web: http://localhost:5000/logs
2. Files: `data/logs/*.log`

### Test detection
```bash
python -m src.api.main &
curl -X POST "http://localhost:8000/lpr" \
  -F "file=@tests/test_images/[image].jpg"
```

---

**Need help?** Check the full documentation in `README.md`
