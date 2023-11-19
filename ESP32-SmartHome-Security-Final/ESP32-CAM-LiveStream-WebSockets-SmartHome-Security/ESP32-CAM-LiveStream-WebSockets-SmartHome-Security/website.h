char website[] PROGMEM = R"=====(





<!DOCTYPE html>
<html>
    <head>
        <script>
            var newRed = 0;
            var newGreen = 0;
            var newBlue = 0;

            var red;
            var green;
            var blue;

            const connection = new WebSocket('ws://' + location.hostname + ':81/');
            const img = document.getElementById("stream-frame");
            let urlObject;
            connection.onopen = function () {
                console.log("WebSocket connection is open.");
                };
            connection.onmessage = function (event) {
                const full_data = event.data;
                if(full_data instanceof Blob){
                  if(urlObject){
                    URL.revokeObjectURL(urlObject);
                  }
                  urlObject = URL.createObjectURL(new Blob([full_data]));
                  document.getElementById("stream-frame").src = urlObject;
                  console.log("Received frame: " + urlObject);
                } else {
                  //console.log("Received data: " + full_data);
                  try {
                      const data = JSON.parse(full_data);
                      red = data.reddata;
                      green = data.greendata;
                      blue = data.bluedata;
                      document.getElementById("rate_red").value = red;
                      document.getElementById("rate_green").value = green;
                      document.getElementById("rate_blue").value = blue;
                      newRed = data.reddata * 4;
                      newGreen = data.greendata * 4;
                      newBlue = data.bluedata * 4;
                      document.getElementById("rate-red-val").textContent = newRed;
                      document.getElementById("rate-green-val").textContent = newGreen;
                      document.getElementById("rate-blue-val").textContent = newBlue;
                      colorBox();
                      document.getElementById("color-box").innerHTML = "RGB(" + newRed + ", " + newGreen + ", " + newBlue + ")";
                  } catch (error) {
                      console.error("Error parsing JSON data: " + error);
                  }
                }
            };
            connection.onclose = function () {
                console.log("WebSocket connection is closed.");
            };
            connection.onerror = function (error) {
                console.error("WebSocket error: " + error);
            };
            // Function to send data to the server
            function send_data() {
                var full_data = '{"RED":' + red + ',"GREEN":' + green + ',"BLUE":' + blue + '}';
                connection.send(full_data);
            }

            // Function to update the color
            function colorBox() {
                newRed = red * 4;
                newGreen = green * 4;
                newBlue = blue * 4;
                var avg = (newRed + newGreen + newBlue) / 3;
                if (avg < 123) {
                    document.getElementById("color-box").style.color = "white";
                    document.getElementById("color-box").style.backgroundColor = "rgb(" + (newRed - 1) + ", " + (newGreen - 1) + ", " + (newBlue - 1) + ")";
                } else if (avg >= 123) {
                    document.getElementById("color-box").style.color = "black";
                    document.getElementById("color-box").style.backgroundColor = "rgb(" + (newRed - 1) + ", " + (newGreen - 1) + ", " + (newBlue - 1) + ")";
                }
                if (newRed == 256) {
                    newRed = 255;
                }
                if (newGreen == 256) {
                    newGreen = 255;
                }
                if (newBlue == 256) {
                    newBlue = 255;
                }
            }

            // Function to update color based on the sliders
            function updateColor() {
                red = document.getElementById("rate_red").value;
                green = document.getElementById("rate_green").value;
                blue = document.getElementById("rate_blue").value;
                colorBox();
                document.getElementById("rate-red-val").textContent = newRed;
                document.getElementById("rate-green-val").textContent = newGreen;
                document.getElementById("rate-blue-val").textContent = newBlue;
                document.getElementById("color-box").innerHTML = "RGB(" + newRed + ", " + newGreen + ", " + newBlue + ")";
                send_data();
            }

            // Functions to turn on the RGB colors
            function red_on() {
                red = 64;
                green = 0;
                blue = 0;
                colorBox();
                document.getElementById("rate_red").value = (red * 4) - 1;
                document.getElementById("rate_green").value = green;
                document.getElementById("rate_blue").value = blue;
                document.getElementById("rate-red-val").textContent = (red * 4) - 1;
                document.getElementById("rate-green-val").textContent = green;
                document.getElementById("rate-blue-val").textContent = blue;
                document.getElementById("color-box").innerHTML = "RGB(" + (red * 4) + ", " + green + ", " + blue + ")";
                send_data();
            }

            function green_on() {
                red = 0;
                green = 64;
                blue = 0;
                colorBox();
                document.getElementById("rate_red").value = red;
                document.getElementById("rate_green").value = (green * 4) - 1;
                document.getElementById("rate_blue").value = blue;
                document.getElementById("rate-red-val").textContent = red;
                document.getElementById("rate-green-val").textContent = (green * 4) - 1;
                document.getElementById("rate-blue-val").textContent = blue;
                document.getElementById("color-box").innerHTML = "RGB(" + red + ", " + ((green * 4) - 1) + ", " + blue + ")";
                send_data();
            }

            function blue_on() {
                red = 0;
                green = 0;
                blue = 64;
                colorBox();
                document.getElementById("rate_red").value = red;
                document.getElementById("rate_green").value = green;
                document.getElementById("rate_blue").value = (blue * 4) - 1;
                document.getElementById("rate-red-val").textContent = red;
                document.getElementById("rate-green-val").textContent = green;
                document.getElementById("rate-blue-val").textContent = (blue * 4) - 1;
                document.getElementById("color-box").innerHTML = "RGB(" + red + ", " + green + ", " + ((blue * 4) - 1) + ")";
                send_data();
            }

            function rgb_off() {
                red = 0;
                green = 0;
                blue = 0;
                colorBox();
                document.getElementById("rate_red").value = red;
                document.getElementById("rate_green").value = green;
                document.getElementById("rate_blue").value = blue;
                document.getElementById("rate-red-val").textContent = red;
                document.getElementById("rate-green-val").textContent = green;
                document.getElementById("rate-blue-val").textContent = blue;
                document.getElementById("color-box").innerHTML = "RGB(" + red + ", " + green + ", " + blue + ")";
                send_data();
            }

            // Function to initialize the code when the DOM is ready
            document.addEventListener("DOMContentLoaded", function () {
                red = document.getElementById("rate_red").value;
                green = document.getElementById("rate_green").value;
                blue = document.getElementById("rate_blue").value;
            });

        </script>
        <style>
            button {
                display: inline-block;
                padding: 10px 20px;
                color: black;
                background-color: white;
                border: 2px solid black;
                text-decoration: none;
                font-size: 50px;
                width: 100%;
                height: 15%;
            }
            #redButton {
                background-color: red;
                border: 2px solid red;
            }
            #greenButton {
                background-color: green;
                border: 2px solid green;
            }
            #blueButton {
                background-color: blue;
                border: 2px solid blue;
            }
            .slideContainer {
                width: 100%;
            }
            .slider {
                -webkit-appearance: none;
                appearance: none;
                width: 100%;
                height: 36px;
                background: #d3d3d3;
                outline: none;
                opacity: 0.7;
                -webkit-transition: .2s;
                transition: opacity .2s;
            }
            .slider::hover {
                opacity: 1;
            }
            /* The slider handle (use -webkit- (Chrome, Opera, Safari, Edge) and -moz- (Firefox) to override default look) */
            #rate_red::-webkit-slider-thumb {
                -webkit-appearance: none; /* Override default look */
                appearance: none;
                width: 36px; /* Set a specific slider handle width */
                height: 36px; /* Slider handle height */
                background: red; /* Green background */
                cursor: pointer; /* Cursor on hover */
            }

            #rate_red::-moz-range-thumb {
                width: 25px; /* Set a specific slider handle width */
                height: 25px; /* Slider handle height */
                background: red; /* Green background */
                cursor: pointer; /* Cursor on hover */
            }
            #rate_blue::-webkit-slider-thumb {
                -webkit-appearance: none; /* Override default look */
                appearance: none;
                width: 36px; /* Set a specific slider handle width */
                height: 36px; /* Slider handle height */
                background: blue; /* Green background */
                cursor: pointer; /* Cursor on hover */
            }

            #rate_blue::-moz-range-thumb {
                width: 25px; /* Set a specific slider handle width */
                height: 25px; /* Slider handle height */
                background: blue; /* Green background */
                cursor: pointer; /* Cursor on hover */
            }
            #rate_green::-webkit-slider-thumb {
                -webkit-appearance: none; /* Override default look */
                appearance: none;
                width: 36px; /* Set a specific slider handle width */
                height: 36px; /* Slider handle height */
                background: green; /* Green background */
                cursor: pointer; /* Cursor on hover */
            }

            #rate_green::-moz-range-thumb {
                width: 25px; /* Set a specific slider handle width */
                height: 25px; /* Slider handle height */
                background: green; /* Green background */
                cursor: pointer; /* Cursor on hover */
            }
        </style>
    </head>
    <body>
        <center>
        <h1>ESP32-S3 RGB Controller Web Page</h1>
        <button id="redButton" onclick="red_on()">RED ON</button>
        <button id="greenButton" onclick="green_on()">GREEN ON</button>
        <button id="blueButton" onclick="blue_on()">BLUE ON</button>
        <button id="rgbOffButton" onclick="rgb_off()">RGB OFF</button>
        <!--<button id="resetWiFiManagerButton" href="/ResetWiFiManager">Reset WiFiManager</button>-->
        <br/><br/>
        <div class="slideContainer">
        <h3>RED <span id="rate-red-val"> 0 </span></h3><input type="range" id="rate_red" min="0" max="64" value="0" step="1" class="slider" onchange="updateColor()">
        <br/>
        <h3>GREEN <span id="rate-green-val">0</span></h3><input type="range" id="rate_green" min="0" max="64" value="0" step="1" class="slider" onchange="updateColor()">
        <br/>
        <h3>BLUE <span id="rate-blue-val">0</span></h3><input type="range" id="rate_blue" min="0" max="64" value="0" step="1" class="slider" onchange="updateColor()">
        <br/>
        </div>
        <h1 id="color-box" style="width:100%; height: 50%;">RGB(0, 0, 0)</h1>
        <img src="" id="stream-frame">
        </center>
    </body>
</html>







)=====";



char portal[] PROGMEM = R"=====(
  <!DOCTYPE html>
<html>
<head>
    <title>WiFi Config Portal</title>
    <style>
        /* Internal CSS for styling */
        body {
            font-family: Arial, sans-serif;
            background-color: #f0f0f0;
            text-align: center;
        }

        #header {
            background-color: #0073e6;
            color: #ffffff;
            padding: 20px;
        }

        #container {
            background-color: #ffffff;
            border: 1px solid #ccc;
            padding: 20px;
            border-radius: 5px;
            box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);
            width: 300px;
            margin: 0 auto;
        }

        h1 {
            font-size: 24px;
        }

        .chat-box {
            background-color: #f0f0f0;
            padding: 10px;
            border: 1px solid #ccc;
            border-radius: 5px;
            margin: 10px 0;
            text-align: left;
        }

        .user-message {
            background-color: #0073e6;
            color: #ffffff;
            text-align: right;
        }

        .message-input {
            width: 100%;
            padding: 10px;
            border: 1px solid #ccc;
            border-radius: 5px;
        }

        .send-button {
            background-color: #0073e6;
            color: #ffffff;
            border: none;
            padding: 10px 20px;
            border-radius: 5px;
            cursor: pointer;
        }
    </style>
</head>
<body>
    <div id="header">
        <h1>WiFi Configuration Portal</h1>
    </div>
    <div id="container">
        <div class="chat-box user-message">
            ChatGPT: Welcome to the WiFi Config Portal! How can I assist you?
        </div>
        <div class="chat-box">
            User: I'd like to configure my WiFi connection.
        </div>
        <div class="chat-box user-message">
            ChatGPT: Sure, please enter your WiFi credentials below.
        </div>
        <input type="text" class="message-input" placeholder="SSID (Network Name)">
        <input type="password" class="message-input" placeholder="Password">
        <button class="send-button">Connect</button>
    </div>
</body>
</html>

)====="