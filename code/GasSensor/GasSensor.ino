Oto przepisany kod z bardziej logicznymi i uporządkowanymi nazwami funkcji oraz dodatkowymi komentarzami:

#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_now.h>

#define MICROSECONDS_TO_SECONDS_FACTOR 1000000  
#define SLEEP_DURATION  30       

char gasSensorName[8] = "GAS";
char gasSensorSleepTimeName[8] = "GAS_t_s";

uint8_t broadcastMACAddress[] = {0x3C, 0x71, 0xBF, 0x4D, 0x79, 0x34};

unsigned long sleepTime = 0;
unsigned long lastSleepTime = 0;
int awakeDuration = 1000;
int lastMillis = 0;
int startMillis = 0;
unsigned long lastTime = 0;  
unsigned long timerDelay = 1000;  
unsigned long lastTriggerTime = 0;
unsigned long triggerDelay = 1000;
unsigned long buttonTime = 0;  
unsigned long lastButtonTime = 0; 
int triggerFlag = 0;
int pinStatus = 0;
int trigger = 0;
int gasTrigger;
RTC_DATA_ATTR int32_t wifiChannel;
RTC_DATA_ATTR int32_t wakeupCounter;
const unsigned int wakeupChannelRefreshRate = 20;

constexpr char WIFI_SSID[] = "DSLABPROJECT";

int32_t getWiFiChannel(const char *ssid) {
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

esp_now_peer_info_t peerInfo;

void onDataTransmissionComplete(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.println("Last Packet Send Status: ");
  if (status == 0){
    Serial.println(" Delivery success ");
  }
  else{
    Serial.println(" Delivery fail ");
  }
}

struct GasSensor {
	const uint8_t PIN;
  unsigned long time;
	int detected;
};

GasSensor gasSensor = {25, 0, 0};

void IRAM_ATTR interruptServiceRoutine() {
  trigger = 1;
  triggerFlag = 1;
	gasSensor.detected = 1;
}

void printWakeupReason(){
  esp_sleep_wakeup_cause_t wakeupReason;
  wakeupCounter++;
  wakeupReason = esp_sleep_get_wakeup_cause();

  switch(wakeupReason)

  {case ESP_SLEEP_WAKEUP_EXT0 : 

  trigger = 1;
  triggerFlag = 1;
	gasSensor.detected = 1;
  break;

   case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
  default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeupReason); break;
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(gasSensor.PIN, INPUT_PULLUP);
  attachInterrupt(gasSensor.PIN, interruptServiceRoutine, FALLING);

  esp_sleep_enable_timer_wakeup(SLEEP_DURATION * MICROSECONDS_TO_SECONDS_FACTOR);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_25,0);

  printWakeupReason();

  WiFi.mode(WIFI_STA);

  Serial.println(wakeupCounter); 
if(wakeupCounter == wakeupChannelRefreshRate){
wakeupCounter = 0;
}

if(wakeupCounter == 1){
wifiChannel = getWiFiChannel(WIFI_SSID);
}

  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(wifiChannel, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);


  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  
  esp_now_register_send_cb(onDataTransmissionComplete);

  memcpy(peerInfo.peer_addr, broadcastMACAddress, 6);
  peerInfo.channel = wifiChannel;  
  peerInfo.encrypt = false;

 if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
}}

 
void loop() {
  pinStatus = digitalRead(gasSensor.PIN);

  if(pinStatus == 1 && trigger == 1){
    gasSensor.detected = 0;
    trigger = 0;
  }


  if(gasSensor.detected == 0 && pinStatus == 1){
    gasTrigger = 0;
    if ((millis() - lastTime) > timerDelay){


    strcpy(messageData.name, gasSensorName);
    messageData.data = gasTrigger;
    esp_now_send(broadcastMACAddress, (uint8_t *) &messageData, sizeof(messageData));
      
      lastTime = millis();
    }

  }

  else if(gasSensor.detected == 1 && pinStatus == 0){
         gasTrigger = 1;

    if ((millis() - lastTriggerTime) > triggerDelay){
      gasSensor.time++;

      strcpy(messageData.name, gasSensorName);
      messageData.data = gasTrigger;
      esp_now_send(broadcastMACAddress, (uint8_t *) &messageData, sizeof(messageData));

      lastTriggerTime = millis();
    }
  
}

 if((gasSensor.time > 1) && pinStatus == 1){
        strcpy(messageData.name, gasSensorSleepTimeName);
        messageData.data = gasSensor.time;
        esp_now_send(broadcastMACAddress, (uint8_t *) &messageData, sizeof(messageData));
        gasSensor.time = 0;
        delay(100);
        }

sleepTime = millis();
if ((sleepTime - lastSleepTime > awakeDuration) && pinStatus == 1)
{
  lastSleepTime = sleepTime;
  esp_deep_sleep_start();
}

}