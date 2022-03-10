
#include <WiFi.h>                                     // needed to connect to WiFi
#include <WebServer.h>                                // needed to create a simple webserver
#include <WebSocketsServer.h>                         // needed for instant communication between client and server through Websockets
#include <ArduinoJson.h>                              // needed for JSON encapsulation (send multiple variables with one string)

#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>

#define PIN 8  //changed from 6 to 8 by DK

// SSID and password of Wifi connection:
const char* ssid = "earrings (password: earrings, goto: 192.168.1.22)";
const char* password = "earrings";

// Configure IP addresses of the local access point
IPAddress local_IP(192,168,1,22);
IPAddress gateway(192,168,1,5);
IPAddress subnet(255,255,255,0);

Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(5, 5, PIN,
NEO_MATRIX_TOP  + NEO_MATRIX_LEFT + //Change to rotate text 90 degrees
NEO_MATRIX_ROWS + NEO_MATRIX_PROGRESSIVE, //Change to rotate text 90 degrees
NEO_GRB            + NEO_KHZ800);

const uint16_t colors[] = {
  matrix.Color(255, 0, 0), matrix.Color(0, 255, 0), matrix.Color(0, 0, 255) };

int x = matrix.width();
int pass = 0;

String message = "hello";
int messageLength = 5;

// The String below "webpage" contains the complete HTML code that is sent to the client whenever someone connects to the webserver
String webpage = "<!DOCTYPE html><html> <head> <title>LED Earrings</title> <style>body{height: 100vh; display: flex; flex-direction: column; justify-content: center; align-items: center; font-family: sans-serif; padding: 1rem; font-size: 2rem;}button{background: yellow; border: none; border-radius: 5px; padding: 1rem; border: 1px solid black;}textarea{margin-bottom: 1rem;}h1{margin: 0 0 0.5rem 0;}</style> </head> <body> <h1>LED Earrings</h1> <p>Enter your message and hit send :)</p><textarea></textarea> <button>Send</button> </body> <script>let Socket; const button=document.querySelector('button'); const input=document.querySelector('textarea'); const init=()=>{Socket=new WebSocket('ws://' + window.location.hostname + ':81/');}; button.addEventListener('click', (e)=>{e.preventDefault(); const message=JSON.stringify({message: input.value, messageLength: input.value.length,}); Socket.send(message); console.log(JSON.parse(message)); input.value='';}); window.onload=function (event){init();}; </script></html>";
// The JSON library uses static memory, so this will need to be allocated:
StaticJsonDocument<200> doc_tx;                       // provision memory for about 200 characters
StaticJsonDocument<200> doc_rx;

// We want to periodically send values to the clients, so we need to define an "interval" and remember the last time we sent data to the client (with "previousMillis")
int interval = 200;                                  // send data to the client every 1000ms -> 1s
unsigned long previousMillis = 0;                     // we use the "millis()" command for time reference and this will output an unsigned long

// Initialization of webserver and websocket
WebServer server(80);                                 // the server uses port 80 (standard port for websites
WebSocketsServer webSocket = WebSocketsServer(81);    // the websocket uses port 81 (standard port for websockets

void setup() {
  Serial.begin(115200);                               // init serial port for debugging
 
  Serial.print("Setting up Access Point ... ");
  Serial.println(WiFi.softAPConfig(local_IP, gateway, subnet) ? "Ready" : "Failed!");

  Serial.print("Starting Access Point ... ");
  Serial.println(WiFi.softAP(ssid, password) ? "Ready" : "Failed!");

  Serial.print("IP address = ");
  Serial.println(WiFi.softAPIP());
  
  server.on("/", []() {                               // define here wat the webserver needs to do
    server.send(200, "text\html", webpage);           //    -> it needs to send out the HTML string "webpage" to the client
  });
  server.begin();                                     // start server
  
  webSocket.begin();                                  // start websocket
  webSocket.onEvent(webSocketEvent);                  // define a callback function -> what does the ESP32 need to do when an event from the websocket is received? -> run function "webSocketEvent()"

  matrix.begin();
  matrix.setTextWrap(false);
  matrix.setBrightness(40);
  matrix.setTextColor(colors[0]);
}

void loop() {
  server.handleClient();                              // Needed for the webserver to handle all clients
  webSocket.loop();                                   // Update function for the webSockets

  unsigned long now = millis();                       // read out the current "time" ("millis()" gives the time in ms since the Arduino started)
  if ((unsigned long)(now - previousMillis) > interval) { // check if "interval" ms has passed since last time the clients were updated
    matrix.fillScreen(0);
    matrix.setCursor(x, 0);
    matrix.print(message); //the "8" in the line below is the number of characters in the text to be printed.
    if(--x < -(5+messageLength*5)) {
      x = matrix.width();
      if(++pass >= 3) pass = 0;
      matrix.setTextColor(colors[pass]);
    }
    matrix.show();
    
    previousMillis = now;                             // reset previousMillis
  }
}

void webSocketEvent(byte num, WStype_t type, uint8_t * payload, size_t length) {      // the parameters of this callback function are always the same -> num: id of the client who send the event, type: type of message, payload: actual data sent and length: length of payload
  switch (type) {                                     // switch on the type of information sent
    case WStype_DISCONNECTED:                         // if a client is disconnected, then type == WStype_DISCONNECTED
      Serial.println("Client " + String(num) + " disconnected");
      break;
    case WStype_CONNECTED:                            // if a client is connected, then type == WStype_CONNECTED
      Serial.println("Client " + String(num) + " connected");
      // optionally you can add code here what to do when connected
      break;
    case WStype_TEXT:                                 // if a client has sent data, then type == WStype_TEXT
      // try to decipher the JSON string received
      DeserializationError error = deserializeJson(doc_rx, payload);
      if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
      }
      else {
        // JSON string was received correctly, so information can be retrieved:
        const char* _message = doc_rx["message"];
        const int _messageLength = doc_rx["messageLength"];
   
        message = String(_message);
        messageLength = _messageLength;
      }
      Serial.println("");
      break;
  }
}
