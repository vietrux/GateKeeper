"""
Configuration settings for GateKeeper system
"""

import os
from pathlib import Path

# Base directory
BASE_DIR = Path(__file__).resolve().parent.parent

# Model paths
MODEL_PATH = BASE_DIR / "models" / "best.pt"

# Database configuration
DATABASE_DIR = BASE_DIR / "data" / "databases"
DATABASE_PATH = DATABASE_DIR / "car_park.db"

# Logging configuration
LOG_DIR = BASE_DIR / "data" / "logs"
LOG_LEVEL = os.getenv("LOG_LEVEL", "INFO")

# API configuration
API_HOST = os.getenv("API_HOST", "0.0.0.0")
API_PORT = int(os.getenv("API_PORT", "8000"))

# Web application configuration
WEB_HOST = os.getenv("WEB_HOST", "0.0.0.0")
WEB_PORT = int(os.getenv("WEB_PORT", "5000"))
WEB_SECRET_KEY = os.getenv("WEB_SECRET_KEY", None)  # Should be set in production

# Admin credentials (should be changed in production)
ADMIN_USERNAME = os.getenv("ADMIN_USERNAME", "admin")
ADMIN_PASSWORD = os.getenv("ADMIN_PASSWORD", "admin")

# Hardware configuration
UART_PORT = os.getenv("UART_PORT", "/dev/ttyAMA0")
UART_BAUDRATE = int(os.getenv("UART_BAUDRATE", "115200"))

# Camera configuration
CAMERA_ID = int(os.getenv("CAMERA_ID", "0"))
CAMERA_WIDTH = int(os.getenv("CAMERA_WIDTH", "1280"))
CAMERA_HEIGHT = int(os.getenv("CAMERA_HEIGHT", "720"))

# Detection configuration
DETECTION_CONFIDENCE = float(os.getenv("DETECTION_CONFIDENCE", "0.3"))

# Ensure required directories exist
os.makedirs(DATABASE_DIR, exist_ok=True)
os.makedirs(LOG_DIR, exist_ok=True)
