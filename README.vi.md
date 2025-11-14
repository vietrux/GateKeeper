# GateKeeper

Hệ thống cổng thông minh tự động nhận dạng biển số xe, kết hợp giữa dịch vụ xử lý hình ảnh FastAPI và bộ điều khiển ESP32.

## Giới thiệu

GateKeeper hoạt động như một hệ thống kiểm soát ra vào tự động:
- **Phần xử lý trung tâm (FastAPI)**: Chụp ảnh từ camera, nhận dạng biển số bằng YOLOv8 và PaddleOCR, sau đó đưa ra quyết định cho phép hoặc từ chối
- **Phần điều khiển thiết bị (ESP32)**: Sử dụng cảm biến LM393 phát hiện xe, gọi API kiểm tra, điều khiển servo mở cổng và hiển thị kết quả trên màn hình OLED

## Tính năng chính

### Xử lý hình ảnh thông minh
- Phát hiện biển số xe bằng YOLOv8
- Nhận dạng ký tự bằng PaddleOCR, tối ưu cho biển số Việt Nam
- Xử lý trực tiếp từ camera, không cần thao tác thủ công

### Tự động hóa hoàn toàn
- Cảm biến LM393 tự động phát hiện xe
- Gọi API và xử lý kết quả
- Điều khiển servo mở/đóng cổng
- Hiển thị trạng thái thời gian thực trên màn OLED

### Dễ dàng triển khai
- Đóng gói đầy đủ trong Docker container
- Cấu hình linh hoạt qua biến môi trường
- Hỗ trợ nhiều loại camera

## Cách hoạt động

```
Xe đến → Cảm biến phát hiện → ESP32 gọi API
                                    ↓
                        API chụp ảnh & nhận dạng
                                    ↓
                        Trả kết quả cho ESP32
                                    ↓
            Mở cổng (nếu hợp lệ) & hiển thị trên màn hình
```

## Cấu trúc dự án

```
GateKeeper/
├── src/
│   ├── api/
│   │   └── main.py              # Khởi chạy FastAPI và định nghĩa các endpoint
│   └── core/
│       ├── detector.py          # Phát hiện biển số bằng YOLOv8
│       ├── ocr_reader.py        # Đọc ký tự bằng PaddleOCR
│       └── parser.py            # Xử lý và chuẩn hóa biển số
├── firmware/
│   └── GateKeeper/
│       └── src/
│           └── main.cpp         # Code ESP32 (WiFi, HTTP, servo, OLED)
├── config/
│   └── settings.py              # Cấu hình hệ thống
├── models/
│   └── best.pt                  # Mô hình YOLOv8 (cần tải riêng)
├── docker-compose.yml           # Chạy dịch vụ bằng Docker
├── Dockerfile                   # Build image Docker
└── requirements.txt             # Thư viện Python cần thiết
```

## Yêu cầu phần cứng

### Cho phần API
- Camera USB hoặc webcam
- Máy tính/server chạy Docker (hoặc Python 3.11+)
- GPU tùy chọn (CPU vẫn chạy được nhưng chậm hơn)

### Cho phần ESP32
- Board ESP32 Wroom 32 DevKit v1
- Cảm biến chướng ngại vật LM393
- Servo SG90
- Màn hình OLED SSD1306 128x64 (giao tiếp I²C)

## Hướng dẫn cài đặt

### Bước 1: Chuẩn bị môi trường Python

```bash
# Tạo môi trường ảo
python -m venv .venv

# Kích hoạt môi trường
source .venv/bin/activate  # Linux/Mac
# hoặc
.venv\Scripts\activate     # Windows

# Cài đặt thư viện
pip install --upgrade pip
pip install -r requirements.txt
```

### Bước 2: Tải mô hình YOLOv8

Đặt file `best.pt` (mô hình đã huấn luyện) vào thư mục `models/`

### Bước 3: Chạy API

**Cách 1: Chạy trực tiếp**
```bash
python -m src.api.main
```

Server sẽ khởi động tại `http://localhost:8000`  
Truy cập `http://localhost:8000/docs` để xem tài liệu API

**Cách 2: Chạy bằng Docker**
```bash
docker compose up --build
```

### Bước 4: Cấu hình biến môi trường (tùy chọn)

Tạo file `.env` hoặc export các biến:

```bash
# Cấu hình API
export API_HOST=0.0.0.0
export API_PORT=8000

# Cấu hình camera
export CAMERA_ID=0           # 0 cho camera mặc định
export CAMERA_WIDTH=1280
export CAMERA_HEIGHT=720

# Độ chính xác nhận dạng
export DETECTION_CONFIDENCE=0.3  # 0.0 - 1.0
```

### Bước 5: Nạp code cho ESP32

1. Cài đặt [PlatformIO](https://platformio.org/)
2. Mở thư mục `firmware/GateKeeper` trong VS Code
3. Sửa thông tin WiFi và địa chỉ API trong file `src/main.cpp`:
   ```cpp
   const char* WIFI_SSID = "TenWiFi";
   const char* WIFI_PASSWORD = "MatKhauWiFi";
   const char* WEBHOOK_URL = "http://192.168.1.100:8000/lpr";
   ```
4. Build và upload code lên ESP32

## Sơ đồ đấu nối ESP32

```
ESP32          |  LM393 Sensor
---------------|---------------
GPIO 4         →  DO (Digital Out)
GND            →  GND
3.3V           →  VCC

ESP32          |  Servo SG90
---------------|----------------------------------
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

⚠️ **Lưu ý quan trọng**:
- Không cấp nguồn servo trực tiếp từ ESP32 (sẽ làm hỏng board)
- Dùng nguồn 5V riêng cho servo
- Nối chung mass (GND) giữa ESP32, servo và nguồn ngoài

## API Endpoints

### `GET /lpr`
Chụp ảnh từ camera, nhận dạng biển số và trả về kết quả

**Response:**
```json
{
  "status": true,
  "plate": "29A12345"
}
```

## Quy trình hoạt động

1. **Phát hiện xe**: Cảm biến LM393 phát hiện có xe đến
2. **Gửi yêu cầu**: ESP32 hiển thị "Checking..." và gọi API
3. **Xử lý ảnh**: 
   - API chụp ảnh từ camera
   - YOLOv8 phát hiện vị trí biển số
   - PaddleOCR đọc ký tự trên biển số
   - Làm sạch và chuẩn hóa kết quả
4. **Trả kết quả**: API gửi kết quả về ESP32
5. **Điều khiển cổng**:
   - Nếu hợp lệ: Mở servo 90°, hiển thị "ACCEPT" và biển số
   - Nếu không hợp lệ: Hiển thị "DENY"
