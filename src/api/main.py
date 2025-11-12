import os, logging, cv2
from io import BytesIO
from fastapi import FastAPI, File, UploadFile, HTTPException
from fastapi.responses import JSONResponse
import uvicorn
from src.core.detector import LicensePlateDetector
from src.core.ocr_reader import OCRReader

logging.basicConfig(
    level=logging.INFO, format="%(asctime)s - %(name)s - %(levelname)s - %(message)s"
)
logger = logging.getLogger(__name__)
app = FastAPI(
    title="License Plate Recognition API",
    description="API for detecting and recognizing license plates from images",
    version="1.0.0",
)
detector = LicensePlateDetector(model_path="models/best.pt")
ocr = OCRReader()

# Camera configuration
CAMERA_ID = 2
FRAME_WIDTH = 1920
FRAME_HEIGHT = 1080


def capture_image_from_camera():
    """Capture a single frame from the camera"""
    try:
        logger.info(f"Opening camera (ID: {CAMERA_ID})")
        cap = cv2.VideoCapture(CAMERA_ID)

        if not cap.isOpened():
            logger.error(f"Failed to open camera with ID {CAMERA_ID}")
            return None

        # Set camera parameters
        cap.set(cv2.CAP_PROP_FRAME_WIDTH, FRAME_WIDTH)
        cap.set(cv2.CAP_PROP_FRAME_HEIGHT, FRAME_HEIGHT)

        # Wait for camera to warm up
        logger.info("Warming up camera...")
        for _ in range(5):
            cap.read()

        # Capture frame
        ret, frame = cap.read()
        cap.release()

        if not ret or frame is None:
            logger.error("Failed to capture frame from camera")
            return None

        logger.info(f"Captured frame with shape: {frame.shape}")
        return frame

    except Exception as e:
        logger.error(f"Error capturing image from camera: {str(e)}")
        return None


@app.get("/lpr")
async def recognize_license_plate_from_camera():
    """
    Capture image from camera, detect license plate, and perform OCR.
    Returns: {"plate": "plate_text", "status": True} if detected, else {"status": False}
    """
    try:
        # Step 1: Capture image from camera
        logger.info("Triggering license plate recognition from camera")
        frame = capture_image_from_camera()

        if frame is None:
            logger.warning("Failed to capture image from camera")
            return {"status": False}

        # Step 2: Convert frame to bytes for detector
        ret, buffer = cv2.imencode(".jpg", frame)
        if not ret:
            logger.error("Failed to encode captured image")
            return {"status": False}

        image_bytes = buffer.tobytes()

        # Step 3: Detect license plate
        logger.info("Detecting license plate...")
        cropped_plate = detector.detect_and_crop(image_bytes)

        if cropped_plate is None:
            logger.warning("No license plate detected in captured image")
            return {"status": False}

        # Step 4: Perform OCR
        logger.info("Performing OCR on detected plate...")
        plate_text = ocr.read_text(cropped_plate)

        if not plate_text:
            logger.warning("Could not read text from license plate")
            return {"status": False}

        # Step 5: Clean and format plate text
        plate_text = "".join(ch for ch in plate_text if ch.isalnum()).upper()
        logger.info(f"Successfully recognized license plate: {plate_text}")

        return {"plate": plate_text, "status": True}

    except Exception as e:
        logger.error(f"Error processing request: {str(e)}")
        return {"status": False}


@app.post("/lpr/upload")
async def recognize_license_plate_from_upload(file: UploadFile = File(...)):
    """
    Legacy endpoint: Upload image file for license plate recognition.
    Returns: {"plate": "plate_text", "status": True} if detected, else {"status": False}
    """
    try:
        if file.content_type and not file.content_type.startswith("image/"):
            raise HTTPException(status_code=400, detail="File is not an image")

        contents = await file.read()
        cropped_plate = detector.detect_and_crop(contents)

        if cropped_plate is None:
            return {"status": False}

        plate_text = ocr.read_text(cropped_plate)

        if not plate_text:
            return {"status": False}

        plate_text = "".join(ch for ch in plate_text if ch.isalnum()).upper()
        return {"plate": plate_text, "status": True}

    except Exception as e:
        logger.error(f"Error processing request: {str(e)}")
        return {"status": False}


if __name__ == "__main__":
    uvicorn.run("main:app", host="0.0.0.0", port=8000, reload=True)
