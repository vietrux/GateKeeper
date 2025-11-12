FROM python:3.13-slim

WORKDIR /app

# Install system dependencies for OpenCV
RUN apt-get update && apt-get install -y --no-install-recommends \
    libgl1-mesa-glx \
    libglib2.0-0 \
    && rm -rf /var/lib/apt/lists/*

# Copy requirements first to leverage Docker cache
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

# Copy project structure
COPY src/ ./src/
COPY models/ ./models/
COPY data/ ./data/

# Create necessary directories
RUN mkdir -p data/logs data/databases

# Expose the port used by the API
EXPOSE 8000

# Command to run the API
CMD ["python", "-m", "src.api.main"]