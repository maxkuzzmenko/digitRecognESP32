# digitRecognESP32
digit recognition CNN on an ESP32

# How to use
1. Install the required libraries
2. Upload the project to the Arduino board (Or use Arduino IDE)<br />
   `arduino-cli compile --upload -p [Port] --fqbn esp32:esp32:esp32 [Project Path]`  <br />
   (for the port, see `arduino-cli board list`)
3. Connect to the board's WiFi: <br />
   SSID: `ESP32-Digit-Recognition` <br />
   Password: `12345678`
4. Go to board's web page:
Open any browser and go to http://192.168.4.1/
5. Draw a digit on the field and press the "Recognize Digit" button to, well, recognize the digit.
   Alternatively, you can try uploading an image

# Training
In this section, I'm referring to the code [here](https://colab.research.google.com/drive/1pWUb0X2E_uFTku0zPEwHjEUigY8v-js_?usp=sharing). <br />
It was originally trained on the MNIST dataset. But it showed poor performance recognizing our own digits. <br />
So I decided it would be better to make my own dataset. So I wrote a script that allows you to create your own dataset easily and efficiently.

To start creating your own dataset run `node server.js`, this will start a web server. Once the server starts, go to http://localhost:3000/ and you'll see the web page. <br />
*Note: If you don't have node js, you can download it [here](https://nodejs.org/en/download).*

From that point, it's quite self-explanatory. You draw a digit, it gets automatically gets processed and saved as `[timestamp]_[digit].png` in the `./create-dataset/dataset/[digit]` folder. <br /> 
You can also access the dataset in `create-dataset/dataset/`. <br />

The code is designed to get the model trained on the homemade dataset. To change that, edit the get_model() function after the instruction provided in the comments of the function itself.

# Notes about the model
The network consists of 4 layers, the input (28x28) - two hidden layers (32 - 32) - one output layer (10). <br />
It has **[26.506](## "28*28*32+32 + 32*32+32 + 32*10+10")**(hover for the details on calculation) parameters. And is trained on 1753 custom-drawn digit images. <br />

The board I'm using doesn't support quantisation, so I'm using float32 in this project. I know it's inefficient, but there's nothing I can/want to do at this point <br />

Other contribution is always welcome :D 
