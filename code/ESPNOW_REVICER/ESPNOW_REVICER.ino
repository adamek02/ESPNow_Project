#include <esp_now.h>
#include <WiFi.h>

String Device;
String Message;
String Temp_setting = "Temp-refresh ";
String Temp_setting_ms = " ms";
const long Sensor_awake_time = 5000;
unsigned long clock_old = 0;

unsigned int number = 0;

float Temperature;
float Pressure;
float Raw_Data;
int Send_temp = 0;
int serial_temp = 0;

uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

typedef struct struct_message {
    char name[8];
    int data;
} struct_message;

struct_message DataSend;
struct_message DataRecieve;

void OnDataSent(const uint8_t *mac, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
  Serial.print("Wysłano wartość opóźnienia w sekundach: ");
  Serial.println(float(number/1000));

}

void printMAC(const uint8_t * mac){
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  Serial.println(macStr);
}

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&DataRecieve, incomingData, sizeof(DataRecieve));
 
 Device = DataRecieve.name;

 if(Device == "Temp"){
  Serial.print("Packet received from: ");
  printMAC(mac);
  Serial.print("Bytes received: ");
  Serial.println(len);
  Serial.print("Temperature: ");
  Raw_Data = DataRecieve.data;
  Temperature = Raw_Data/100;
  Serial.print(Temperature);
  Serial.println(" °C");
  Send_temp = 1;
  }
  else if(Device == "Press"){
  Serial.print("Packet received from: ");
  printMAC(mac);
  Serial.print("Bytes received: ");
  Serial.println(len);
  Serial.print("Pressure: ");
  Raw_Data = DataRecieve.data;
  Pressure = Raw_Data/100;
  Serial.print(Pressure);
  Serial.println(" hPa");
  }
  else{
  Serial.print("Packet received from: ");
  printMAC(mac);
  Serial.print("Bytes received: ");
  Serial.println(len);
  Serial.print("Name: ");
  Serial.println(DataRecieve.name);
  Serial.print("Raw Data: ");
  Serial.println(DataRecieve.data);
  }
}
 
esp_now_peer_info_t peerInfo = {};

void setup() {

  Serial.begin(115200);
  
  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
// Register the receiver board as peer

  esp_now_register_send_cb(OnDataSent);

  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  // Set encryption to false
  peerInfo.encrypt = false;
  
  // Add receiver as peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }

  esp_now_register_recv_cb(OnDataRecv);
}
 
void loop() {

if(Serial.available()){

Message = Serial.readString();

if(Message.startsWith(Temp_setting) && Message.endsWith(Temp_setting_ms)){
Message.remove(0,13);
Message.remove((Message.length()-3),3);
number = Message.toInt();
serial_temp = 1;
}
}

if(millis() - clock_old > Sensor_awake_time){
  Send_temp = 0;
  clock_old = millis();
}

if(Send_temp == 1 && serial_temp == 1){
char name[8] = "T_delay";
strcpy(DataSend.name, name);
DataSend.data = number;

esp_now_send(broadcastAddress, (uint8_t *) &DataSend, sizeof(DataSend));
Send_temp = 0;
serial_temp = 0;
}

}