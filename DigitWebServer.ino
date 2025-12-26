/**
 * ESP32 Digit Recognition Web Server
 * Creates WiFi Access Point and serves web interface for digit recognition
 * Works offline - no internet required!
 */

#include "digit_recogn.h"
#include <tflm_esp32.h>
#include <eloquent_tinyml.h>
#include <WiFi.h>
#include <WebServer.h>

// WiFi Access Point credentials
const char* ap_ssid = "ESP32-Digit-Recognition";
const char* ap_password = "12345678";  // Must be at least 8 characters

// TensorFlow model configuration
#define ARENA_SIZE 10000
Eloquent::TF::Sequential<10, ARENA_SIZE> tf;

// Web server on port 80
WebServer server(80);

// HTML page with camera and upload interface (will be defined below)
const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Digit Recognition</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            display: flex;
            justify-content: center;
            align-items: center;
            padding: 20px;
        }
        .container {
            background: white;
            border-radius: 20px;
            padding: 30px;
            max-width: 500px;
            width: 100%;
            box-shadow: 0 20px 60px rgba(0,0,0,0.3);
        }
        h1 {
            color: #667eea;
            text-align: center;
            margin-bottom: 10px;
            font-size: 28px;
        }
        .subtitle {
            text-align: center;
            color: #666;
            margin-bottom: 30px;
            font-size: 14px;
        }
        .btn {
            width: 100%;
            padding: 15px;
            margin: 10px 0;
            border: none;
            border-radius: 10px;
            font-size: 16px;
            font-weight: 600;
            cursor: pointer;
            transition: all 0.3s;
        }
        .btn-primary {
            background: #667eea;
            color: white;
        }
        .btn-primary:hover {
            background: #5568d3;
            transform: translateY(-2px);
            box-shadow: 0 5px 15px rgba(102,126,234,0.4);
        }
        .btn-secondary {
            background: #f0f0f0;
            color: #333;
        }
        .btn-secondary:hover {
            background: #e0e0e0;
        }
        input[type="file"] {
            display: none;
        }
        #video {
            width: 100%;
            border-radius: 10px;
            margin: 15px 0;
            display: none;
            background: #000;
        }
        .preview-container {
            display: flex;
            justify-content: center;
            align-items: center;
            margin: 20px 0;
            gap: 20px;
        }
        .preview-box {
            text-align: center;
        }
        .preview-box h3 {
            color: #666;
            font-size: 12px;
            margin-bottom: 10px;
            text-transform: uppercase;
        }
        canvas {
            border: 2px solid #e0e0e0;
            border-radius: 8px;
        }
        #originalCanvas {
            max-width: 200px;
            max-height: 200px;
        }
        #canvas {
            width: 112px;
            height: 112px;
            image-rendering: pixelated;
            image-rendering: crisp-edges;
        }
        .result {
            text-align: center;
            margin-top: 20px;
            padding: 20px;
            border-radius: 10px;
            background: #f8f9fa;
            display: none;
        }
        .result.show {
            display: block;
        }
        .result-digit {
            font-size: 72px;
            font-weight: bold;
            color: #667eea;
            margin: 10px 0;
        }
        .result-label {
            color: #666;
            font-size: 14px;
        }
        .confidence {
            margin-top: 10px;
            font-size: 12px;
            color: #999;
        }
        .loading {
            display: none;
            text-align: center;
            margin: 20px 0;
            color: #667eea;
        }
        .loading.show {
            display: block;
        }
        .spinner {
            border: 3px solid #f3f3f3;
            border-top: 3px solid #667eea;
            border-radius: 50%;
            width: 40px;
            height: 40px;
            animation: spin 1s linear infinite;
            margin: 0 auto;
        }
        @keyframes spin {
            0% { transform: rotate(0deg); }
            100% { transform: rotate(360deg); }
        }
        .info {
            background: #e3f2fd;
            border-left: 4px solid #2196f3;
            padding: 12px;
            margin: 15px 0;
            border-radius: 5px;
            font-size: 13px;
            color: #1976d2;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🔢 Digit Recognition</h1>
        <div class="subtitle">ESP32 + TensorFlow Lite</div>

        <div class="info">
            📝 Draw a <strong>single, large digit (0-9)</strong> on paper. Make it bold and clear! The image will be automatically cropped, centered, and inverted to match MNIST format.
        </div>

        <button class="btn btn-primary" onclick="startCamera()">📷 Take Photo</button>
        <button class="btn btn-secondary" onclick="document.getElementById('fileInput').click()">📁 Upload Image</button>
        <input type="file" id="fileInput" accept="image/*" onchange="handleFile(event)">

        <video id="video" autoplay playsinline></video>
        <button id="captureBtn" class="btn btn-primary" onclick="capturePhoto()" style="display:none;">Capture</button>

        <div class="preview-container" id="previewContainer" style="display:none;">
            <div class="preview-box">
                <h3>Original</h3>
                <canvas id="originalCanvas"></canvas>
            </div>
            <div class="preview-box">
                <h3>28×28 Processed</h3>
                <canvas id="canvas" width="28" height="28"></canvas>
            </div>
        </div>

        <div class="loading" id="loading">
            <div class="spinner"></div>
            <p style="margin-top: 10px;">Processing...</p>
        </div>

        <div class="result" id="result">
            <div class="result-label">Predicted Digit</div>
            <div class="result-digit" id="prediction">-</div>
        </div>
    </div>

    <script>
        let stream = null;
        const video = document.getElementById('video');
        const canvas = document.getElementById('canvas');
        const originalCanvas = document.getElementById('originalCanvas');
        const ctx = canvas.getContext('2d');
        const originalCtx = originalCanvas.getContext('2d');

        async function startCamera() {
            try {
                stream = await navigator.mediaDevices.getUserMedia({
                    video: { facingMode: 'environment' }
                });
                video.srcObject = stream;
                video.style.display = 'block';
                document.getElementById('captureBtn').style.display = 'block';
                document.getElementById('result').classList.remove('show');
            } catch (err) {
                alert('Camera access denied or not available: ' + err.message);
            }
        }

        function stopCamera() {
            if (stream) {
                stream.getTracks().forEach(track => track.stop());
                video.style.display = 'none';
                document.getElementById('captureBtn').style.display = 'none';
            }
        }

        function capturePhoto() {
            originalCanvas.width = video.videoWidth;
            originalCanvas.height = video.videoHeight;
            originalCtx.drawImage(video, 0, 0);
            stopCamera();
            processImage();
        }

        function handleFile(event) {
            const file = event.target.files[0];
            if (!file) {
                console.log('No file selected');
                return;
            }

            console.log('File selected:', file.name, file.type);

            const reader = new FileReader();
            reader.onload = function(e) {
                console.log('File loaded, creating image...');
                const img = new Image();
                img.onload = function() {
                    console.log('Image loaded, size:', img.width, 'x', img.height);
                    originalCanvas.width = img.width;
                    originalCanvas.height = img.height;
                    originalCtx.drawImage(img, 0, 0);
                    processImage();
                };
                img.onerror = function() {
                    console.error('Failed to load image');
                    alert('Failed to load image. Please try again.');
                };
                img.src = e.target.result;
            };
            reader.onerror = function() {
                console.error('Failed to read file');
                alert('Failed to read file. Please try again.');
            };
            reader.readAsDataURL(file);
        }

        function processImage() {
            // Show preview container
            document.getElementById('previewContainer').style.display = 'flex';

            // Step 1: Draw original to temporary canvas for processing
            const tempCanvas = document.createElement('canvas');
            const tempCtx = tempCanvas.getContext('2d');
            tempCanvas.width = originalCanvas.width;
            tempCanvas.height = originalCanvas.height;
            tempCtx.drawImage(originalCanvas, 0, 0);

            // Step 2: Convert to grayscale
            let imageData = tempCtx.getImageData(0, 0, tempCanvas.width, tempCanvas.height);
            let data = imageData.data;
            for (let i = 0; i < data.length; i += 4) {
                const gray = 0.299 * data[i] + 0.587 * data[i + 1] + 0.114 * data[i + 2];
                data[i] = data[i + 1] = data[i + 2] = gray;
            }
            tempCtx.putImageData(imageData, 0, 0);

            // Step 3: Detect if we need to invert (before resizing for better detection)
            let totalBrightness = 0;
            for (let i = 0; i < data.length; i += 4) {
                totalBrightness += data[i];
            }
            const avgBrightness = totalBrightness / (data.length / 4);
            const needsInversion = avgBrightness > 127;

            // Step 4: Invert if needed
            if (needsInversion) {
                for (let i = 0; i < data.length; i += 4) {
                    data[i] = data[i + 1] = data[i + 2] = 255 - data[i];
                }
                tempCtx.putImageData(imageData, 0, 0);
            }

            // Step 5: Find bounding box of the digit (crop to content)
            imageData = tempCtx.getImageData(0, 0, tempCanvas.width, tempCanvas.height);
            data = imageData.data;

            let minX = tempCanvas.width, minY = tempCanvas.height;
            let maxX = 0, maxY = 0;
            const threshold = 50; // Consider pixels > 50 as part of the digit

            for (let y = 0; y < tempCanvas.height; y++) {
                for (let x = 0; x < tempCanvas.width; x++) {
                    const idx = (y * tempCanvas.width + x) * 4;
                    if (data[idx] > threshold) {
                        if (x < minX) minX = x;
                        if (x > maxX) maxX = x;
                        if (y < minY) minY = y;
                        if (y > maxY) maxY = y;
                    }
                }
            }

            // Add padding around the digit (20% of the digit size)
            const digitWidth = maxX - minX;
            const digitHeight = maxY - minY;
            const padding = Math.max(digitWidth, digitHeight) * 0.2;

            minX = Math.max(0, minX - padding);
            minY = Math.max(0, minY - padding);
            maxX = Math.min(tempCanvas.width, maxX + padding);
            maxY = Math.min(tempCanvas.height, maxY + padding);

            const cropWidth = maxX - minX;
            const cropHeight = maxY - minY;

            // Check if we found any content
            if (cropWidth <= 0 || cropHeight <= 0 || !isFinite(cropWidth) || !isFinite(cropHeight)) {
                alert('Could not detect a digit in the image. Please try:\n- Drawing a larger, bolder digit\n- Better lighting\n- Higher contrast');
                document.getElementById('previewContainer').style.display = 'none';
                return;
            }

            // Step 6: Crop to bounding box
            const croppedCanvas = document.createElement('canvas');
            const croppedCtx = croppedCanvas.getContext('2d');
            croppedCanvas.width = cropWidth;
            croppedCanvas.height = cropHeight;
            croppedCtx.drawImage(tempCanvas, minX, minY, cropWidth, cropHeight, 0, 0, cropWidth, cropHeight);

            // Step 7: Resize to 20x20 (MNIST uses 20x20 digit, then centers in 28x28)
            const size20Canvas = document.createElement('canvas');
            const size20Ctx = size20Canvas.getContext('2d');
            const maxDim = Math.max(cropWidth, cropHeight);
            const scale = 25 / maxDim;
            const scaledWidth = cropWidth * scale;
            const scaledHeight = cropHeight * scale;
            size20Canvas.width = scaledWidth;
            size20Canvas.height = scaledHeight;
            size20Ctx.drawImage(croppedCanvas, 0, 0, scaledWidth, scaledHeight);

            // Step 8: Center in 28x28 canvas (like MNIST does)
            ctx.fillStyle = 'black';
            ctx.fillRect(0, 0, 28, 28);
            const offsetX = (28 - scaledWidth) / 2;
            const offsetY = (28 - scaledHeight) / 2;
            ctx.drawImage(size20Canvas, offsetX, offsetY);

            // Step 9: Apply aggressive threshold - pure black background
            let finalData = ctx.getImageData(0, 0, 28, 28);

            for (let i = 0; i < 784; i++) {
                let value = finalData.data[i * 4];

                // VERY aggressive threshold: only quite bright pixels survive
                if (value < 100) {
                    value = 0; // Pure black (0) - if it's not bright, it's black
                } else {
                    // Scale the remaining values to use full white range
                    value = Math.min(255, ((value - 100) / 155) * 255);
                }

                finalData.data[i * 4] = value;
                finalData.data[i * 4 + 1] = value;
                finalData.data[i * 4 + 2] = value;
            }

            // Step 10: Apply very subtle blur to round pixels
            const blurredData = ctx.createImageData(28, 28);

            for (let y = 0; y < 28; y++) {
                for (let x = 0; x < 28; x++) {
                    const idx = (y * 28 + x) * 4;
                    let sum = 0;
                    let totalWeight = 0;

                    // Very subtle weighted blur - center pixel has most weight
                    for (let dy = -1; dy <= 1; dy++) {
                        for (let dx = -1; dx <= 1; dx++) {
                            const nx = x + dx;
                            const ny = y + dy;
                            if (nx >= 0 && nx < 28 && ny >= 0 && ny < 28) {
                                const nIdx = (ny * 28 + nx) * 4;
                                // Center pixel gets weight 4, neighbors get weight 1
                                const weight = (dx === 0 && dy === 0) ? 4 : 1;
                                sum += finalData.data[nIdx] * weight;
                                totalWeight += weight;
                            }
                        }
                    }

                    const blurred = sum / totalWeight;
                    blurredData.data[idx] = blurred;
                    blurredData.data[idx + 1] = blurred;
                    blurredData.data[idx + 2] = blurred;
                    blurredData.data[idx + 3] = 255;
                }
            }

            ctx.putImageData(blurredData, 0, 0);

            // Update preview to show the final thresholded result
            ctx.putImageData(finalData, 0, 0);

            // Step 11: Extract pixel data for ESP32
            const pixels = new Float32Array(784);
            for (let i = 0; i < 784; i++) {
                pixels[i] = finalData.data[i * 4] / 255.0;
            }

            // Send to ESP32
            sendToESP32(pixels);
        }

        async function sendToESP32(pixels) {
            document.getElementById('loading').classList.add('show');
            document.getElementById('result').classList.remove('show');

            try {
                const response = await fetch('/predict', {
                    method: 'POST',
                    headers: {
                        'Content-Type': 'application/json',
                    },
                    body: JSON.stringify({ pixels: Array.from(pixels) })
                });

                const data = await response.json();

                document.getElementById('loading').classList.remove('show');

                if (data.success) {
                    document.getElementById('prediction').textContent = data.digit;
                    document.getElementById('result').classList.add('show');
                } else {
                    alert('Error: ' + data.error);
                }
            } catch (error) {
                document.getElementById('loading').classList.remove('show');
                alert('Failed to connect to ESP32: ' + error.message);
            }
        }
    </script>
</body>
</html>
)rawliteral";

void handleRoot() {
    server.send(200, "text/html", htmlPage);
}

void handlePredict() {
    if (server.method() != HTTP_POST) {
        server.send(405, "application/json", "{\"success\":false,\"error\":\"Method not allowed\"}");
        return;
    }

    String body = server.arg("plain");

    // Parse JSON manually (simple parsing for array of floats)
    int pixelsStart = body.indexOf("[");
    int pixelsEnd = body.indexOf("]");

    if (pixelsStart == -1 || pixelsEnd == -1) {
        server.send(400, "application/json", "{\"success\":false,\"error\":\"Invalid JSON\"}");
        return;
    }

    float imageData[784];
    String pixelsStr = body.substring(pixelsStart + 1, pixelsEnd);

    int idx = 0;
    int lastComma = -1;
    for (int i = 0; i <= pixelsStr.length(); i++) {
        if (i == pixelsStr.length() || pixelsStr.charAt(i) == ',') {
            if (idx >= 784) break;
            String valueStr = pixelsStr.substring(lastComma + 1, i);
            valueStr.trim();
            imageData[idx++] = valueStr.toFloat();
            lastComma = i;
        }
    }

    if (idx != 784) {
        server.send(400, "application/json",
            "{\"success\":false,\"error\":\"Expected 784 pixels, got " + String(idx) + "\"}");
        return;
    }

    // Run prediction
    if (!tf.predict(imageData).isOk()) {
        server.send(500, "application/json",
            "{\"success\":false,\"error\":\"Prediction failed\"}");
        return;
    }

    // Send result
    String response = "{\"success\":true,\"digit\":" + String(tf.classification) + "}";
    server.send(200, "application/json", response);
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n\n=== ESP32 Digit Recognition Web Server ===");

    // Initialize TensorFlow model
    Serial.println("Initializing TensorFlow Lite model...");
    tf.setNumInputs(784);
    tf.setNumOutputs(10);
    tf.resolver.AddConv2D();
    tf.resolver.AddMaxPool2D();
    tf.resolver.AddReshape();
    tf.resolver.AddFullyConnected();
    tf.resolver.AddSoftmax();

    while (!tf.begin(digits_model).isOk()) {
        Serial.println("ERROR: " + String(tf.exception.toString()));
        delay(1000);
    }
    Serial.println("✓ Model loaded successfully!");

    // Set up WiFi Access Point
    Serial.println("\nStarting WiFi Access Point...");
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ap_ssid, ap_password);

    IPAddress IP = WiFi.softAPIP();
    Serial.println("✓ Access Point started!");
    Serial.println("\n========== CONNECTION INFO ==========");
    Serial.println("WiFi Network: " + String(ap_ssid));
    Serial.println("Password: " + String(ap_password));
    Serial.println("IP Address: " + IP.toString());
    Serial.println("=====================================\n");
    Serial.println("Instructions:");
    Serial.println("1. Connect to WiFi: " + String(ap_ssid));
    Serial.println("2. Open browser and go to: http://" + IP.toString());
    Serial.println("3. Upload or capture a digit image!");
    Serial.println("\n=====================================\n");

    // Set up web server routes
    server.on("/", handleRoot);
    server.on("/predict", handlePredict);

    server.begin();
    Serial.println("✓ Web server started!\n");
}

void loop() {
    server.handleClient();
}
