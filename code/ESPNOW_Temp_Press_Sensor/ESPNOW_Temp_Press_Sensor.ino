#include <ESP8266WiFi.h>
#include <espnow.h>
#include <Adafruit_BMP280.h>

Adafruit_BMP280 bmp;

float Temperature;
int Pressure;
int Temp_int;
char Temp_name[8] = "Temp";
char Press_name[8] = "Press";

int Device_data = 0;

uint8_t broadcastAddress[] = {0x3C, 0x71, 0xBF, 0x4D, 0x79, 0x34};

typedef struct struct_message {
  char name[8];
  int data;
} struct_message;

struct_message myData;

unsigned long lastTime = 0;  
unsigned long timerDelay = 2000;  

void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
  Serial.print("Last Packet Send Status: ");
  if (sendStatus == 0){
    Serial.println("Delivery success");
  }
  else{
    Serial.println("Delivery fail");
  }
}


void setup() {
  Serial.begin(115200);
  Wire.begin(5,4);

  bmp.begin(0x76,0x58);

  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                  Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                  Adafruit_BMP280::SAMPLING_X8,    /* Pressure oversampling */
                  Adafruit_BMP280::FILTER_X8,      /* Filtering. */
                  Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  esp_now_register_send_cb(OnDataSent);

  esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);
}


 
void loop() {
  Temperature = bmp.readTemperature();
  Pressure = bmp.readPressure();

  Temp_int = int(Temperature * 100);

  if ((millis() - lastTime) > timerDelay) {
    
    //temperature
    strcpy(myData.name, Temp_name);
    myData.data = Temp_int;

    esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
    delay(20);
    //pressure
    strcpy(myData.name, Press_name);
    myData.data = Pressure;


    esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));


    lastTime = millis();
  }
}