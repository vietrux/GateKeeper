#!/usr/bin/env python3
"""
Car Park License Plate Recognition System - Raspberry Pi 5

This program:
1. Listens for car detection events from STM32 via UART
2. Captures image from camera when car is detected
3. Performs license plate detection and recognition
4. Checks if license plate is in the database
5. Sends response back to STM32
"""

import time
import logging
import sqlite3
import serial
import cv2
import numpy as np
from io import BytesIO
import threading
import os
import sys
from datetime import datetime

# Import detector and OCR modules directly instead of using API
from src.core.detector import LicensePlateDetector
from src.core.ocr_reader import OCRReader

# Configure enhanced logging
log_dir = "data/logs"
os.makedirs(log_dir, exist_ok=True)

# Create log file name with timestamp
log_file = os.path.join(
    log_dir, f"carpark_{datetime.now().strftime('%Y%m%d_%H%M%S')}.log"
)

# Set up handlers for both file and console output
logger = logging.getLogger("CarParkSystem")
logger.setLevel(logging.DEBUG)  # Set to DEBUG level for detailed logging

# File handler with INFO level
file_handler = logging.FileHandler(log_file)
file_handler.setLevel(logging.INFO)
file_format = logging.Formatter("%(asctime)s - %(name)s - %(levelname)s - %(message)s")
file_handler.setFormatter(file_format)

# Console handler with DEBUG level (more verbose)
console_handler = logging.StreamHandler(sys.stdout)
console_handler.setLevel(logging.DEBUG)
console_format = logging.Formatter("%(asctime)s [%(levelname)s] %(message)s")
console_handler.setFormatter(console_format)

# Add both handlers to logger
logger.addHandler(file_handler)
logger.addHandler(console_handler)

# Prevent propagation to root logger to avoid duplicate messages
logger.propagate = False


class CarParkSystem:
    def __init__(self):
        logger.debug("Initializing Car Park System")
        # UART configuration for Raspberry Pi 5
        self.PORT = "/dev/ttyAMA0"
        self.BAUDRATE = 115200
        self.serial_conn = None

        # Initialize plate detection models
        logger.debug("Loading license plate detection model")
        self.detector = LicensePlateDetector(model_path="models/best.pt")
        logger.debug("Loading OCR model")
        self.ocr = OCRReader()

        # Database connection
        self.db_conn = None

        # Camera settings
        self.camera_id = 0
        self.frame_width = 1280
        self.frame_height = 720
        logger.debug(
            f"Camera settings: ID={self.camera_id}, Resolution={self.frame_width}x{self.frame_height}"
        )

        # State variables
        self.is_running = False
        self.current_frame = None
        self.camera_thread = None
        logger.debug("Car Park System initialization complete")

    def connect_to_serial(self):
        """Initialize serial connection to STM32"""
        logger.debug(
            f"Attempting to connect to STM32 on port {self.PORT} at {self.BAUDRATE} baud"
        )
        try:
            self.serial_conn = serial.Serial(
                port=self.PORT,
                baudrate=self.BAUDRATE,
                bytesize=serial.EIGHTBITS,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE,
                timeout=1,
            )
            logger.info(f"Connected to STM32 on {self.PORT}")
            return True
        except Exception as e:
            logger.error(f"Failed to connect to serial port: {e}")
            logger.debug(f"Serial connection error details: {str(e)}", exc_info=True)
            return False

    def connect_to_database(self):
        """Connect to SQLite database"""
        db_path = "car_park.db"
        logger.debug(f"Attempting to connect to database at {os.path.abspath(db_path)}")
        try:
            self.db_conn = sqlite3.connect(db_path, check_same_thread=False)

            # Verify database structure
            cursor = self.db_conn.cursor()
            cursor.execute(
                "SELECT name FROM sqlite_master WHERE type='table' AND name='plates'"
            )
            if not cursor.fetchone():
                logger.warning("Plates table not found in database. Creating it.")
                cursor.execute(
                    "CREATE TABLE plates (id INTEGER PRIMARY KEY, plate_number TEXT, owner_name TEXT, access_level INTEGER)"
                )
                self.db_conn.commit()
                logger.debug("Created plates table in database")
            else:
                cursor.execute("PRAGMA table_info(plates)")
                columns = [col[1] for col in cursor.fetchall()]
                logger.debug(f"Database columns: {columns}")

            logger.info("Connected to database successfully")
            return True
        except Exception as e:
            logger.error(f"Database connection error: {e}")
            logger.debug(f"Database connection error details: {str(e)}", exc_info=True)
            return False

    def initialize_camera(self):
        """Start camera capture in a separate thread"""
        logger.debug(f"Initializing camera (ID: {self.camera_id})")

        def camera_loop():
            logger.debug("Camera thread started")
            cap = cv2.VideoCapture(self.camera_id)
            if not cap.isOpened():
                logger.error(f"Failed to open camera with ID {self.camera_id}")
                return

            # Set camera parameters
            cap.set(cv2.CAP_PROP_FRAME_WIDTH, self.frame_width)
            cap.set(cv2.CAP_PROP_FRAME_HEIGHT, self.frame_height)

            # Get actual camera parameters (might differ from requested)
            actual_width = cap.get(cv2.CAP_PROP_FRAME_WIDTH)
            actual_height = cap.get(cv2.CAP_PROP_FRAME_HEIGHT)
            actual_fps = cap.get(cv2.CAP_PROP_FPS)
            logger.debug(
                f"Camera parameters: {actual_width}x{actual_height} at {actual_fps} FPS"
            )

            logger.info("Camera initialized successfully")

            frame_count = 0
            start_time = time.time()

            while self.is_running:
                ret, frame = cap.read()
                if ret:
                    self.current_frame = frame
                    frame_count += 1

                    # Log FPS every 30 seconds
                    if frame_count % 900 == 0:  # ~30 seconds at 30 FPS
                        elapsed = time.time() - start_time
                        fps = frame_count / elapsed if elapsed > 0 else 0
                        logger.debug(f"Camera capturing at {fps:.2f} FPS")
                        frame_count = 0
                        start_time = time.time()
                else:
                    logger.warning("Failed to capture frame from camera")

                time.sleep(0.03)  # ~30 FPS

            logger.debug("Camera thread stopping")
            cap.release()
            logger.debug("Camera released")

        self.is_running = True
        self.camera_thread = threading.Thread(target=camera_loop, name="CameraThread")
        self.camera_thread.daemon = True
        self.camera_thread.start()

        # Wait for camera to initialize
        logger.debug("Waiting for camera initialization")
        time.sleep(2)

        is_alive = self.camera_thread.is_alive()
        logger.debug(f"Camera thread status: {'Running' if is_alive else 'Failed'}")
        return is_alive

    def check_plate_in_database(self, plate_number):
        """Check if the license plate exists in the database"""
        logger.debug(f"Checking plate '{plate_number}' in database")
        if not self.db_conn:
            logger.error("Database not connected")
            return False

        try:
            cursor = self.db_conn.cursor()
            query = "SELECT COUNT(*) FROM plates WHERE plate_number = ?"
            logger.debug(f"Executing query: {query} with param: {plate_number}")

            cursor.execute(query, (plate_number,))
            count = cursor.fetchone()[0]

            logger.info(
                f"Plate {plate_number}: {'Authorized' if count > 0 else 'Unauthorized'}"
            )
            return count > 0
        except Exception as e:
            logger.error(f"Database query error: {e}")
            logger.debug(f"Database query error details: {str(e)}", exc_info=True)
            return False

    def process_captured_image(self):
        """Process the current camera frame to detect license plate"""
        logger.debug("Processing captured image for license plate detection")
        if self.current_frame is None:
            logger.error("No camera frame available")
            return None

        try:
            frame_shape = self.current_frame.shape
            logger.debug(f"Processing frame with shape: {frame_shape}")

            # Convert frame to bytes for detector
            start_time = time.time()
            ret, buffer = cv2.imencode(".jpg", self.current_frame)
            if not ret:
                logger.error("Failed to encode image")
                return None

            jpg_bytes = BytesIO(buffer).getvalue()
            logger.debug(f"Image encoded to JPEG: {len(jpg_bytes)} bytes")

            # Detect and crop license plate
            detection_start = time.time()
            logger.debug("Starting license plate detection")
            cropped_plate = self.detector.detect_and_crop(jpg_bytes)
            detection_time = time.time() - detection_start
            logger.debug(f"License plate detection took {detection_time:.3f} seconds")

            if cropped_plate is None:
                logger.warning("No license plate detected in the image")
                return None

            # Read text from cropped plate
            ocr_start = time.time()
            logger.debug("Starting OCR processing")
            plate_text = self.ocr.read_text(cropped_plate)
            ocr_time = time.time() - ocr_start
            logger.debug(f"OCR processing took {ocr_time:.3f} seconds")

            if not plate_text:
                logger.warning("Could not read license plate text")
                return None

            # Clean plate text (alphanumeric only, uppercase)
            original_text = plate_text
            plate_text = "".join(ch for ch in plate_text if ch.isalnum()).upper()
            logger.debug(f"OCR result: '{original_text}' cleaned to '{plate_text}'")

            total_time = time.time() - start_time
            logger.info(
                f"Detected license plate: {plate_text} (processing took {total_time:.3f} seconds)"
            )

            return plate_text
        except Exception as e:
            logger.error(f"Error processing image: {e}")
            logger.debug(f"Image processing error details: {str(e)}", exc_info=True)
            return None

    def run(self):
        """Main loop to handle car detection events"""
        logger.info("Starting Car Park System")

        # Initialize connections
        if not self.connect_to_serial():
            logger.error("Failed to initialize serial connection")
            return

        if not self.connect_to_database():
            logger.error("Failed to initialize database")
            return

        if not self.initialize_camera():
            logger.error("Failed to initialize camera")
            return

        logger.info("System initialized and ready")
        logger.debug("Entering main event loop")

        try:
            idle_count = 0
            while True:
                # Read from serial port
                if self.serial_conn.in_waiting > 0:
                    idle_count = 0
                    serial_data = self.serial_conn.readline().decode("utf-8").strip()
                    logger.info(f"Received from STM32: {serial_data}")

                    if "CAR_DETECTED" in serial_data:
                        logger.info("Processing car detection event")
                        event_start = time.time()

                        # Take a short delay to ensure car is properly positioned
                        logger.debug("Waiting for car to position properly")
                        time.sleep(0.5)

                        # Process image to detect license plate
                        logger.debug("Starting license plate processing")
                        plate_number = self.process_captured_image()

                        if plate_number:
                            # Check if plate is authorized
                            logger.debug(
                                f"Checking if plate {plate_number} is authorized"
                            )
                            is_authorized = self.check_plate_in_database(plate_number)

                            # Send response back to STM32
                            if is_authorized:
                                logger.debug("Sending authorization success to STM32")
                                self.serial_conn.write(b"OK\n")
                                logger.info(f"Access granted for plate: {plate_number}")
                            else:
                                logger.debug("Sending authorization failure to STM32")
                                self.serial_conn.write(b"NO\n")
                                logger.info(f"Access denied for plate: {plate_number}")
                        else:
                            # No plate detected or couldn't read plate
                            logger.debug("Sending plate recognition failure to STM32")
                            self.serial_conn.write(b"NO\n")
                            logger.info("Access denied: License plate not recognized")

                        event_time = time.time() - event_start
                        logger.debug(
                            f"Car detection event processing completed in {event_time:.3f} seconds"
                        )
                else:
                    idle_count += 1
                    # Log system is alive every ~10 seconds if idle
                    if idle_count >= 100:
                        logger.debug("System idle - waiting for car detection events")
                        idle_count = 0

                time.sleep(0.1)

        except KeyboardInterrupt:
            logger.info("System shutdown requested by user (KeyboardInterrupt)")
        except Exception as e:
            logger.error(f"Unexpected error: {e}")
            logger.critical(f"Critical system error details: {str(e)}", exc_info=True)
        finally:
            # Cleanup
            logger.debug("Starting cleanup sequence")
            self.is_running = False
            if self.camera_thread:
                logger.debug("Waiting for camera thread to terminate")
                self.camera_thread.join(timeout=1.0)
                logger.debug(
                    f"Camera thread terminated: {'Yes' if not self.camera_thread.is_alive() else 'No, had to leave it'}"
                )
            if self.serial_conn:
                logger.debug("Closing serial connection")
                self.serial_conn.close()
            if self.db_conn:
                logger.debug("Closing database connection")
                self.db_conn.close()
            logger.info("System shutdown complete")


if __name__ == "__main__":
    logger.debug("Script started")
    try:
        # Start the system
        car_park_system = CarParkSystem()
        car_park_system.run()
    except Exception as e:
        logger.critical(f"Fatal error: {e}", exc_info=True)
        sys.exit(1)
