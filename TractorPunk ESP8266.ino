/* JoyStick WiFi escrito por Dani No www.esploradores.com

Este sofware está escrito bajo la licencia CREATIVE COMMONS con Reconocimiento - CompartirIgual
(CC BY-SA) https://creativecommons.org/
Basado en el muti-touch game controler de Seb Lee-Delisle (C)2010-2011, www.sebleedelisle.com

-Redistributions of source code must retain the above creative commons and copyright notices, this
 list of conditions and the following disclaimer.
-Redistributions in binary form must reproduce the above creative commons and copyright notices, this 
 list of conditions and the following disclaimer in the documentation and/or other materials provided 
 with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED 
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE 
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN 
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <ESP8266WiFi.h>
#include <WebSocketsServer.h>
#include <ESP8266WebServer.h>

#define IZQ_PWM     16 // D0 (PWMB)           
#define IZQ_AVZ     05 // D1 (BIN2)         
#define IZQ_RET     04 // D2 (BIN1)
#define STBY        02 // D4 (STBY)
#define DER_RET     14 // D5 (AIN1)
#define DER_AVZ     12 // D6 (AIN2)
#define DER_PWM     13 // D7 (PWMA)

#define PI 3.141593

const char* ssid = "ZumoCam";
const char* password = "123456789";

static const char PROGMEM INDEX_HTML[] = R"( 
  <!doctype html>
  <html>
  <head>
  <meta charset=utf-8>
  <meta name='viewport' content='width=device-width, height=device-height, initial-scale=1.0, maximum-scale=1.0'/>
  <title>JoyStick</title>

  <style> 
    *{-webkit-touch-callout: none; -webkit-text-size-adjust: none; -webkit-tap-highlight-color: #FFFFFF; -webkit-user-select: none; -webkit-tap-highlight-color: #FFFFFF;}
    body {margin: 0px;}
    canvas {display:block; position:absolute}
    .containerButtons {width: auto; display: flex; justify-content: space-around;} 
    .buttons {border: none; color: white; padding: 4px 4px; text-align: center; text-decoration: none; display: inline-block; font-size: 4.0vmin; font-weight: bold; margin: 4px 2px; 
            -webkit-transition-duration: 0.4s; transition-duration: 0.4s; cursor: pointer; width: 25%; border-radius: 2px;}
    .btn1 {background-color: #808080; color: #FFFFFF; border: 3px solid #808080;} .btn1:hover {background-color: #F2F2F2; color: #000000; border: 3px solid #FF0000;}
    .btn2 {background-color: #1E90FF; color: #FFFFFF; border: 3px solid #1E90FF;} .btn2:hover {background-color: #F2F2F2; color: #000000; border: 3px solid #FF0000;}
    .data {padding: 2px 4px; background-color: #F2F2F2; color: #000000; font-size: 3.5vmin; border: 2px solid #000000;}
    .containerCanvas {width: auto;}
  </style>

  <body id='body'>
  <div class='containerButtons'>
    <button class='buttons btn1' onclick='onStatic()' autofocus>Estático</button>
    <button class='buttons btn2' onclick='onDynamic()'>Dinámico</button>
    <p id='txt1_id' class='buttons data'></p>
    <p id='txt2_id' class='buttons data'></p>
  </div>

  <script>

   ///////VARIABLES GLOBALES Y OPERACIONES CON VECTORES ///////
  var canvas, c , containerCanvas, ratioWindow, ladoMenor;
  var estatico = true, touchStart = false, mouseDown  = false;
  var angRad, angGrd, vectorHypot, speed;
  var vector2 = function (x,y) {this.x= x || 0; this.y = y || 0;};

  vector2.prototype = {
    reset:      function ( x, y )   {this.x = x;    this.y = y;     return this;},
    copyFrom :  function (v)        {this.x = v.x;  this.y = v.y;   return this;},  
    minusEq :   function (v)        {this.x-= v.x;  this.y-= v.y;   return this;},
  };
    var pos = new vector2(0,0), startPos = new vector2(0,0), vector = new vector2(0,0), posMax = new vector2(0,0); 

  var connection = new WebSocket('ws://'+location.hostname+':81/', ['arduino']);
    connection.onopen =     function ()         {connection.send('Connect ' + new Date()); };
    connection.onerror =    function (error)    {console.log('WebSocket Error ', error);};
    connection.onmessage =  function (event)    {console.log('Server: ', event.data);};

  ///////CANVAS///////
  setupCanvas();
  function setupCanvas() {
  canvas = document.createElement('canvas');
    c = canvas.getContext('2d');
    resetAll();
    containerCanvas = document.createElement('div');
    containerCanvas.className = "containerCanvas";
    document.body.appendChild(containerCanvas);
    containerCanvas.appendChild(canvas);
  }
  function resetAll () {
    var buttonsHeight = Math.min(window.innerWidth, window.innerHeight)*0.04+44; 
    canvas.width = window.innerWidth; canvas.height = window.innerHeight-buttonsHeight;
    ratioWindow = canvas.width/canvas.height;  ladoMenor=Math.min(canvas.width, canvas.height); 
    window.scrollTo(0,0);
    c.font = Math.round(ladoMenor*0.03) + 'px sans-serif';
    c.fillStyle = "white";
    c.textAlign="center"; 
    estatico? onStatic() : onDynamic();
  }
  function resetByOrientationchange () {
    document.body.removeChild(containerCanvas);   
    setTimeout(function(){
      setupCanvas();
      startEventListener();
      }, 450);
  }

  ///////DETECCIÓN DE EVENTOS Y DETERMINACIÓN DEL TIPO DE DISPOSITIVO SEÑALADOR (TÁCTIL O RATÓN)///////
  var touchable = 'createTouch' in document; 
  startEventListener();
  function startEventListener(){
    window.onresize = resetAll;
    if(touchable) {
      canvas.addEventListener( 'touchstart', onTouchStart, false);
      canvas.addEventListener( 'touchmove', onTouchMove, false);
      canvas.addEventListener( 'touchend', onTouchEnd, false);
      window.onorientationchange = resetByOrientationchange;  
    } else {
      canvas.addEventListener( 'mousedown', onMouseDown, false);
      canvas.addEventListener( 'mousemove', onMouseMove, false);
      ['mouseup', 'mouseout'].forEach(function(event){canvas.addEventListener(event,onMouseEnd,false);});
    }
  }

  ///////ESTÁTICO / DINÁMICO///////
  function onStatic() {
    estatico = true;
    document.getElementById('body').style.background= '#CECECE';
    c.strokeStyle = '#808080';
    drawOnCanvasStartOrMoveEnd();
  }
  function onDynamic() {
    estatico = false;
    document.getElementById('body').style.background= '#ADD8E6';
    c.strokeStyle = '#1E90FF';
    drawOnCanvasStartOrMoveEnd();
  }

///////GESTIÓN DE DISPOSITIVO TÁCTIL///////
  function onTouchStart(event) {
    touchStart = true;
    if (estatico){
      pos.reset(event.touches[0].clientX-canvas.getBoundingClientRect().left, event.touches[0].clientY-canvas.getBoundingClientRect().top);   
      vector.copyFrom(pos); 
      vector.minusEq(startPos);
    } else {
      startPos.reset(event.touches[0].clientX-canvas.getBoundingClientRect().left, event.touches[0].clientY-canvas.getBoundingClientRect().top);
      pos.copyFrom(startPos); 
    }
  } 
  function onTouchMove(event) {
    if (touchStart){
      event.preventDefault();
      pos.reset(event.touches[0].clientX-canvas.getBoundingClientRect().left, event.touches[0].clientY-canvas.getBoundingClientRect().top);   
      vector.copyFrom(pos); 
      vector.minusEq(startPos);   
    }
  }   
  function onTouchEnd(event) {
    touchStart = false;
    drawOnCanvasStartOrMoveEnd();
    sendData ();
  }   

  ///////GESTIÓN DEL RATÓN///////
  function onMouseDown(event) {
    event.preventDefault();
    mouseDown = true;
    if (estatico){
      pos.reset(event.offsetX, event.offsetY); 
      vector.copyFrom(pos); 
      vector.minusEq(startPos); 
    } else {
      startPos.reset(event.offsetX, event.offsetY);
      pos.copyFrom(startPos); 
    }
  }
  function onMouseMove(event) {
    if(mouseDown){
      pos.reset(event.offsetX, event.offsetY); 
      vector.copyFrom(pos); 
      vector.minusEq(startPos);
    }
  }
  function onMouseEnd(event) {
    mouseDown = false;
    drawOnCanvasStartOrMoveEnd();
    sendData ();
  }

  ///////DIBUJO DE LA BASE Y STICK///////
  setInterval(drawOnMove, 1000/50); 
  function drawOnMove() {
    if(touchStart || mouseDown) {
      c.clearRect(0,0,canvas.width, canvas.height);
      drawBase (startPos.x, startPos.y);
      newValues ();
      drawStick ();
      drawText ();
      sendData ();
    }
  }
  function drawOnCanvasStartOrMoveEnd(){
    c.clearRect(0,0,canvas.width, canvas.height);
    startPos.reset(canvas.width/2,canvas.height/2);
    vector.reset(0,0);
    estatico? drawBase() : c.fillText('TOCA LA PANTALLA PARA COMENZAR',canvas.width/2,canvas.height/2);
    newValues ();
    drawText ();
  }
  function newValues(){
    angRad = Math.atan2(-vector.y,vector.x).toFixed(2);    
    angGrd = Math.round(angRad*180/Math.PI);
    vectorHypot = Math.hypot(vector.x,vector.y);
    posMax.reset(startPos.x+Math.min(vectorHypot, ladoMenor*0.25)*Math.cos(angRad), startPos.y-Math.min(vectorHypot, ladoMenor*0.25)*Math.sin(angRad));
    speed = Math.round((Math.min(vectorHypot,ladoMenor*0.25)/(ladoMenor*0.25)*100));
  }
  function drawBase(){ 
    c.beginPath(); c.lineWidth = 6; c.arc(startPos.x, startPos.y, ladoMenor*0.10 ,0,Math.PI*2,true); c.stroke();
    c.beginPath(); c.lineWidth = 2; c.arc(startPos.x, startPos.y, ladoMenor*0.35 ,0,Math.PI*2,true); c.stroke();
  }
  function drawStick(){
    c.beginPath(); c.arc(posMax.x, posMax.y, ladoMenor*0.10, 0,Math.PI*2, true); c.stroke();
  }

  function drawText(){
    document.getElementById('txt2_id').innerHTML = 'Ángulo<br/>'+angGrd+'º';
    document.getElementById('txt1_id').innerHTML = 'Velocidad<br/>'+speed+'%';
  }
  function sendData(){
    var dir = 'Velocidad:'+speed+' Angulo:'+angRad;
    console.log(dir);
    connection.send(dir);
  }
  </script>
  </body>
  </html>
)";

ESP8266WebServer server (80);
WebSocketsServer webSocket = WebSocketsServer(81);

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {

switch(type) {
  case WStype_DISCONNECTED:
    Serial.printf("[%u] Disconnected!\n", num);
    analogWrite(IZQ_PWM, 0);
    analogWrite(DER_PWM, 0);
    break;

  case WStype_CONNECTED: {
    IPAddress ip = webSocket.remoteIP(num);
    Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
    webSocket.sendTXT(num, "Connected");          // send message to client
  }
  break;

  case WStype_TEXT:
    Serial.printf("Número de conexión: %u - Texto recibido: %s\n", num, payload);
    if(num == 0) {                                            //Únicamente va a reconocer la primera conexión (softAP). 
      String str = (char*)payload;                            //Ejemplo de texto recibido: Velocidad:100 Angulo:-1.88
      int velocidad = str.substring(str.indexOf(':',str.indexOf("Velocidad:"))+1,str.indexOf(' ',str.indexOf("Velocidad:"))).toInt();
      float angulo = str.substring(str.indexOf(':',str.indexOf("Angulo:"))+1).toFloat();

      //uint8_t velocidadMax = velocidad*255/100;     //La velocidad se envía con valores entre 0 y 100. Se debe mapear a 0-255
      uint8_t velocidadMax = 85+velocidad*170/100;    //Se corrige el mapeo porque el motor entre los valores 0-84 no se mueve, se mueve ente los valores 85-255

      uint8_t velocidadSin = abs(velocidadMax*sin(angulo));                     //Giro coche rápido
      //uint8_t velocidadSin = abs(velocidadMax*sin(abs(2*angulo/3)+PI/6));         //Giro coche intermedio
      //uint8_t velocidadSin = abs(velocidadMax*sin(abs(angulo/2)+PI/4));         //Giro coche lento
      float anguloCos = cos(angulo);

      //Sentido de giro de los motores 
      if(angulo > 0) {                                        //Primer y segundo cuadrante
        digitalWrite(STBY, 1);
        digitalWrite(IZQ_AVZ,  1); digitalWrite(IZQ_RET,  0); digitalWrite(DER_AVZ,  1); digitalWrite(DER_RET,  0);
      }else if(angulo < 0) {                                  //Tercer y cuarto cuadrante
        digitalWrite(STBY, 1);
        digitalWrite(IZQ_AVZ,  0); digitalWrite(IZQ_RET,  1); digitalWrite(DER_AVZ,  0); digitalWrite(DER_RET,  1);
      }else {                                                 //Parado
        digitalWrite(STBY, 0);
        digitalWrite(IZQ_AVZ,  0); digitalWrite(IZQ_RET,  0); digitalWrite(DER_AVZ,  0); digitalWrite(DER_RET,  0);
      }
      //Velocidad de los motores
      uint8_t izq, der;
      if(anguloCos > 0) {                                     //Primer y cuarto cuadrante
        izq = velocidadMax;   der = velocidadSin;
      } else {                                                //Segundo y tercer cuadrante
        izq = velocidadSin;   der = velocidadMax;
      }
      analogWrite(IZQ_PWM, izq);
      analogWrite(DER_PWM, der);                   
      //Serial.print(anguloCos); Serial.print("    "); Serial.print(izq); Serial.print("    "); Serial.println(der);
      }
    break;
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println();

  WiFi.begin(ssid, password);
while (WiFi.status() != WL_CONNECTED) {
  delay(500);
  Serial.print(".");
}
Serial.println("");
Serial.println("WiFi connected");

 
 /* IPAddress myIP = WiFi.softAPIP(); 
  Serial.print("IP del access point: ");
  Serial.println(myIP);
  Serial.println("WebServer iniciado...");
*/
   // start webSocket server
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  // handle index
  server.on("/", []() {
      server.send_P(200, "text/html", INDEX_HTML);
  });

  server.begin();

  pinMode(IZQ_PWM, OUTPUT); pinMode(DER_PWM, OUTPUT); 
  pinMode(IZQ_AVZ, OUTPUT); pinMode(DER_AVZ, OUTPUT);
  pinMode(IZQ_RET, OUTPUT); pinMode(DER_RET, OUTPUT);
  pinMode(STBY, OUTPUT);

  digitalWrite(IZQ_PWM, 0); digitalWrite(DER_PWM, 0);
  digitalWrite(IZQ_AVZ, 0); digitalWrite(DER_AVZ, 0);
  digitalWrite(IZQ_RET, 0); digitalWrite(DER_RET, 0);
  digitalWrite(STBY, 0);

  analogWriteRange(255);
}

void loop() {
    webSocket.loop();
    server.handleClient();
}
