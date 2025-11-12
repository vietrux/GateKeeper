# Migration Guide: Old to New Structure

This guide helps you understand the changes made to reorganize the GateKeeper project structure.

## What Changed?

### Old Structure
```
GateKeeper/
├── templates/
├── test_img/
├── app.py
├── main.py
├── detector.py
├── ocr_reader.py
├── parser.py
├── smart_car_park.py
├── stm32_main.c
├── best.pt
├── car_park.db
└── *.log files
```

### New Structure
```
GateKeeper/
├── src/
│   ├── api/main.py              # Was: main.py
│   ├── web/app.py               # Was: app.py
│   ├── web/templates/           # Was: templates/
│   ├── hardware/smart_car_park.py  # Was: smart_car_park.py
│   └── core/                    # Was: detector.py, ocr_reader.py, parser.py
├── firmware/stm32_main.c        # Was: stm32_main.c
├── models/best.pt               # Was: best.pt
├── data/
│   ├── databases/car_park.db   # Was: car_park.db
│   └── logs/*.log              # Was: *.log
├── tests/test_images/           # Was: test_img/
├── config/settings.py           # NEW: Centralized configuration
├── scripts/setup_db.py          # NEW: Database setup utility
└── docker-compose.yml           # NEW: Multi-container setup
```

## Import Path Changes

### Before
```python
from detector import LicensePlateDetector
from ocr_reader import OCRReader
```

### After
```python
from src.core.detector import LicensePlateDetector
from src.core.ocr_reader import OCRReader
```

## File Path Changes

### Database
- **Before**: `car_park.db`
- **After**: `data/databases/car_park.db`

### Logs
- **Before**: `*.log` in root
- **After**: `data/logs/*.log`

### Model
- **Before**: `best.pt`
- **After**: `models/best.pt`

## How to Run Applications

### API Server

**Before:**
```bash
python main.py
```

**After:**
```bash
python -m src.api.main
# OR
cd /path/to/GateKeeper && python -m src.api.main
```

### Web Application

**Before:**
```bash
python app.py
```

**After:**
```bash
python -m src.web.app
# OR
cd /path/to/GateKeeper && python -m src.web.app
```

### Hardware Integration

**Before:**
```bash
python smart_car_park.py
```

**After:**
```bash
python -m src.hardware.smart_car_park
# OR
cd /path/to/GateKeeper && python -m src.hardware.smart_car_park
```

## Docker Changes

### Before
```dockerfile
COPY *.py .
CMD ["python", "main.py"]
```

### After
```dockerfile
COPY src/ ./src/
COPY models/ ./models/
CMD ["python", "-m", "src.api.main"]
```

## Configuration

### Before
Configuration was hardcoded in each file:
```python
db_path = "car_park.db"
model_path = "best.pt"
```

### After
Use centralized configuration:
```python
from config.settings import DATABASE_PATH, MODEL_PATH
```

Or use environment variables:
```bash
export DATABASE_PATH=data/databases/car_park.db
```

## Database Setup

### Before
Database was created automatically on first run (or didn't exist).

### After
Run the setup script:
```bash
python scripts/setup_db.py
```

This ensures proper database initialization with optional sample data.

## Benefits of New Structure

1. **Clear Separation**: API, web, hardware, and core logic are separated
2. **Better Testing**: Test files are organized in dedicated directory
3. **Easier Deployment**: Docker compose for multi-container setup
4. **Centralized Config**: Single source of truth for configuration
5. **Data Organization**: Logs and databases in separate data directory
6. **Scalability**: Easy to add new modules without cluttering root
7. **Best Practices**: Follows Python package structure conventions

## Troubleshooting

### Import Errors

If you see `ModuleNotFoundError: No module named 'src'`:

**Solution**: Run from project root directory
```bash
cd /path/to/GateKeeper
python -m src.api.main
```

### File Not Found Errors

If you see `FileNotFoundError` for database or model:

**Solution**: Ensure you're running from project root and paths are correct
```bash
cd /path/to/GateKeeper
ls models/best.pt          # Should exist
ls data/databases/         # Should exist
python scripts/setup_db.py # Create database
```

### Template Not Found (Flask)

Flask might not find templates if not running from correct directory:

**Solution**: The app.py now uses proper template folder configuration. Run from root:
```bash
cd /path/to/GateKeeper
python -m src.web.app
```

## Migration Checklist

- [ ] Database moved to `data/databases/`
- [ ] Run `python scripts/setup_db.py` to initialize new database
- [ ] Update any custom scripts to use new import paths
- [ ] Update any systemd services or cron jobs with new command paths
- [ ] Update environment variables if using custom paths
- [ ] Test API: `python -m src.api.main`
- [ ] Test Web: `python -m src.web.app`
- [ ] Test Hardware: `python -m src.hardware.smart_car_park`
- [ ] Update documentation links

## Questions?

If you encounter any issues during migration, check:
1. Are you in the project root directory?
2. Are all dependencies installed? (`pip install -r requirements.txt`)
3. Does the database exist? (`ls data/databases/car_park.db`)
4. Is the model file present? (`ls models/best.pt`)
