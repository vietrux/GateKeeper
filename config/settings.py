"""
Configuration settings for GateKeeper system
"""

import os
from pathlib import Path

# Base directory
BASE_DIR = Path(__file__).resolve().parent.parent

# Model paths
MODEL_PATH = BASE_DIR / "models" / "best.pt"

# API configuration
API_HOST = os.getenv("API_HOST", "0.0.0.0")
API_PORT = int(os.getenv("API_PORT", "8000"))

# Camera configuration
CAMERA_ID = int(os.getenv("CAMERA_ID", "0"))
CAMERA_WIDTH = int(os.getenv("CAMERA_WIDTH", "1280"))
CAMERA_HEIGHT = int(os.getenv("CAMERA_HEIGHT", "720"))

# Detection configuration
DETECTION_CONFIDENCE = float(os.getenv("DETECTION_CONFIDENCE", "0.3"))
