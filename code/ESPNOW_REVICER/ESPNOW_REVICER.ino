#include <esp_now.h>
#include <WiFi.h>

String Device;

float Temperature;
float Pressure;
float Raw_Data;

typedef struct struct_message {
    char name[8];
    int data;
} struct_message;

struct_message myData;

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&myData, incomingData, sizeof(myData));
 
 Device = myData.name;

 if(Device == "Temp"){
  Serial.print("Bytes received: ");
  Serial.println(len);
  Serial.print("Temperature: ");
  Raw_Data = myData.data;
  Temperature = Raw_Data/100;
  Serial.print(Temperature);
  Serial.println(" °C");
  }
  else if(Device == "Press"){
  Serial.print("Bytes received: ");
  Serial.println(len);
  Serial.print("Pressure: ");
  Raw_Data = myData.data;
  Pressure = Raw_Data/100;
  Serial.print(Pressure);
  Serial.println(" hPa");
  }
  else{
  Serial.print("Bytes received: ");
  Serial.println(len);
  Serial.print("Name: ");
  Serial.println(myData.name);
  Serial.print("Raw Data: ");
  Serial.println(myData.data);
  }
}
 
void setup() {

  Serial.begin(115200);
  
  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  esp_now_register_recv_cb(OnDataRecv);
}
 
void loop() {

}