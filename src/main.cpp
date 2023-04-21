#include <Arduino.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include  <EEPROM.h>
#include <ArduinoJson.h>

#define RXD2 16//16,2
#define TXD2 17//17,4

struct
{
 StaticJsonDocument<256> doc;
}Json;

struct
{
  char buffer[256];
  char index;
  unsigned long ref;
}uart;


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
  unsigned int id;
  public:
  P_Memory(){}
  void memory_init(){EEPROM.begin(EEPROM_SIZE);}
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
void check_serial_request(void);
void check_firebase_request(void);
bool check_serial();

unsigned char buffer[150];
void setup() 
{
  Serial.begin(115200);
  Serial2.begin(115200,SERIAL_8N1,RXD2,TXD2);
  ESP32_Flash.memory_init();

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
  setup_network();
}



void loop()
{
  if(Serial2.available())Serial.write(Serial2.read());
  if(Serial.available())Serial2.write(Serial.read());
  
  //digitalWrite(2,!digitalRead(2));
  //delay(1000);
}

void check_serial_request(void)
{
  if( check_serial())
  {
    DeserializationError error = deserializeJson(Json.doc, uart.buffer);
    if (error) 
    {
      Serial.println("{\"topic\":\"status\",\"value\":\"error\"}");
    }
    else
    {
      String topic=Json.doc["topic"];
      String value=Json.doc["value"];
      if(topic=="url")
      {
        if(value=="?")
        {
        }
        else
        {
          if(value!=NULL)
          {
            //if(parse_time_date(value))
            //Serial.println("{\"topic\":\"status\",\"value\":\"ok\"}");
            //else Serial.println("{\"topic\":\"status\",\"value\":\"error\"}");
          }
        }
      }
      
    }
  }
}

bool check_serial()
{
  if(Serial.available())
  {
    memset(uart.buffer,'\0',256);
    uart.ref=millis();
    uart.index=0;
    while((unsigned long)(millis()-uart.ref)<15)
    {
       if(Serial.available())
       {
         uart.ref=millis();
         uart.buffer[uart.index++]=Serial.read();      
       }
    }
    return true;
  }
  return false;
}


static void setup_network(void)
{
  unsigned char wifi_ssid[50];
  unsigned char wifi_psk[50];
  unsigned char firebase_url[150];
  unsigned char firebase_key[150];

  ESP32_Flash.get_wifi_ssid(wifi_ssid);
  ESP32_Flash.get_wifi_psk(wifi_psk);
  ESP32_Flash.get_firebase_url(firebase_url);
  ESP32_Flash.get_firebase_api_key(firebase_key);

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
unsigned int get_device_id(void)
{
  unsigned int id;
  unsigned char *ptr=(unsigned char *)&id;
  ptr[0]=EEPROM.read(CONTROLLER_ID_ADDRESS);
  ptr[1]=EEPROM.read(CONTROLLER_ID_ADDRESS+1);
  return id;
}
void set_device_id(unsigned int id)
{
  unsigned char *ptr=(unsigned char *)&id;
  EEPROM.write(CONTROLLER_ID_ADDRESS,ptr[0]);
  EEPROM.write(CONTROLLER_ID_ADDRESS+1,ptr[1]);
  EEPROM.commit();
}

