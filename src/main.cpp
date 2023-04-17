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
  #define CONTROLLER_ID_ADDRESS 2
  #define WIFI_SSID_ADDRESS 5
  #define WIFI_PSK_ADDRESS 50
  #define FIREBASE_URL_ADDRESS 100
  #define FIREBASE_API_KEY_ADDRESS 250
  #define EEPROM_SIZE 512
  private:
  int memory_size;
  unsigned int id;
  public:
  P_Memory(){}
  void network_init(void);
  void set_wifi_ssid(const char * ssid, int  len);
  void get_wifi_ssid(unsigned char * ssid);
  void set_wifi_psk(const char * psk, int  len);
  void get_wifi_psk(unsigned char * psk);
  void set_firebase_url(const char * url, int  len);
  void get_firebase_url(unsigned char * url);
  void set_firebase_api_key(const char * key, int  len);
  void get_firebase_api_key(unsigned  char * key);
  unsigned int get_device_id();
  void set_device_id(unsigned int id);
  void clear_all_memory(void);
}ESP32_Flash;

static void setup_network(void);

unsigned char buffer[150];
void setup() 
{
  Serial.begin(115200);
  EEPROM.begin(EEPROM_SIZE);
  Serial2.begin(115200,SERIAL_8N1,RXD2,TXD2);

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
  delay(100);
  //ESP32_Flash.set_wifi_psk("59S6xVrtFT",10);
  //ESP32_Flash.get_wifi_ssid(buffer);
  setup_network();
}



void loop()
{
  digitalWrite(2,!digitalRead(2));
  //Serial.printf("%s\n",buffer);
  delay(1000);
}

static void setup_network(void)
{
  unsigned char wifi_ssid[50];
  unsigned char wifi_psk[50];
  unsigned char firebase_url[150];
  unsigned char firebase_key[150];
  ESP32_Flash.get_wifi_ssid(wifi_ssid);
  ESP32_Flash.get_wifi_psk(wifi_psk);
  WiFi.begin((const char *)wifi_ssid,(const char *)wifi_psk);
  
  Serial.printf("ssid:%s\n",wifi_ssid);
  Serial.printf("psk:%s\n",wifi_psk);
  Serial.print("connecting to wifi");

  unsigned long ref=millis();
  while(WiFi.status()!=WL_CONNECTED && (millis()-ref)<20000)
  {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP:");
  Serial.println(WiFi.localIP());
  Serial.println();
}

void P_Memory::clear_all_memory(void)
{
  for(int i=0; i<EEPROM_SIZE; i++)
  EEPROM.write(i,0);
  EEPROM.commit();
}

void P_Memory::set_wifi_ssid(const char * ssid, int  len)
{
  if(len<45)
  for(int i=0; i<len; i++)
  EEPROM.write(WIFI_SSID_ADDRESS+i,ssid[i]);
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

void P_Memory::set_wifi_psk(const char * psk, int  len)
{
  if(len<50)
  for(int i=0; i<len; i++)
  EEPROM.write(WIFI_PSK_ADDRESS+i,psk[i]);
  EEPROM.write(WIFI_PSK_ADDRESS+len,0);
  EEPROM.commit();
}
void P_Memory::get_wifi_psk(unsigned char * psk)
{
  for(int i=0; i<50; i++)
  {
    uint8_t val=EEPROM.read(WIFI_PSK_ADDRESS+i);
    psk[i]=val;
    if(val==0)
    break;
  }
}

void P_Memory::get_firebase_url(unsigned  char * url)
{
  for(int i=0; i<150; i++)
  {
    uint8_t val=EEPROM.read(FIREBASE_URL_ADDRESS+i);
    url[i]=val;
    if(val==0)
    break;
  }
}
void P_Memory::set_firebase_url(const char * url,int  len)
{
  if(len<150)
  for(int i=0; i<len; i++)
  EEPROM.write(FIREBASE_URL_ADDRESS+i,url[i]);
  EEPROM.write(FIREBASE_URL_ADDRESS+len,0);
  EEPROM.commit();
}
void P_Memory::get_firebase_api_key(unsigned char * key)
{
  for(int i=0; i<150; i++)
  {
    uint8_t val=EEPROM.read(FIREBASE_API_KEY_ADDRESS+i);
    key[i]=val;
    if(val==0)
    break;
  }
}
void P_Memory::set_firebase_api_key(const char * key, int  len)
{
  if(len<150)
  for(int i=0; i<len; i++)
  EEPROM.write(FIREBASE_API_KEY_ADDRESS+i,key[i]);
  EEPROM.write(FIREBASE_API_KEY_ADDRESS+len,0);
  EEPROM.commit();
}