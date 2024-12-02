/*********
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/esp32-cam-take-photo-display-web-server/
  
  IMPORTANT!!! 
   - Select Board "AI Thinker ESP32-CAM"
   - GPIO 0 must be connected to GND to upload a sketch
   - After connecting GPIO 0 to GND, press the ESP32-CAM on-board RESET button to put your board in flashing mode
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*********/

#include "WiFi.h"
#include "esp_camera.h"
#include "esp_timer.h"
#include "img_converters.h"
#include "Arduino.h"
#include "soc/soc.h"           // Disable brownour problems
#include "soc/rtc_cntl_reg.h"  // Disable brownour problems
#include "driver/rtc_io.h"
#include <ESPAsyncWebServer.h>
#include <base64.h>
#include <ESP32Servo.h>

//CALIBRATION SETTINGS:

////////// 2:

// Left foot forward walking rotation Speed
int LFFWRS= 12;    // 0 = Slowest   90 = Fastest  Default = 12

// Right foot forward walking rotation Speed
int RFFWRS= 12;   // 0 = Slowest   90 = Fastest  Default = 12

////////// 3:

// Left foot Backward walking rotation Speed
int LFBWRS= 12;   // 0 = Slowest   90 = Fastest  Default = 12

// Right foot Backward walking rotation Speed
int RFBWRS= 12;   // 0 = Slowest   90 = Fastest  Default = 12

////////// 4:

// Left Leg standing Position
int LA0= 60;    // 0 = Full Tilt Right   180 = Full Tilt Left   Default = 60

// Right Leg standing position
int RA0= 120;   // 0 = Full Tilt Right   180 = Full Tilt Left   Default = 120

////////// 5:

// Left Leg tilt left walking position
int LATL= 85;   // 0 = Full Tilt Right   180 = Full Tilt Left   Default BASIC = 85   Default HUMANOID = 80

// Right Leg tilt left walking position
int RATL= 175;  // 0 = Full Tilt Right   180 = Full Tilt Left   Default BASIC = 175   Default HUMANOID = 150

// Left Leg tilt right walking position
int LATR= 5;   // 0 = Full Tilt Right   180 = Full Tilt Left   Default BASIC = 5   Default HUMANOID = 30

// Right Leg tilt right walking position
int RATR= 95;  // 0 = Full Tilt Right   180 = Full Tilt Left   Default BASIC = 95  Default HUMANOID = 100

////////// 6:

// Replace with your network credentials
const char* ssid = "WebWay";
const char* password = "123456789";

IPAddress Ip(192, 168, 1, 1);
IPAddress NMask(255, 255, 255, 0);

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// OV2640 camera module pins (CAMERA_MODEL_AI_THINKER)
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

#define LeftFoot 2 // Left Foot
#define LeftLeg 13 // Left leg
#define RightLeg 14 // Right leg
#define RightFoot 15 // Right Foot

Servo myservoLeftFoot;
Servo myservoLeftLeg;
Servo myservoRightFoot;
Servo myservoRightLeg;

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body {
      text-align: center;
      font-family: "Trebuchet MS", Arial;
      margin: 0 0 0 0;
      --color: rgb(255,255,255);
      --inv-color: rgb(0,0,0);
    }

    .light-mode {
      --color: rgb(0,0,0);
      --inv-color: rgb(255,255,255);
    }

    #photo-container{
      position: absolute;
      top: 0;
      left: 0;
      width: 100%;
      height: 100%;
    }

    #photo {
      height: 100%;
      width: 100%;
    }

    .rotated{
      transform:rotate(180deg);
    }

    .overlay-svg {
      position: absolute;
      top: 10%;
      left: 10%;
      width: 80%;
      height: 80%;
    }

    .time {
      position: absolute;
      top: 12%;
      width: 100%;
    }

    .theme-button {
      position: absolute;
      top: 15%;
      right: 25%;
      padding: 10px 10px 10px 10px;
      width: 60px;
      height: 60px;
      border-radius: 10px;

      svg {
        stroke: var(--color);
        fill: var(--inv-color);
      }
    }

    .rotate-button {
      position: absolute;
      top: 15%;
      right: 20%;
      padding: 10px 10px 10px 10px;
      width: 60px;
      height: 60px;
      border-radius: 10px;

      &:hover {
        background-color: var(--color);
        opacity: 50%;
        svg {
          stroke: var(--inv-color);
        }
      }

      svg {
        stroke: var(--color);
      }
    }

    .text {
      color: var(--color);
      font-size: 40px;
      opacity: 100%;
    }

    .button-container {
      display: flex;
      flex-direction: column;
      gap: 5%;
      position: absolute;
      top: 35%;
      left: 14%;
      height: 30%;
      width: 10%;
    }

    .button {
      background-color: unset;
      height: 50px;
      width: 100%;
      border-radius: 10px;
      display: flex;

      &:hover {
        background-color: var(--color);
        opacity: 50%;
        .text {
          color: var(--inv-color);
        }
      }

      & .text{
        margin: auto;
      }
    }

    .selected {
      background-color: var(--color);
      opacity: 100%;
      .text {
        color: var(--inv-color);
      }
      &:hover {
        opacity: 50%;
      }
    }
  </style>
</head>
<body id="body">
  <div id="photo-container">
    <img id="photo"/>
  </div>
  <svg
    viewBox="0 0 198.4375 132.29166"
    version="1.1"
    id="svg1"
    xmlns="http://www.w3.org/2000/svg"
    xmlns:svg="http://www.w3.org/2000/svg"
    class="overlay-svg"
    >
    <defs
      id="defs1" />
    <g
      id="layer1"
      transform="translate(0.52916503)">
      <g
        id="g15"
        transform="matrix(0.69710614,0,0,0.69710614,-11.021926,-10.098355)"
        style="stroke-width:1.13863576;stroke-dasharray:none">
        <ellipse
          style="fill:#ffffff;fill-opacity:0;stroke:var(--color);stroke-width:1.13863576;stroke-linecap:round;stroke-linejoin:round;stroke-dasharray:none"
          id="path1"
          cx="157.78625"
          cy="109.10783"
          rx="25.571575"
          ry="25.263485" />
        <g
          id="g4"
          transform="matrix(1.530704,0,0,1,-97.449133,1.0264033e-5)"
          style="stroke-width:0.92032074;stroke-dasharray:none">
          <path
            style="fill:#ffffff;fill-opacity:0;stroke:var(--color);stroke-width:0.92032074;stroke-linecap:round;stroke-linejoin:round;stroke-dasharray:none"
            d="m 177.24403,109.37241 h 6.37837"
            id="path3" />
          <path
            style="fill:#ffffff;fill-opacity:0;stroke:var(--color);stroke-width:0.92032074;stroke-linecap:round;stroke-linejoin:round;stroke-dasharray:none"
            d="m 183.47592,109.37241 h 6.37837"
            id="path4" />
        </g>
        <g
          id="g6"
          transform="matrix(1.530704,0,0,1,-149.09873,1.0264031e-5)"
          style="stroke-width:0.92032074;stroke-dasharray:none">
          <path
            style="fill:#ffffff;fill-opacity:0;stroke:var(--color);stroke-width:0.92032074;stroke-linecap:round;stroke-linejoin:round;stroke-dasharray:none"
            d="m 177.24403,109.37241 h 6.37837"
            id="path5" />
          <path
            style="fill:#ffffff;fill-opacity:0;stroke:var(--color);stroke-width:0.92032074;stroke-linecap:round;stroke-linejoin:round;stroke-dasharray:none"
            d="m 183.47592,109.37241 h 6.37837"
            id="path6" />
        </g>
        <g
          id="g12"
          transform="matrix(0,1.530704,-1,0,267.42324,-197.37968)"
          style="stroke-width:0.92032074;stroke-dasharray:none">
          <path
            style="fill:#ffffff;fill-opacity:0;stroke:var(--color);stroke-width:0.92032074;stroke-linecap:round;stroke-linejoin:round;stroke-dasharray:none"
            d="m 177.24403,109.37241 h 6.37837"
            id="path11" />
          <path
            style="fill:#ffffff;fill-opacity:0;stroke:var(--color);stroke-width:0.92032074;stroke-linecap:round;stroke-linejoin:round;stroke-dasharray:none"
            d="m 183.47592,109.37241 h 6.37837"
            id="path12" />
        </g>
        <g
          id="g14"
          transform="matrix(0,1.530704,-1,0,267.42324,-146.21142)"
          style="stroke-width:0.92032074;stroke-dasharray:none">
          <path
            style="fill:#ffffff;fill-opacity:0;stroke:var(--color);stroke-width:0.92032074;stroke-linecap:round;stroke-linejoin:round;stroke-dasharray:none"
            d="m 177.24403,109.37241 h 6.37837"
            id="path13" />
          <path
            style="fill:#ffffff;fill-opacity:0;stroke:var(--color);stroke-width:0.92032074;stroke-linecap:round;stroke-linejoin:round;stroke-dasharray:none"
            d="m 183.47592,109.37241 h 6.37837"
            id="path14" />
        </g>
      </g>
      <g
        id="g21">
        <path
          style="fill:#ffffff;fill-opacity:0;stroke:var(--color);stroke-width:2.11667;stroke-linecap:round;stroke-linejoin:round;stroke-dasharray:none"
          d="M 0.52916825,30.126831 V 1.0583333 l 54.47007375,1e-6"
          id="path15" />
        <path
          style="fill:#ffffff;fill-opacity:0;stroke:var(--color);stroke-width:2.11667;stroke-linecap:round;stroke-linejoin:round;stroke-dasharray:none"
          d="M 196.85,30.126831 V 1.0583333 l -54.47008,1e-6"
          id="path16" />
        <path
          style="fill:#ffffff;fill-opacity:0;stroke:var(--color);stroke-width:2.11667;stroke-linecap:round;stroke-linejoin:round;stroke-dasharray:none"
          d="m 0.52916825,102.16483 v 29.0685 H 54.999242"
          id="path17" />
        <path
          style="fill:#ffffff;fill-opacity:0;stroke:var(--color);stroke-width:2.11667;stroke-linecap:round;stroke-linejoin:round;stroke-dasharray:none"
          d="m 196.85,102.16483 v 29.0685 h -54.47008"
          id="path18" />
      </g>
      <g
        id="g22">
        <text
          xml:space="preserve"
          style="font-size:8.70193px;font-family:Ubuntu;-inkscape-font-specification:Ubuntu;text-align:center;writing-mode:lr-tb;direction:ltr;text-anchor:middle;fill:var(--color);fill-opacity:1;stroke:var(--color);stroke-width:0;stroke-linecap:round;stroke-linejoin:round;stroke-dasharray:none"
          x="27.107025"
          y="14.164822"
          id="text21"
          transform="scale(1.0141515,0.98604597)"><tspan
            id="tspan21"
            style="fill:var(--color);fill-opacity:1;stroke-width:0;stroke-dasharray:none"
            x="27.107025"
            y="14.164822">REC</tspan></text>
        <ellipse
          style="fill:#fe0000;fill-opacity:1;stroke:var(--color);stroke-width:0;stroke-linecap:round;stroke-linejoin:round;stroke-dasharray:none"
          id="path21"
          cx="12.21787"
          cy="10.994022"
          rx="3.1572769"
          ry="3.0208659" />
      </g>
    </g>
  </svg>

  <div class="time">
    <span class="text" id="clock"></span>
  </div>

  <div class="theme-button" onclick="document.getElementById('body').classList.toggle('light-mode');">
    <svg
      width="60"
      height="60"
      viewBox="0 0 6.3499999 6.35"
      version="1.1"
      id="svg1"
      stroke="current"
      fill="current"
      xmlns="http://www.w3.org/2000/svg"
      xmlns:svg="http://www.w3.org/2000/svg">
      <defs
        id="defs1" />
      <g
        id="layer1">
        <path
          style="fill-opacity:1;stroke-width:0.593989;stroke-linecap:round;stroke-linejoin:round"
          id="path1"
          d="M 3.1706061,6.0529757 A 2.8830538,2.8777454 0 0 1 0.67380834,4.614103 2.8830538,2.8777454 0 0 1 0.67380833,1.7363576 2.8830538,2.8777454 0 0 1 3.1706061,0.29748487 V 3.1752303 Z" />
        <path
          style="fill:var(--color);fill-opacity:1;stroke-width:0.593989;stroke-linecap:round;stroke-linejoin:round"
          id="path2"
          d="m -3.1701577,6.0524852 a 2.8830538,2.8777454 0 0 1 -2.4967978,-1.4388726 2.8830538,2.8777454 0 0 1 0,-2.8777455 2.8830538,2.8777454 0 0 1 2.4967978,-1.43887265 V 3.1747398 Z"
          transform="scale(-1,1)" />
      </g>
    </svg>
  </div>

  <div class="rotate-button" onclick="document.getElementById('photo-container').classList.toggle('rotated');">
    <svg
      width="60"
      height="60"
      viewBox="0 0 6.3499999 6.35"
      version="1.1"
      id="svg1"
      stroke="current"
      xmlns="http://www.w3.org/2000/svg"
      xmlns:svg="http://www.w3.org/2000/svg">
      <defs
        id="defs1">
        <marker
          style="overflow:visible"
          id="Triangle"
          refX="0"
          refY="0"
          orient="auto-start-reverse"
          markerWidth="1"
          markerHeight="1"
          viewBox="0 0 1 1"
          preserveAspectRatio="xMidYMid">
          <path
            transform="scale(0.5)"
            style="fill:context-stroke;fill-rule:evenodd;stroke:context-stroke;stroke-width:1pt"
            d="M 5.77,0 -2.88,5 V -5 Z"
            id="path135" />
        </marker>
      </defs>
      <g
        id="layer1">
        <path
          style="fill:#474747;fill-opacity:0;stroke-width:0.430338;stroke-linecap:round;stroke-linejoin:round;stroke-dasharray:none;marker-start:url(#Triangle)"
          id="path4"
          d="M -2.0711025,1.5706284 A 2.7034926,2.4660258 0 0 1 0.31990284,3.8487053 2.7034926,2.4660258 0 0 1 -1.7000482,6.4082834 2.7034926,2.4660258 0 0 1 -4.8271936,5.0630054"
          transform="matrix(0.15733141,-0.98754586,0.97649435,0.21554301,0,0)" />
      </g>
    </svg>
  </div>

  <div class="button-container">
    <div
      id="bt1"
      class="button"
      onclick="document.getElementById('bt1').classList.toggle('selected');
      document.getElementById('bt2').classList.remove('selected');
      document.getElementById('bt3').classList.remove('selected');
      document.getElementById('bt4').classList.remove('selected');
      document.getElementById('bt5').classList.remove('selected');
      var xhr = new XMLHttpRequest();xhr.open('POST', '/stop', true);xhr.send();"
    >
      <div class="text">
        STOP
      </div>
    </div>
    <div
      id="bt2"
      class="button"
      onclick="document.getElementById('bt2').classList.toggle('selected');
      document.getElementById('bt1').classList.remove('selected');
      document.getElementById('bt3').classList.remove('selected');
      document.getElementById('bt4').classList.remove('selected');
      document.getElementById('bt5').classList.remove('selected');
      var xhr = new XMLHttpRequest();xhr.open('POST', '/walk', true);xhr.send();"
    >
      <div class="text">
        FWD
      </div>
    </div>
    <div
      id="bt3"
      class="button"
      onclick="document.getElementById('bt3').classList.toggle('selected');
      document.getElementById('bt1').classList.remove('selected');
      document.getElementById('bt2').classList.remove('selected');
      document.getElementById('bt4').classList.remove('selected');
      document.getElementById('bt5').classList.remove('selected');
      var xhr = new XMLHttpRequest();xhr.open('POST', '/back', true);xhr.send();"
    >
      <div class="text">
        BWD
      </div>
    </div>
    <div
      id="bt4"
      class="button"
      onclick="document.getElementById('bt4').classList.toggle('selected');
      document.getElementById('bt1').classList.remove('selected');
      document.getElementById('bt2').classList.remove('selected');
      document.getElementById('bt3').classList.remove('selected');
      document.getElementById('bt5').classList.remove('selected');
      var xhr = new XMLHttpRequest();xhr.open('POST', '/rot_clk', true);xhr.send();"
    >
      <div class="text">
        ROT CLK
      </div>
    </div>
    <div
      id="bt5"
      class="button"
      onclick="document.getElementById('bt5').classList.toggle('selected');
      document.getElementById('bt1').classList.remove('selected');
      document.getElementById('bt2').classList.remove('selected');
      document.getElementById('bt3').classList.remove('selected');
      document.getElementById('bt4').classList.remove('selected');
      var xhr = new XMLHttpRequest();xhr.open('POST', '/rot_iclk', true);xhr.send();"
    >
      <div class="text">
        ROT ICLK
      </div>
    </div>
  </div>
</body>
<script>
  (function () {

    var clockElement = document.getElementById( "clock" );

    function updateClock ( clock ) {
      clock.innerHTML = new Date().toLocaleTimeString();
    }

    function capturePhoto() {
      var xhr = new XMLHttpRequest();
      xhr.onreadystatechange = function() {
        if (this.status == 200) {
          let canvas = document.getElementById("photo");
          if (this.responseText.length > 0) {
            let base64 = this.responseText;
            var base64regex = /^([0-9a-zA-Z+/]{4})*(([0-9a-zA-Z+/]{2}==)|([0-9a-zA-Z+/]{3}=))?$/;
            if (base64regex.test(base64)) {
              canvas.src = "data:image/jpeg;base64," + base64;
            }
          }
        }
      };
      xhr.open('GET', "/capture", true);
      xhr.send();
    }

    setInterval(function () {
        updateClock( clockElement );
    }, 1000);

    setInterval(capturePhoto, 100);
  }());
</script>
</html>)rawliteral";

enum action {FWD, BWD, ROTCLK, ROTICLK, STOP};
enum action do_action = STOP;

void Home()
{ 
  myservoLeftLeg.attach(LeftLeg, 544, 2400); 
  myservoRightLeg.attach(RightLeg, 544, 2400);
  myservoLeftFoot.attach(LeftFoot, 544, 2400); 
  myservoRightFoot.attach(RightFoot, 544, 2400);  
  myservoLeftLeg.write(LA0); 
  myservoRightLeg.write(RA0);
  myservoLeftFoot.write(90);
  myservoRightFoot.write(90);  
  delay(400);
  myservoLeftLeg.detach();
  myservoRightLeg.detach();
  myservoLeftFoot.detach();
  myservoRightFoot.detach();
}

int currentmillis1 = 0;

void Walk2() {

    // Move legs up
    myservoRightLeg.attach(RightLeg, 544, 2400);
    myservoLeftLeg.attach(LeftLeg, 544, 2400);
    myservoLeftLeg.write(100);
    myservoRightLeg.write(180);
    myservoLeftLeg.detach();
    myservoRightLeg.detach();
    delay(1000);

    // Rotate left leg
    myservoLeftFoot.attach(LeftFoot, 544, 2400);
    myservoLeftFoot.write(90);
    myservoLeftFoot.detach();
    delay(1000);

    // Move legs down
    myservoRightLeg.attach(RightLeg, 544, 2400);
    myservoLeftLeg.attach(LeftLeg, 544, 2400);
    myservoLeftLeg.write(60);
    myservoRightLeg.write(120);
    myservoLeftLeg.detach();
    myservoRightLeg.detach();
    delay(1000);

    // Move legs up
    myservoRightLeg.attach(RightLeg, 544, 2400);
    myservoLeftLeg.attach(LeftLeg, 544, 2400);
    myservoLeftLeg.write(0);
    myservoRightLeg.write(80);
    myservoLeftLeg.detach();
    myservoRightLeg.detach();
    delay(1000);

    // Rotate left leg
    myservoRightFoot.attach(RightFoot, 544, 2400);
    myservoRightFoot.write(90);
    myservoRightFoot.detach();
    delay(1000);

    // Move legs down
    myservoRightLeg.attach(RightLeg, 544, 2400);
    myservoLeftLeg.attach(LeftLeg, 544, 2400);
    myservoLeftLeg.write(60);
    myservoRightLeg.write(120);
    myservoLeftLeg.detach();
    myservoRightLeg.detach();
    delay(1000);
}

void Walk() {
  int lt= map(0, 100, -100, 200, 700); 
  int rt= map(0, 100, -100, 700, 200); 
  int Interval1 = 250;
  int Interval2 = 250 + rt;
  int Interval3 = 250 + rt + 250;
  int Interval4 = 250 + rt + 250 + lt;
  int Interval5 = 250 + rt + 250 + lt + 50;
  
  if(millis() > currentmillis1 + Interval5)
  {
  currentmillis1 = millis();
  }
  
  
  if(millis() - currentmillis1 <= Interval1)
  {   
      myservoLeftLeg.attach(LeftLeg, 544, 2400);
      myservoRightLeg.attach(RightLeg, 544, 2400);
      myservoRightFoot.attach(RightFoot, 544, 2400);  
      myservoLeftFoot.attach(LeftFoot, 544, 2400); 
      
      myservoLeftLeg.write(LATR); 
      myservoRightLeg.write(RATR);
  }
      
  if((millis() - currentmillis1 >= Interval1)&&(millis() - currentmillis1 <= Interval2))
  {      
      myservoRightFoot.write(90-RFFWRS);
      
  }

  if((millis() - currentmillis1 >= Interval2)&&(millis() - currentmillis1 <= Interval3))
  {  
      myservoRightFoot.detach();
      myservoLeftLeg.write(LATL); 
      myservoRightLeg.write(RATL);
  }

  if((millis() - currentmillis1 >= Interval3)&&(millis() - currentmillis1 <= Interval4))
  {       
      myservoLeftFoot.write(90+LFFWRS);      
  }  

  if((millis() - currentmillis1 >= Interval4)&&(millis() - currentmillis1 <= Interval5))
  {
  myservoLeftFoot.detach();  
  }
}

void RotCLK() {
  int lt= map(-100, 100, -100, 200, 700); 
  int rt= map(100, 100, -100, 700, 200); 
  int Interval1 = 250;
  int Interval2 = 250 + rt;
  int Interval3 = 250 + rt + 250;
  int Interval4 = 250 + rt + 250 + lt;
  int Interval5 = 250 + rt + 250 + lt + 50;
  
  if(millis() > currentmillis1 + Interval5)
  {
  currentmillis1 = millis();
  }
  
  
  if(millis() - currentmillis1 <= Interval1)
  {   
      myservoLeftLeg.attach(LeftLeg, 544, 2400);
      myservoRightLeg.attach(RightLeg, 544, 2400);
      myservoRightFoot.attach(RightFoot, 544, 2400);  
      myservoLeftFoot.attach(LeftFoot, 544, 2400); 
      
      myservoLeftLeg.write(LATR); 
      myservoRightLeg.write(RATR);
  }
      
  if((millis() - currentmillis1 >= Interval1)&&(millis() - currentmillis1 <= Interval2))
  {      
      myservoRightFoot.write(90-RFFWRS);
      
  }

  if((millis() - currentmillis1 >= Interval2)&&(millis() - currentmillis1 <= Interval3))
  {  
      myservoRightFoot.detach();
      myservoLeftLeg.write(LATL); 
      myservoRightLeg.write(RATL);
  }

  if((millis() - currentmillis1 >= Interval3)&&(millis() - currentmillis1 <= Interval4))
  {       
      myservoLeftFoot.write(90+LFFWRS);      
  }  

  if((millis() - currentmillis1 >= Interval4)&&(millis() - currentmillis1 <= Interval5))
  {
  myservoLeftFoot.detach();  
  }
}

void RotICLK() {
  int lt= map(100, 100, -100, 200, 700); 
  int rt= map(-100, 100, -100, 700, 200); 
  int Interval1 = 250;
  int Interval2 = 250 + rt;
  int Interval3 = 250 + rt + 250;
  int Interval4 = 250 + rt + 250 + lt;
  int Interval5 = 250 + rt + 250 + lt + 50;
  
  if(millis() > currentmillis1 + Interval5)
  {
  currentmillis1 = millis();
  }
  
  
  if(millis() - currentmillis1 <= Interval1)
  {   
      myservoLeftLeg.attach(LeftLeg, 544, 2400);
      myservoRightLeg.attach(RightLeg, 544, 2400);
      myservoRightFoot.attach(RightFoot, 544, 2400);  
      myservoLeftFoot.attach(LeftFoot, 544, 2400); 
      
      myservoLeftLeg.write(LATR); 
      myservoRightLeg.write(RATR);
  }
      
  if((millis() - currentmillis1 >= Interval1)&&(millis() - currentmillis1 <= Interval2))
  {      
      myservoRightFoot.write(90-RFFWRS);
      
  }

  if((millis() - currentmillis1 >= Interval2)&&(millis() - currentmillis1 <= Interval3))
  {  
      myservoRightFoot.detach();
      myservoLeftLeg.write(LATL); 
      myservoRightLeg.write(RATL);
  }

  if((millis() - currentmillis1 >= Interval3)&&(millis() - currentmillis1 <= Interval4))
  {       
      myservoLeftFoot.write(90+LFFWRS);      
  }  

  if((millis() - currentmillis1 >= Interval4)&&(millis() - currentmillis1 <= Interval5))
  {
  myservoLeftFoot.detach();  
  }
}

void Back() {
  int lt= map(0, 100, -100, 200, 700); 
  int rt= map(0, 100, -100, 700, 200); 
  int Interval1 = 250;
  int Interval2 = 250 + rt;
  int Interval3 = 250 + rt + 250;
  int Interval4 = 250 + rt + 250 + lt;
  int Interval5 = 250 + rt + 250 + lt + 50;
  
  if(millis() > currentmillis1 + Interval5)
  {
  currentmillis1 = millis();
  }
  
  
  if(millis() - currentmillis1 <= Interval1)
  {   
      myservoLeftLeg.attach(LeftLeg, 544, 2400);
      myservoRightLeg.attach(RightLeg, 544, 2400);
      myservoRightFoot.attach(RightFoot, 544, 2400);  
      myservoLeftFoot.attach(LeftFoot, 544, 2400); 
      
      myservoLeftLeg.write(LATR); 
      myservoRightLeg.write(RATR);
  }
      
  if((millis() - currentmillis1 >= Interval1)&&(millis() - currentmillis1 <= Interval2))
  {      
      myservoRightFoot.write(90+RFFWRS);
      
  }

  if((millis() - currentmillis1 >= Interval2)&&(millis() - currentmillis1 <= Interval3))
  {  
      myservoRightFoot.detach();
      myservoLeftLeg.write(LATL); 
      myservoRightLeg.write(RATL);
  }

  if((millis() - currentmillis1 >= Interval3)&&(millis() - currentmillis1 <= Interval4))
  {       
      myservoLeftFoot.write(90-LFFWRS);      
  }  

  if((millis() - currentmillis1 >= Interval4)&&(millis() - currentmillis1 <= Interval5))
  {
  myservoLeftFoot.detach();  
  }
}

void walk(AsyncWebServerRequest *request) {
  Serial.println("Walk");
  do_action = FWD;
}

void back(AsyncWebServerRequest *request) {
  Serial.println("Walk backwards");
  do_action = BWD;
}

void rot_clock(AsyncWebServerRequest *request) {
  Serial.println("Rot Clock");
  do_action = ROTCLK;
}

void rot_inv_clock(AsyncWebServerRequest *request) {
  Serial.println("Rot Inverse Clock");
  do_action = ROTICLK;
}

void stop(AsyncWebServerRequest *request) {
  Serial.println("Stop");
  do_action = STOP;
}

void setup() {
  // Serial port for debugging purposes
  Serial.begin(115200);

  // Connect to Wi-Fi
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  delay(100);
  WiFi.softAPConfig(Ip, Ip, NMask);

  // Print ESP32 Local IP Address
  Serial.print("IP Address: http://");
  Serial.println(WiFi.softAPIP());

  Home();

  // Turn-off the 'brownout detector'
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  // OV2640 camera module
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  // Small image in order to have high frequency
  config.frame_size = FRAMESIZE_CIF;
  config.jpeg_quality = 12;
  config.fb_count = 4;

  // Camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    ESP.restart();
  }

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/html", index_html);
  });

  server.on("/capture", HTTP_GET, [](AsyncWebServerRequest * request) {
    // takeNewPhoto = true;
    camera_fb_t * fb = NULL; // pointer
    fb = esp_camera_fb_get();

    if (fb) {
      String encoded = base64::encode(fb->buf, fb->len);
      const char* img = encoded.c_str();
      request->send_P(200, "text/plain", img);
    }

    esp_camera_fb_return(fb);
  });

  server.on("/walk", HTTP_POST, walk);
  server.on("/back", HTTP_POST, back);
  server.on("/stop", HTTP_POST, stop);
  server.on("/rot_clk", HTTP_POST, rot_clock);
  server.on("/rot_iclk", HTTP_POST, rot_inv_clock);

  // Start server
  server.begin();
}

void loop() {
  switch (do_action)
  {
  case FWD:
    Walk();
    break;
  case BWD:
    Back();
    break;
  case ROTCLK:
    RotCLK();
    break;
  case ROTICLK:
    RotICLK();
    break;
  default:
    Home();
    break;
  }
  delay(10);
}
