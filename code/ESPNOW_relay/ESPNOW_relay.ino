#include <WiFi.h>
#include <esp_now.h>


char Dev_name[8] = "BUTTON";
//variables to keep track of the timing of recent interrupts
unsigned long button_time = 0;  
unsigned long last_button_time = 0; 
unsigned long sleep_time = 0;
unsigned long last_sleep_time = 0;
int awake_time = 5000;
unsigned long wait_time = 0;
unsigned long last_wait_time = 0;
int push_time = 300;

uint8_t broadcastAddress[] = {0x3C, 0x71, 0xBF, 0x4D, 0x79, 0x34};

typedef struct struct_message {
  char name[8];
  int data;
} struct_message;

struct_message myData;

esp_now_peer_info_t peerInfo;

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
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
  int count;
	int pressed;
};

Button button = {25, 0, 0};

void IRAM_ATTR isr() {
button_time = millis();
if (button_time - last_button_time > 10)
{
	button.pressed = 1;
  button.count++;
  last_button_time = button_time;
}
}


void setup() {
  Serial.begin(115200);

  pinMode(button.PIN, INPUT_PULLUP);
  attachInterrupt(button.PIN, isr, RISING);

  esp_sleep_enable_ext0_wakeup(GPIO_NUM_25,0);

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_register_send_cb(OnDataSent);

  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;

 if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
}
}



 
void loop() {

if(button.pressed == 1){
  
    delay(push_time);

    button.pressed = 0;

    strcpy(myData.name, Dev_name);
    myData.data = button.count;
    esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));

    button.count = 0;
  }

sleep_time = millis();
if (sleep_time - last_sleep_time > awake_time)
{
  last_sleep_time = sleep_time;
  esp_deep_sleep_start();
}
}
  