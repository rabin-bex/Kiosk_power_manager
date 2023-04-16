#include <Arduino.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include  <EEPROM.h>



//define the number of bytes you want to access

#define RXD2 2
#define TXD2 4

#define WIFI_SSID "Paaila_Wifi"
#define WIFI_PASSWORD "59S6xVrtFT"
#define API_KEY "AIzaSyBZynTrydZRQvNCJPXcEV--2A4zaXAZO8A"
#define DATABASE_URL "https://esp32-rtdb-83408-default-rtdb.asia-southeast1.firebasedatabase.app"

unsigned long sendDataPrevMillis=0;
bool signupOK=false;
int ldrData=0;
float voltage=0.0;

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

uint8_t test_val;
//class for permant memory
class P_Memory
{
  #define WIFI_SSID_ADDRESS 5
  #define WIFI_PSK_ADDRESS 50
  #define FIREBASE_URL_ADDRESS 100
  #define FIREBASE_API_KEY_ADDRESS 250
  #define EEPROM_SIZE 512
  private:
  int memory_size;
  public:
  void network_init(void);
  void set_wifi_ssid(unsigned char * ssid, int  len);
  void get_wifi_ssid(unsigned char * ssid);
  void set_wifi_psk(void);
  void get_firebase_url(void);
  void set_firebase_url(void);
  void get_firebase_api_key(void);
  void set_firebase_api_key(void);
  void clear_all_memory(void);
}ESP32_Flash;


void setup() {
  Serial.begin(115200);
  EEPROM.begin(EEPROM_SIZE);
  // WiFi.begin(WIFI_SSID,WIFI_PASSWORD);
  // Serial.print("connecting to wifi");
  // Serial2.begin(115200,SERIAL_8N1,RXD2,TXD2);
  // while(WiFi.status()!=WL_CONNECTED)
  // {
  //   Serial.print(".");
  //   delay(300);
  // }
  // Serial.println();
  // Serial.print("Connected with IP:");
  // Serial.println(WiFi.localIP());
  // Serial.println();
  // config.api_key=API_KEY;
  // config.database_url=DATABASE_URL;
  // if(Firebase.signUp(&config,&auth,"",""))
  // {
  //   Serial.println("signup OK");
  //   signupOK=true;
  // }
  // else
  // {
  //   Serial.printf("%s\n",config.signer.signupError.message.c_str());
  // }
  // config.token_status_callback=tokenStatusCallback;
  // Firebase.begin(&config,&auth);
  // Firebase.reconnectWiFi(true);
  pinMode(2,OUTPUT); 
}



void loop()
{
  digitalWrite(2,!digitalRead(2));
  // Serial.printf("%d\n",test_val);
  delay(1000);
}

void P_Memory::clear_all_memory(void)
{
  for(int i=0; i<EEPROM_SIZE; i++)
  EEPROM.write(i,0);
  EEPROM.commit();
}

void P_Memory::set_wifi_ssid(unsigned char * val, int  len)
{
  if(len<45)
  for(int i=0; i<len; i++)
  EEPROM.write(WIFI_SSID_ADDRESS+i,val[i]);
  EEPROM.write(WIFI_SSID_ADDRESS+len,0);
  EEPROM.commit();
}

void P_Memory::get_wifi_ssid(unsigned char * ssid)
{
  for(int i=0; i<45; i++)
  {
    uint8_t val=EEPROM.read(WIFI_SSID_ADDRESS+i);
    ssid[i]=val;
    if(val==0)
    break;
  }
}