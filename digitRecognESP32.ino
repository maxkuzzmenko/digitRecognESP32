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
<!DOCTYPE html><html><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Digit Recognition</title><style>*{margin:0;padding:0;box-sizing:border-box}body{font-family:sans-serif;background:linear-gradient(135deg,#667eea,#764ba2);min-height:100vh;display:flex;justify-content:center;align-items:center;padding:20px}.c{background:#fff;border-radius:15px;padding:25px;max-width:450px;width:100%;box-shadow:0 10px 30px rgba(0,0,0,.3)}h1{color:#667eea;text-align:center;margin-bottom:20px;font-size:24px}.btn{width:100%;padding:12px;margin:8px 0;border:none;border-radius:8px;font-size:15px;font-weight:600;cursor:pointer}.b1{background:#667eea;color:#fff}.b2{background:#f0f0f0;color:#333}input[type=file]{display:none}.pv{display:none;text-align:center;margin:15px 0}.pv.s{display:block}.pv h3{color:#666;font-size:11px;margin-bottom:8px}canvas{border:2px solid #e0e0e0;border-radius:5px}#c{width:84px;height:84px;image-rendering:pixelated}.r{text-align:center;margin-top:15px;padding:15px;border-radius:8px;background:#f8f9fa;display:none}.r.s{display:block}.rd{font-size:60px;font-weight:bold;color:#667eea;margin:8px 0}.rl{color:#666;font-size:13px}.l{display:none;text-align:center;margin:15px 0;color:#667eea}.l.s{display:block}.sp{border:3px solid #f3f3f3;border-top:3px solid #667eea;border-radius:50%;width:35px;height:35px;animation:sp 1s linear infinite;margin:0 auto}@keyframes sp{to{transform:rotate(360deg)}}</style></head><body><div class="c"><h1>Digit Recognition</h1><div style="text-align:center;margin:15px 0"><canvas id="d" width="280" height="280" style="border:3px solid #667eea;border-radius:8px;background:#fff;cursor:crosshair;touch-action:none"></canvas></div><div style="display:flex;gap:8px"><button class="btn b1" onclick="sub()" style="flex:1">Recognize</button><button class="btn b2" onclick="clr()" style="flex:1">Clear</button></div><button class="btn b2" onclick="document.getElementById('f').click()" style="margin-top:10px">Upload Image</button><input type="file" id="f" accept="image/*" onchange="hf(event)"><div class="pv" id="pv"><h3>Processed (28x28)</h3><canvas id="c" width="28" height="28"></canvas></div><div class="l" id="l"><div class="sp"></div><p style="margin-top:8px">Processing...</p></div><div class="r" id="r"><div class="rl">Result</div><div class="rd" id="p">-</div></div></div><script>const d=document.getElementById('d'),c=document.getElementById('c'),dx=d.getContext('2d'),cx=c.getContext('2d');let dr=0,lx=0,ly=0;dx.fillStyle='#fff';dx.fillRect(0,0,d.width,d.height);dx.strokeStyle='#000';dx.lineWidth=20;dx.lineCap='round';dx.lineJoin='round';function st(e){dr=1;const r=d.getBoundingClientRect(),sx=d.width/r.width,sy=d.height/r.height;if(e.type.includes('mouse')){lx=(e.clientX-r.left)*sx;ly=(e.clientY-r.top)*sy}else{lx=(e.touches[0].clientX-r.left)*sx;ly=(e.touches[0].clientY-r.top)*sy}}function dw(e){if(!dr)return;e.preventDefault();const r=d.getBoundingClientRect(),sx=d.width/r.width,sy=d.height/r.height;let x,y;if(e.type.includes('mouse')){x=(e.clientX-r.left)*sx;y=(e.clientY-r.top)*sy}else{x=(e.touches[0].clientX-r.left)*sx;y=(e.touches[0].clientY-r.top)*sy}dx.beginPath();dx.moveTo(lx,ly);dx.lineTo(x,y);dx.stroke();lx=x;ly=y}function ed(){dr=0}function clr(){dx.fillStyle='#fff';dx.fillRect(0,0,d.width,d.height);document.getElementById('pv').classList.remove('s');document.getElementById('r').classList.remove('s')}function sub(){proc(d)}d.addEventListener('mousedown',st);d.addEventListener('mousemove',dw);d.addEventListener('mouseup',ed);d.addEventListener('mouseout',ed);d.addEventListener('touchstart',st);d.addEventListener('touchmove',dw);d.addEventListener('touchend',ed);function hf(e){const f=e.target.files[0];if(!f)return;const rd=new FileReader();rd.onload=function(e){const im=new Image();im.onload=function(){proc(im)};im.onerror=function(){alert('Failed to load image')};im.src=e.target.result};rd.onerror=function(){alert('Failed to read file')};rd.readAsDataURL(f)}function proc(src){document.getElementById('pv').classList.add('s');const t=document.createElement('canvas'),tx=t.getContext('2d');t.width=src.width;t.height=src.height;tx.drawImage(src,0,0,t.width,t.height);let id=tx.getImageData(0,0,t.width,t.height),dt=id.data;for(let i=0;i<dt.length;i+=4){const g=.299*dt[i]+.587*dt[i+1]+.114*dt[i+2];dt[i]=dt[i+1]=dt[i+2]=g}tx.putImageData(id,0,0);let tb=0;for(let i=0;i<dt.length;i+=4)tb+=dt[i];if(tb/(dt.length/4)>127){for(let i=0;i<dt.length;i+=4)dt[i]=dt[i+1]=dt[i+2]=255-dt[i];tx.putImageData(id,0,0)}id=tx.getImageData(0,0,t.width,t.height);dt=id.data;let mnx=t.width,mny=t.height,mxx=0,mxy=0;for(let y=0;y<t.height;y++){for(let x=0;x<t.width;x++){const ix=(y*t.width+x)*4;if(dt[ix]>20){if(x<mnx)mnx=x;if(x>mxx)mxx=x;if(y<mny)mny=y;if(y>mxy)mxy=y}}}const cw=mxx-mnx,ch=mxy-mny;if(cw<=0||ch<=0){alert('No digit detected');document.getElementById('pv').classList.remove('s');return}const cr=document.createElement('canvas'),crx=cr.getContext('2d');cr.width=cw;cr.height=ch;crx.drawImage(t,mnx,mny,cw,ch,0,0,cw,ch);const md=Math.max(cw,ch),sc=20/md,sw=cw*sc,sh=ch*sc,s2=document.createElement('canvas'),s2x=s2.getContext('2d');s2.width=sw;s2.height=sh;s2x.drawImage(cr,0,0,sw,sh);cx.fillStyle='#000';cx.fillRect(0,0,28,28);cx.drawImage(s2,(28-sw)/2,(28-sh)/2);const fd=cx.getImageData(0,0,28,28),px=new Float32Array(784);for(let i=0;i<784;i++)px[i]=fd.data[i*4]/255;snd(px)}async function snd(px){document.getElementById('l').classList.add('s');document.getElementById('r').classList.remove('s');try{const rs=await fetch('/predict',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({pixels:Array.from(px)})});const dt=await rs.json();document.getElementById('l').classList.remove('s');if(dt.success){document.getElementById('p').textContent=dt.digit;document.getElementById('r').classList.add('s')}else{alert('Error: '+dt.error)}}catch(e){document.getElementById('l').classList.remove('s');alert('Connection failed: '+e.message)}}</script></body></html>
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
