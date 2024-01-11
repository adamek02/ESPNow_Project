#include <WiFi.h>
#include <esp_now.h>


char Dev_name[8] = "GAS";
char Dev_name_2[8] = "GAS_t_s";

uint8_t broadcastAddress[] = {0x3C, 0x71, 0xBF, 0x4D, 0x79, 0x34};

int last_millis = 0;
int start_millis = 0;
unsigned long lastTime = 0;  
unsigned long timerDelay = 10000;  
unsigned long lastTime_trig = 0;
unsigned long timerDelay_trig = 1000;
unsigned long button_time = 0;  
unsigned long last_button_time = 0; 
int trigger = 0;
int PIN_STATUS = 0;
int TRIG = 0;
int gas_trigger;

typedef struct struct_message {
  char name[8];
  int data;
} struct_message;

struct_message myData;

esp_now_peer_info_t peerInfo;

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  // Serial.println("Last Packet Send Status: ");
  // if (status == 0){
  //   Serial.println(" Delivery success ");
  // }
  // else{
  //   Serial.println(" Delivery fail ");
  // }
}

struct Gas_Sensor {
	const uint8_t PIN;
  unsigned long time;
	int detected;
};

Gas_Sensor gas_sensor = {25, 0, 0};

void IRAM_ATTR isr() {
  TRIG = 1;
  trigger = 1;
	gas_sensor.detected = 1;
}


void setup() {
  Serial.begin(115200);

  pinMode(gas_sensor.PIN, INPUT_PULLUP);
  attachInterrupt(gas_sensor.PIN, isr, FALLING);

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  //esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  esp_now_register_send_cb(OnDataSent);

  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;

 if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
}}


 
void loop() {
  PIN_STATUS = digitalRead(gas_sensor.PIN);

  if(PIN_STATUS == 1 && TRIG == 1){
    gas_sensor.detected = 0;
    TRIG = 0;
  }
 


  if(gas_sensor.detected == 0 && PIN_STATUS == 1){
    gas_trigger = 0;
    if ((millis() - lastTime) > timerDelay){

    strcpy(myData.name, Dev_name);
    myData.data = gas_trigger;
    esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));

    if(gas_sensor.time > 1){
        strcpy(myData.name, Dev_name_2);
        myData.data = gas_sensor.time;
        esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
        gas_sensor.time = 0;
    }

//
    Serial.print(" Petla 1  ");
    Serial.print(PIN_STATUS);
    Serial.print("   ");
    Serial.print(gas_sensor.detected);
    Serial.print("   ");
    Serial.print(" Times triggered 0:  ");
    Serial.print(trigger);
    Serial.println("   ");

//
      
      lastTime = millis();
    }

  }

  else if(gas_sensor.detected == 1 && PIN_STATUS == 0){
         gas_trigger = 1;

    if ((millis() - lastTime_trig) > timerDelay_trig){
      gas_sensor.time++;

      strcpy(myData.name, Dev_name);
      myData.data = gas_trigger;
      esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));

//
    Serial.print(" Petla 2  ");
    Serial.print(PIN_STATUS);
    Serial.print("   ");
    Serial.print(gas_sensor.detected);
    Serial.print("   ");
    Serial.print(" Times triggered 1:  ");
    Serial.print(trigger);
    Serial.println("   ");
//
      lastTime_trig = millis();
    }
    

}}