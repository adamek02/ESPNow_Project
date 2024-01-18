#include <esp_now.h>
#include <WiFi.h>
#include "ESPAsyncWebServer.h"
#include <Arduino_JSON.h>

String deviceName;
String Message;
String tempRefreshCommand = "Temp-refresh ";
String tempRefreshCommandSuffix = " ms";
const long Sensor_awake_time = 5000;
unsigned long previousClockTime = 0;
float temperatureSliderThreshold = 99.0;

const size_t MAX_VALUE_LENGTH = 2;
const char* ssid = "TP-LINK_WIFI";
const char* password = "kczerwiec1969";

unsigned int SerialValue = 0;

String sliderValue = "0";
const char* PARAM_INPUT = "value";
float temperatureReading;
float pressureReading;
float Raw_Data;
int temperatureSendFlag  = 0;
int serialTemperatureFlag  = 0;
int StateRelay = 0;
int temperatureThresholdSendFlag  = 0;

uint8_t broadcastMACAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

uint8_t relayMACAddress[] = {0x68, 0xC6, 0x3A, 0xD3, 0xE9, 0x47};

uint8_t temperatureSensorMACAddress[] = {0xBC, 0xDD, 0xC2, 0x9D, 0x69, 0x11};

typedef struct struct_message {
    char name[8];
    int data;
} struct_message;

struct_message DataSend;
struct_message DataRecieve;

JSONVar board;

AsyncWebServer server(80);
AsyncEventSource events("/events");

// Callback function to print the status of the sent packet
void OnDataSent(const uint8_t *mac, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}
// Function to print MAC address
void printMAC(const uint8_t * mac) {
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  Serial.println(macStr);
}
// Callback function to handle received data
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy( & DataRecieve, incomingData, sizeof(DataRecieve));
  // Handle data based on device name
  deviceName = DataRecieve.name;

  if (deviceName == "Temp") {         // Handle temperature data
    Serial.print("Packet received from: ");
    printMAC(mac);
    Serial.print("Bytes received: ");
    Serial.println(len);
    Serial.print("Temperature: ");
    Raw_Data = DataRecieve.data;
    temperatureReading = Raw_Data / 100;
    Serial.print(temperatureReading);
    Serial.println(" °C");
    temperatureSendFlag  = 1;
    board["name"] = DataRecieve.name;
    board["data"] = temperatureReading;
    String jsonString = JSON.stringify(board);
    events.send(jsonString.c_str(), "new_readings", millis());
  } else if (deviceName == "Press") {   // Handle pressure data
    Serial.print("Packet received from: ");
    printMAC(mac);
    Serial.print("Bytes received: ");
    Serial.println(len);
    Serial.print("Pressure: ");
    Raw_Data = DataRecieve.data;
    pressureReading = Raw_Data / 100;
    Serial.print(pressureReading);
    Serial.println(" hPa");
    board["name"] = DataRecieve.name;
    board["data"] = pressureReading;
    String jsonString = JSON.stringify(board);
    events.send(jsonString.c_str(), "new_readings", millis());
  } else if (deviceName == "GAS") {    // Handle gas data
    Serial.print("Packet received from: ");
    printMAC(mac);
    Serial.print("Bytes received: ");
    Serial.println(len);
    Serial.print("GAS: ");
    Serial.println(DataRecieve.data == 1 ? "Detected" : "Not Detected");
    board["name"] = DataRecieve.name;
    board["data"] = DataRecieve.data;
    String jsonString = JSON.stringify(board);
    events.send(jsonString.c_str(), "new_readings", millis());
  } else if (deviceName == "PIR") {    // Handle PIR data
    Serial.print("Packet received from: ");
    printMAC(mac);
    Serial.print("Bytes received: ");
    Serial.println(len);
    Serial.print("PIR: ");
    Serial.println(DataRecieve.data == 1 ? "Motion Detected" : "No Motion");
    board["name"] = DataRecieve.name;
    board["data"] = DataRecieve.data;
    String jsonString = JSON.stringify(board);
    events.send(jsonString.c_str(), "new_readings", millis());
  } else if (deviceName == "Relayfb") {    // Handle relay feedback data
    Serial.print("Packet received from: ");
    printMAC(mac);
    Serial.print("Bytes received: ");
    Serial.println(len);
    Serial.print("Relay status: ");
    Serial.println(DataRecieve.data == 1 ? "ON" : "OFF");
	board["name"] = DataRecieve.name;
    board["data"] = DataRecieve.data;
    String jsonString = JSON.stringify(board);
    events.send(jsonString.c_str(), "new_readings", millis());
    if(DataRecieve.data == 1)
    {
      StateRelay = 1;
    }
    else if(DataRecieve.data == 0)
    {
      StateRelay = 0;
    }
  } else if (deviceName == "BUTTON") {    // Handle button data
    Serial.print("Packet received from: ");
    printMAC(mac);
    Serial.print("Bytes received: ");
    Serial.println(len);
    Serial.print("Button: ");
    Serial.println(DataRecieve.data);
    if(DataRecieve.data == 1){
      board["name"] = DataRecieve.name;
      board["data"] = DataRecieve.data;
      String jsonString = JSON.stringify(board);
      events.send(jsonString.c_str(), "new_readings", millis());
    }
    else{
      board["name"] = DataRecieve.name;
      board["data"] = 0;
      String jsonString = JSON.stringify(board);
      events.send(jsonString.c_str(), "new_readings", millis());
    }
  } else {   // Handle other data
    Serial.print("Packet received from: ");
    printMAC(mac);
    Serial.print("Bytes received: ");
    Serial.println(len);
    Serial.print("Name: ");
    Serial.println(DataRecieve.name);
    Serial.print("Raw Data: ");
    Serial.println(DataRecieve.data);
    board["name"] = DataRecieve.name;
    board["data"] = DataRecieve.data;
    String jsonString = JSON.stringify(board);
    events.send(jsonString.c_str(), "new_readings", millis());
  }
}
// HTML content for the web server
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
  <title>Smarthome</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  <link rel="icon" href="data:,">
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    p {  font-size: 1.2rem;}
    body {  margin: 0;}
    .topnav { overflow: hidden; background-color: #2f4468; color: white; font-size: 1.7rem; }
    .content { padding: 20px; }
    .card { background-color: white; box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5); }
    .cards { max-width: 700px; margin: 0 auto; display: grid; grid-gap: 2rem; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); }
    .reading { font-size: 2.8rem; }
    .packet { color: #bebebe; }
    .card.temperature { color: #fd7e14; }
    .card.pressure { color: #1b78e2; }
    .card.GAS { color: #28a745; }
    .card.PIR { color: #6610f2; }
    .card.BUTTON { color: #6610f2; }
    .slider { -webkit-appearance: none; margin: 14px; width: 360px; height: 25px; background: #FFD65C;
      outline: none; -webkit-transition: .2s; transition: opacity .2s;}
    .slider::-webkit-slider-thumb {-webkit-appearance: none; appearance: none; width: 35px; height: 35px; background: #003249; cursor: pointer;}
    .slider::-moz-range-thumb { width: 35px; height: 35px; background: #003249; cursor: pointer; }
    .relay-btn {
      display: inline-block;
      padding: 10px;
      font-size: 1rem;
      cursor: pointer;
      background-color: #4CAF50;
      color: black;
      border: none;
      border-radius: 5px;
      margin-top: 10px;
    }
    .relay-btn:active {
    transform: scale(0.85);
  </style>
</head>
<body>
  <div class="topnav">
    <h3>Smarthome</h3>
  </div>
  <div class="content">
    <div class="cards">
      <div class="card temperature">
        <h4><i class="fas fa-thermometer-half"></i> TEMPERATURE</h4>
        <p><span class="reading"><span id="t"></span> &deg;C</span></p>
        <span class="time"><span id="lastEventTimeTemp"></span></span>
      </div>
      <div class="card pressure">
        <h4><i class="fas fa-tint"></i> PRESSURE</h4>
        <p><span class="reading"><span id="h"></span> hPa</span></p>
        <span class="time"><span id="lastEventTimePress"></span></span>
      </div>
      <div class="card GAS">
        <h4><i class="fas fa-solid fa-skull"></i> GAS</h4><p><span class="reading"><span id="g"></span> </span></p>
        <span class="time"><span id="lastEventTimeGas"></span></span>
      </div>
      <div class="card PIR">
        <h4><i class="fa-solid fa-person-through-window"></i></i> PIR</h4><p><span class="reading"><span id="p"></span> </span></p>
        <span class="time"><span id="lastEventTimePIR"></span></span>
      </div>
      <div class="card BUTTON">
        <h4><i class="fa-duotone fa-t-rex"></i></i></i> BUTTON</h4><p><span class="reading"><span id="bt"></span> </span></p>
        <span class="time"><span id="lastEventTimeBUT"></span></span>
      </div>
      <div class="card relay" id="relayCard">
      <h4><i class="fas fa-power-off"></i> RELAY </h4>
      <p>Relay Status: <span id="relayStatus"></span></p>
      <button class="relay-btn" onclick="toggleRelay()" style="background-color: transparent;">Toggle Relay</button>
      </div>
      <div>
       <p><span id="textSliderValue"></span></p>
       <p><span id="tempRefreshValue">Temperature Refresh: 1s</span></p>
       <input type="range" onchange="updateSliderPWM(this)" id="pwmSlider" min="0" max="255" value="%SLIDERVALUE%" step="1" class="slider">
      </div>
      <div>
       <p><span id="textTEMPSliderValue"></span></p>
       <p><span id="tempTEMPRefreshValue">Temperature MAX: 22 &deg;C</span></p>
       <input type="range" onchange="updateTEMPSliderPWM(this)" id="pwmTEMPSlider" min="0" max="255" value="%SLIDERVALUE%" step="1" class="slider">
      </div>
    </div>
  </div>
<script>
if (!!window.EventSource) {
  var source = new EventSource('/events');
  
  source.addEventListener('open', function(e) {
    console.log("Events Connected");
  }, false);
  
  source.addEventListener('error', function(e) {
    if (e.target.readyState != EventSource.OPEN) {
      console.log("Events Disconnected");
    }
  }, false);
  
  source.addEventListener('message', function(e) {
    console.log("message", e.data);
  }, false);

  source.addEventListener('new_readings', function(e) {
    console.log("new_readings", e.data);
    var obj = JSON.parse(e.data);
    var currentTime = new Date();
    var formattedTime = currentTime.getHours() + ":" + currentTime.getMinutes() + ":" + currentTime.getSeconds();
    if (obj.name === "Temp") {
      document.getElementById("t").innerHTML = obj.data.toFixed(2);
      document.getElementById("lastEventTimeTemp").innerHTML = formattedTime;
    } else if (obj.name === "Press") {
      document.getElementById("h").innerHTML = obj.data.toFixed(2);
      document.getElementById("lastEventTimePress").innerHTML = formattedTime;
    } else if (obj.name === "GAS") {
      document.getElementById("g").innerHTML = obj.data;
      document.getElementById("lastEventTimeGas").innerHTML = formattedTime;
    } else if (obj.name === "PIR") {
      document.getElementById("p").innerHTML = obj.data;
      document.getElementById("lastEventTimePIR").innerHTML = formattedTime;
    } else if (obj.name === "BUTTON") {
      document.getElementById("bt").innerHTML = obj.data;
      document.getElementById("lastEventTimeBUT").innerHTML = formattedTime;
    } else if (obj.name === "Relayfb") {
      var relayStatusElement = document.getElementById("relayStatus");
      relayStatusElement.innerHTML = obj.data === 1 ? "ON" : "OFF";
      var relayCardElement = document.getElementById("relayCard");
      relayCardElement.style.backgroundColor = obj.data === 1 ? "#4CAF50" : "#FF0000";
  }
  }, false);

  function updateSliderPWM(element) {
    var sliderValue = document.getElementById("pwmSlider").value;
    var actualDelay = Math.round(254 * (sliderValue / 255)) + 1;
    var tempRefreshElement = document.getElementById("tempRefreshValue");
    tempRefreshElement.innerHTML = "Temperature Refresh: " + actualDelay + " s";
    console.log(actualDelay);
    var xhr = new XMLHttpRequest();
    xhr.open("GET", "/slider?value=" + Math.round(actualDelay), true);
    xhr.send();
  }

  function updateTEMPSliderPWM(element) {
    var sliderValue = document.getElementById("pwmTEMPSlider").value;
    var actualDelay = Math.round(99 * (sliderValue / 255)) + 1;
    var tempTEMPRefreshElement = document.getElementById("tempTEMPRefreshValue");
    tempTEMPRefreshElement.innerHTML = "Temperature MAX: " + actualDelay + " &deg;C";
    console.log(actualDelay);
    var xhr = new XMLHttpRequest();
    xhr.open("GET", "/slidertemp?value=" + Math.round(actualDelay), true);
    xhr.send();
  }

function toggleRelay() {
    var xhr = new XMLHttpRequest();
    xhr.open("POST", "/toggleRelay", true);
    xhr.send();
  }

}
</script>
</body>
</html>
)rawliteral";



esp_now_peer_info_t peerInfo = {};
esp_now_peer_info_t peerInfo2 = {};
esp_now_peer_info_t peerInfo3 = {};

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Setting as a Wi-Fi Station..");
  }
  Serial.print("Station IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Wi-Fi Channel: ");
  int channel = WiFi.channel();
  Serial.println(WiFi.channel());

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_register_send_cb(OnDataSent);

  memcpy(peerInfo.peer_addr, broadcastMACAddress, 6);
  peerInfo.channel = channel;
  peerInfo.encrypt = false;

  if (esp_now_add_peer( & peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

   memcpy(peerInfo2.peer_addr, relayMACAddress, 6);
  peerInfo2.channel = channel;
  peerInfo2.encrypt = false;

  if (esp_now_add_peer( & peerInfo2) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

   memcpy(peerInfo3.peer_addr, temperatureSensorMACAddress, 6);
  peerInfo3.channel = channel;
  peerInfo3.encrypt = false;

  if (esp_now_add_peer( & peerInfo3) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

  esp_now_register_recv_cb(OnDataRecv);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request -> send_P(200, "text/html", index_html);
  });

  events.onConnect([](AsyncEventSourceClient * client) {
    if (client -> lastId()) {
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client -> lastId());
    }
    client -> send("hello!", NULL, millis(), 10000);
  });

  server.on("/slider", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    if (request->hasParam(PARAM_INPUT)) {
      inputMessage = request->getParam(PARAM_INPUT)->value();
      sliderValue = inputMessage;
      Serial.println(sliderValue);
      char name[8] = "T_delay";
      strcpy(DataSend.name, name);
      int msg = sliderValue.toInt() * 1000;
      DataSend.data =msg;
      esp_now_send(temperatureSensorMACAddress, (uint8_t *)&DataSend, sizeof(DataSend));

    }
    else {
      inputMessage = "No message sent";
    }
    Serial.println(inputMessage);
    request->send(200, "text/plain", "OK");
  });


server.on("/slidertemp", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    if (request->hasParam(PARAM_INPUT)) {
      inputMessage = request->getParam(PARAM_INPUT)->value();
      sliderValue = inputMessage;
      temperatureSliderThreshold = sliderValue.toFloat();
      Serial.println(sliderValue);
      temperatureThresholdSendFlag  = 1;
    }
    else {
      inputMessage = "No message sent";
    }
    Serial.println(inputMessage);
    request->send(200, "text/plain", "OK");
  });

server.on("/toggleRelay", HTTP_POST, [](AsyncWebServerRequest *request) {
  int relayState = (DataRecieve.data == 1) ? 0 : 1; 
  if (relayState == 1){
    char name[8] = "Relay";
    strcpy(DataSend.name, name);
    DataSend.data = relayState;
    esp_now_send(relayMACAddress, (uint8_t *)&DataSend, sizeof(DataSend));
  }
  else if (relayState == 0){
    char name[8] = "Relay";
    strcpy(DataSend.name, name);
    DataSend.data = 3;
    esp_now_send(relayMACAddress, (uint8_t *)&DataSend, sizeof(DataSend));
  }
  request->send(200, "text/plain", "OK");
});


  server.addHandler( & events);
  server.begin();
}

void loop() {
  static unsigned long lastEventTime = millis();
  static const unsigned long EVENT_INTERVAL_MS = 3000;
  if ((millis() - lastEventTime) > EVENT_INTERVAL_MS) {
    events.send("ping", NULL, millis());
	
	if ((temperatureReading > temperatureSliderThreshold) &&  (temperatureThresholdSendFlag  == 1 || StateRelay == 0)) {
            char name[8] = "Relay";
            strcpy(DataSend.name, name);
            DataSend.data = 1; 
            esp_now_send(relayMACAddress, (uint8_t *)&DataSend, sizeof(DataSend));
            temperatureThresholdSendFlag  = 0;
        } 
    else if((temperatureReading < temperatureSliderThreshold) &&  (temperatureThresholdSendFlag  == 1 || StateRelay == 1)) {
            char name[8] = "Relay";
            strcpy(DataSend.name, name);
            DataSend.data = 3; 
            esp_now_send(relayMACAddress, (uint8_t *)&DataSend, sizeof(DataSend));
            temperatureThresholdSendFlag  = 0;
      }
	
    lastEventTime = millis();
  }

  if (Serial.available()) {
    Message = Serial.readString();

    if (Message.startsWith(tempRefreshCommand) && Message.endsWith(tempRefreshCommandSuffix)) {
      Message.remove(0, 13);
      Message.remove((Message.length() - 3), 3);
      SerialValue = Message.toInt();
      serialTemperatureFlag  = 1;
    }
  }

  if (millis() - previousClockTime > Sensor_awake_time) {
    temperatureSendFlag  = 0;
    previousClockTime = millis();
  }

  if (temperatureSendFlag  == 1 && serialTemperatureFlag  == 1) {
    char name[8] = "T_delay";
    strcpy(DataSend.name, name);
    DataSend.data = SerialValue;

    esp_now_send(temperatureSensorMACAddress, (uint8_t * ) & DataSend, sizeof(DataSend));
    temperatureSendFlag  = 0;
    serialTemperatureFlag  = 0;
  }
}