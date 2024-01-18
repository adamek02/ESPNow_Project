Oto przepisany kod z bardziej logicznymi i uporządkowanymi nazwami funkcji oraz dodatkowymi komentarzami:

#include <ESP8266WiFi.h>
#include <espnow.h>

String deviceName;

char relayName[8] = "Relayfb";

int relayState = 0;
uint32_t wifiChannel;
unsigned long lastTransmissionTime = 0;  
unsigned long transmissionDelay = 10000;  

uint8_t broadcastMACAddress[] = {0x3C, 0x71, 0xBF, 0x4D, 0x79, 0x35};

constexpr char WIFI_SSID[] = "TP-LINK_WIFI";

uint8 getWiFiChannel(const char *ssid) {
  if (int32_t n = WiFi.scanNetworks()) {
      for (uint8_t i=0; i<n; i++) {
          if (!strcmp(ssid, WiFi.SSID(i).c_str())) {
              return WiFi.channel(i);
          }
      }
  }
  return 0;
}

typedef struct messageStructure {
  char name[8];
  int data;
} messageStructure;

messageStructure messageData;

void onDataTransmissionComplete(uint8_t *mac_addr, uint8_t sendStatus) {
  Serial.print("Last Packet Send Status: ");
  if (sendStatus == 0){
    Serial.println("Delivery success");
  }
  else{
    Serial.println("Delivery fail");
  }
}

void onDataReceived(uint8_t *mac_addr, uint8_t *incomingData, uint8_t len) {
  memcpy(&messageData, incomingData, sizeof(messageData));

   deviceName = messageData.name;
   
   if(deviceName == "Relay"){
  Serial.print("Bytes received: ");
  Serial.println(len);
  
  if(messageData.data == 1){
    relayState = 1;
  }
  else if(messageData.data == 0){
    relayState = 0;
  }
   }

Serial.print("Bytes received: ");
  Serial.println(len);
  Serial.print("name: ");
  Serial.println(messageData.name);
  Serial.print("val: ");
  Serial.println(messageData.data);

}


void setup() {

  pinMode(0,OUTPUT);
  digitalWrite(0,HIGH);

  Serial.begin(115200);
  ESP.eraseConfig();
  wifiChannel = getWiFiChannel(WIFI_SSID);

   esp_now_deinit();
   WiFi.mode(WIFI_STA);
   // set wifi channel
    wifi_promiscuous_enable(1);
    wifi_set_channel(wifiChannel);
    wifi_promiscuous_enable(0);
    WiFi.disconnect();

  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);

  esp_now_register_recv_cb(onDataReceived);
  esp_now_register_send_cb(onDataTransmissionComplete);

  esp_now_add_peer(broadcastMACAddress, ESP_NOW_ROLE_COMBO, wifiChannel, NULL, 0);
}



void loop() {

  if(relayState == 1)
{
digitalWrite(0,LOW);
}
else if(relayState == 0)
{
digitalWrite(0,HIGH);
}

if ((millis() - lastTransmissionTime) > transmissionDelay){



  Serial.println(relayState);
  Serial.println(wifiChannel);
    strcpy(messageData.name, relayName);
    messageData.data = relayState;

    esp_now_send(broadcastMACAddress, (uint8_t *) &messageData, sizeof(messageData));
    lastTransmissionTime = millis();
}}
