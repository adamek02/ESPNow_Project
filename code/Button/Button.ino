#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

char deviceName[8] = "BUTTON";

// Variables to keep track of the timing of recent interrupts
unsigned long buttonPressTime = 0;  
unsigned long lastButtonPressTime = 0; 
unsigned long sleepStartTime = 0;
unsigned long lastSleepStartTime = 0;
int awakeDuration = 500;
unsigned long waitTime = 0;
unsigned long lastWaitTime = 0;
int buttonPressDuration = 300;
RTC_DATA_ATTR int32_t wifiChannel;
RTC_DATA_ATTR int32_t wakeupCounter;
const unsigned int wakeupChannelRefreshRate = 20;

uint8_t broadcastMACAddress[] = {0x3C, 0x71, 0xBF, 0x4D, 0x79, 0x34};

typedef struct messageStructure {
  char name[8];
  int data;
} messageStructure;

messageStructure messageData;

esp_now_peer_info_t peerInfo;

// Insert your SSID
constexpr char WIFI_SSID[] = "TP-LINK_WIFI";

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

void onDataTransmissionComplete(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.println("Last Packet Send Status: ");
  if (status == 0){
    Serial.println(" Delivery success ");
  }
  else{
    Serial.println(" Delivery fail ");
  }
}

struct Button {
	const uint8_t PIN;
  int pressCount;
	int isPressed;
};

Button button = {32, 0, 0};

void IRAM_ATTR buttonPressInterruptServiceRoutine() {
buttonPressTime = millis();
if (buttonPressTime - lastButtonPressTime > 20)
{
	button.isPressed = 1;
  button.pressCount++;
  lastButtonPressTime = buttonPressTime;
}
}


void setup() {
  Serial.begin(115200);
  wakeupCounter++;
  pinMode(button.PIN, INPUT_PULLUP);
  attachInterrupt(button.PIN, buttonPressInterruptServiceRoutine, RISING);

  esp_sleep_enable_ext0_wakeup(GPIO_NUM_32,0);

  WiFi.mode(WIFI_STA);

    Serial.println(wakeupCounter); // Diagnostics for number of wakeups
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
}
}

void loop() {

if(button.isPressed == 1){
  
    delay(buttonPressDuration);

    button.isPressed = 0;

    strcpy(messageData.name, deviceName);
    messageData.data = button.pressCount;
    esp_now_send(broadcastMACAddress, (uint8_t *) &messageData, sizeof(messageData));

    button.pressCount = 0;
  }

sleepStartTime = millis();
if (sleepStartTime - lastSleepStartTime > awakeDuration)
{
  lastSleepStartTime = sleepStartTime;
  esp_deep_sleep_start();
}
}