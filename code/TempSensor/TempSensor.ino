#include <ESP8266WiFi.h>
#include <espnow.h>
#include <Adafruit_BMP280.h>
extern "C"{
#include "user_interface.h"
}

// Inicjalizacja czujnika BMP280
Adafruit_BMP280 bmpSensor;

String deviceName;

float temperatureReading;
int pressureReading;
int temperatureInteger;
char temperatureLabel[8] = "Temp";
char pressureLabel[8] = "Press";
unsigned long sleepDurationMicroseconds;
uint32_t previousTime = 0;
uint32_t delayDuration = 5000;
uint32_t wifiChannel;


uint8_t broadcastMACAddress[] = {0x3C, 0x71, 0xBF, 0x4D, 0x79, 0x35};

constexpr char WIFI_SSID[] = "TP-LINK_WIFI";

// Funkcja do pobierania kanału WiFi
uint8 getChannelForWiFi(const char *ssid) {
  if (int32_t n = WiFi.scanNetworks()) {
      for (uint8_t i=0; i<n; i++) {
          if (!strcmp(ssid, WiFi.SSID(i).c_str())) {
              return WiFi.channel(i);
          }
      }
  }
  return 0;
}

// Struktura wiadomości do wysyłania danych
typedef struct messageStructure {
  char name[8];
  int data;
} messageStructure;

messageStructure messageData;

// Callback po wysłaniu danych
void onDataTransmissionComplete(uint8_t *mac_addr, uint8_t sendStatus) {
  Serial.print("Last Packet Send Status: ");
  if (sendStatus == 0){
    Serial.println("Delivery success");
  }
  else{
    Serial.println("Delivery fail");
    }
  }

// Callback po otrzymaniu danych
void onDataReceived(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
  memcpy(&messageData, incomingData, sizeof(messageData));

   deviceName = messageData.name;
   
   if(deviceName == "T_delay"){
  Serial.println(messageData.name);
  Serial.print("Bytes received: ");
  Serial.println(len);
  delayDuration = messageData.data;
  Serial.print(" Sending every: ");
  Serial.println(delayDuration);
   }
}

// Ustawienia początkowe
void setup() {

  Serial.begin(115200);

  wifiChannel = getChannelForWiFi(WIFI_SSID);
  Serial.println(WiFi.macAddress());

   esp_now_deinit();
   WiFi.mode(WIFI_STA);
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

  // Inicjalizacja czujnika BMP280
  Wire.begin(5,4);

  bmpSensor.begin(0x76,0x58);

  bmpSensor.setSampling(Adafruit_BMP280::MODE_NORMAL,     
                  Adafruit_BMP280::SAMPLING_X2,     
                  Adafruit_BMP280::SAMPLING_X8,    
                  Adafruit_BMP280::FILTER_X8,      
                  Adafruit_BMP280::STANDBY_MS_500); 

}

// Główna pętla programu
void loop() {

  if ((millis() - previousTime) > delayDuration) {

  temperatureReading = bmpSensor.readTemperature();
  pressureReading = bmpSensor.readPressure();

  temperatureInteger = int(temperatureReading * 100);

    
    strcpy(messageData.name, temperatureLabel);
    messageData.data = temperatureInteger;

    esp_now_send(broadcastMACAddress, (uint8_t *) &messageData, sizeof(messageData));
    delay(20);
    
    strcpy(messageData.name, pressureLabel);
    messageData.data = pressureReading;


    esp_now_send(broadcastMACAddress, (uint8_t *) &messageData, sizeof(messageData));

    previousTime = millis();
  }
}