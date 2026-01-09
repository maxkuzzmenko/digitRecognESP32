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

// Minified HTML (camera removed, optimized for flash storage)
const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
    <title>ESP32 Neural Link</title>
    <style>
        :root {
            --bg-grad: linear-gradient(135deg, #0f172a, #1e1b4b);
            --card-bg: rgba(30, 41, 59, 0.7);
            --primary: #818cf8;
            --primary-hover: #6366f1;
            --text-main: #f8fafc;
            --text-sub: #94a3b8;
            --glass: 1px solid rgba(255, 255, 255, 0.1);
        }

        * { margin: 0; padding: 0; box-sizing: border-box; }
        
        body {
            font-family: 'SF Pro Display', -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            background: var(--bg-grad);
            color: var(--text-main);
            height: 100vh;
            display: flex;
            align-items: center;
            justify-content: center;
            overflow: hidden; 
        }

        .wrapper {
            width: 100%;
            max-width: 900px;
            padding: 20px;
            height: 100vh;
            display: flex;
            flex-direction: column;
            justify-content: center;
        }

        .glass-card {
            background: var(--card-bg);
            backdrop-filter: blur(20px);
            -webkit-backdrop-filter: blur(20px);
            border: var(--glass);
            border-radius: 24px;
            box-shadow: 0 25px 50px -12px rgba(0, 0, 0, 0.5);
            display: flex;
            flex-direction: row;
            overflow: hidden;
            height: 500px; 
            position: relative;
        }

        .panel {
            padding: 30px;
            flex: 1;
            display: flex;
            flex-direction: column;
            justify-content: center;
            align-items: center;
            position: relative;
        }

        .panel-left {
            border-right: var(--glass);
            background: rgba(0,0,0,0.2);
            z-index: 2;
        }

        .panel-right {
            background: radial-gradient(circle at top right, rgba(129, 140, 248, 0.1), transparent);
            justify-content: flex-start;
            padding-top: 50px;
        }

        .canvas-container {
            position: relative;
            border-radius: 16px;
            box-shadow: 0 0 0 4px rgba(255,255,255,0.05);
            transition: transform 0.2s;
            background: #fff;
            overflow: hidden;
        }
        
        canvas#d {
            display: block;
            cursor: crosshair;
            touch-action: none;
        }

        .controls {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 10px;
            width: 280px;
            margin-top: 20px;
        }

        button {
            padding: 12px;
            border: none;
            border-radius: 10px;
            font-weight: 600;
            font-size: 14px;
            cursor: pointer;
            transition: all 0.2s;
            text-transform: uppercase;
            letter-spacing: 0.5px;
        }

        .btn-primary {
            background: var(--primary);
            color: #fff;
            box-shadow: 0 4px 15px rgba(129, 140, 248, 0.4);
        }
        .btn-primary:active { transform: scale(0.96); }

        .btn-secondary {
            background: rgba(255,255,255,0.1);
            color: var(--text-sub);
            border: 1px solid rgba(255,255,255,0.05);
        }
        .btn-secondary:hover { background: rgba(255,255,255,0.15); color: #fff; }

        .label {
            font-size: 12px;
            text-transform: uppercase;
            letter-spacing: 2px;
            color: var(--text-sub);
            margin-bottom: 10px;
        }

        .prediction-box {
            font-size: 120px;
            line-height: 1;
            font-weight: 800;
            background: linear-gradient(to bottom, #fff, #94a3b8);
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
            margin-bottom: 20px;
            height: 120px; 
            padding: 0 10px;
            display: flex;
            align-items: center;
            justify-content: center;
            background-clip: text;
        }

        .debug-view {
            flex: 1;
            width: 100%;
            display: flex;
            flex-direction: column;
            gap: 15px;
            align-items: center;
            justify-content: center;
            opacity: 0.8;
            background: rgba(0,0,0,0.1);
            border-radius: 16px;
            margin-top: 20px;
        }
        
        canvas#c {
            width: 140px;
            height: 140px;
            border-radius: 8px;
            border: 1px solid rgba(255,255,255,0.2);
            image-rendering: pixelated;
            box-shadow: 0 4px 20px rgba(0,0,0,0.2);
        }



        input[type="file"] { display: none; }

        @media (max-width: 768px) {
            body { 
                height: auto; 
                min-height: 100%; 
                overflow-y: auto;
                padding: 10px;
            }
            .wrapper { height: auto; display: block; }
            .glass-card {
                flex-direction: column;
                height: auto;
            }
            .panel-left { border-right: none; border-bottom: var(--glass); padding-bottom: 20px; }
            .panel-right { padding: 30px; align-items: center; }
            .prediction-box { font-size: 80px; height: 80px; }
        }
    </style>
</head>
<body>
    <div class="wrapper">
        <div class="glass-card">
            <!-- Left Panel: Interaction -->
            <div class="panel panel-left">
                <div class="canvas-container">
                    <canvas id="d" width="280" height="280"></canvas>
                </div>
                <div class="controls">
                    <button class="btn-secondary" onclick="clr()">Clear</button>
                    <button class="btn-primary" onclick="sub()">Recognize</button>
                    <button class="btn-secondary" onclick="document.getElementById('f').click()" style="grid-column: span 2">Upload Image</button>
                </div>
                <input type="file" id="f" accept="image/*" onchange="hf(event)">
            </div>

            <!-- Right Panel: Feedback -->
            <div class="panel panel-right">
                <div class="label">PREDICTION</div>
                <div class="prediction-box" id="p">-</div>
                <div class="debug-view">
                    <div style="font-size: 10px; letter-spacing: 2px; font-weight: 600; color: var(--text-sub)">PROCESSED INPUT</div>
                    <canvas id="c" width="28" height="28"></canvas>
                </div>
            </div>
        </div>
    </div>

    <script>
        const d=document.getElementById('d'),c=document.getElementById('c'),dx=d.getContext('2d'),cx=c.getContext('2d');
        const predEl = document.getElementById('p');
        
        let dr=0,lx=0,ly=0;
        
        // Init Canvas
        dx.fillStyle='#fff';
        dx.fillRect(0,0,d.width,d.height);
        dx.strokeStyle='#000';
        dx.lineWidth=20;
        dx.lineCap='round';
        dx.lineJoin='round';



        // Drawing Logic
        function st(e){dr=1;const r=d.getBoundingClientRect(),sx=d.width/r.width,sy=d.height/r.height;if(e.type.includes('mouse')){lx=(e.clientX-r.left)*sx;ly=(e.clientY-r.top)*sy}else{lx=(e.touches[0].clientX-r.left)*sx;ly=(e.touches[0].clientY-r.top)*sy}}
        function dw(e){if(!dr)return;e.preventDefault();const r=d.getBoundingClientRect(),sx=d.width/r.width,sy=d.height/r.height;let x,y;if(e.type.includes('mouse')){x=(e.clientX-r.left)*sx;y=(e.clientY-r.top)*sy}else{x=(e.touches[0].clientX-r.left)*sx;y=(e.touches[0].clientY-r.top)*sy}dx.beginPath();dx.moveTo(lx,ly);dx.lineTo(x,y);dx.stroke();lx=x;ly=y}
        function ed(){dr=0}
        
        d.addEventListener('mousedown',st);d.addEventListener('mousemove',dw);d.addEventListener('mouseup',ed);d.addEventListener('mouseout',ed);
        d.addEventListener('touchstart',st);d.addEventListener('touchmove',dw);d.addEventListener('touchend',ed);

        function clr(){
            dx.fillStyle='#fff';
            dx.fillRect(0,0,d.width,d.height);
            predEl.textContent = "-";

            // Clear debug view too
            cx.clearRect(0,0,28,28);
        }

        function sub(){proc(d)}

        function hf(e){
            const f=e.target.files[0];if(!f)return;
            const rd=new FileReader();
            rd.onload=function(e){const im=new Image();im.onload=function(){proc(im)};im.onerror=function(){alert('Failed to load')};im.src=e.target.result};
            rd.readAsDataURL(f);
        }

        function proc(src){

            
            // Image Processing Pipeline
            const t=document.createElement('canvas'),tx=t.getContext('2d');
            t.width=src.width;t.height=src.height;
            tx.drawImage(src,0,0,t.width,t.height);
            let id=tx.getImageData(0,0,t.width,t.height),dt=id.data;
            for(let i=0;i<dt.length;i+=4){const g=.299*dt[i]+.587*dt[i+1]+.114*dt[i+2];dt[i]=dt[i+1]=dt[i+2]=g}
            tx.putImageData(id,0,0);
            
            let tb=0;for(let i=0;i<dt.length;i+=4)tb+=dt[i];
            if(tb/(dt.length/4)>127){for(let i=0;i<dt.length;i+=4)dt[i]=dt[i+1]=dt[i+2]=255-dt[i];tx.putImageData(id,0,0)} // Invert if white bg
            
            id=tx.getImageData(0,0,t.width,t.height);dt=id.data;
            let mnx=t.width,mny=t.height,mxx=0,mxy=0;
            for(let y=0;y<t.height;y++){for(let x=0;x<t.width;x++){const ix=(y*t.width+x)*4;if(dt[ix]>20){if(x<mnx)mnx=x;if(x>mxx)mxx=x;if(y<mny)mny=y;if(y>mxy)mxy=y}}}
            
            const cw=mxx-mnx,ch=mxy-mny;
            if(cw<=0||ch<=0){
                alert("No digit detected!");
                return;
            }
            
            const cr=document.createElement('canvas'),crx=cr.getContext('2d');
            cr.width=cw;cr.height=ch;
            crx.drawImage(t,mnx,mny,cw,ch,0,0,cw,ch);
            
            const md=Math.max(cw,ch),sc=20/md,sw=cw*sc,sh=ch*sc,s2=document.createElement('canvas'),s2x=s2.getContext('2d');
            s2.width=sw;s2.height=sh;s2x.drawImage(cr,0,0,sw,sh);
            
            cx.fillStyle='#000';cx.fillRect(0,0,28,28);
            cx.drawImage(s2,(28-sw)/2,(28-sh)/2);
            
            const fd=cx.getImageData(0,0,28,28),px=new Float32Array(784);
            for(let i=0;i<784;i++)px[i]=fd.data[i*4]/255;
            
            snd(px);
        }

        async function snd(px){
            try{
                const rs=await fetch('/predict',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({pixels:Array.from(px)})});
                const dt=await rs.json();
                if(dt.success){
                    predEl.textContent=dt.digit;

                }else{
                    alert("Error: "+dt.error);
                }
            }catch(e){
                alert("Connection failed");
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
