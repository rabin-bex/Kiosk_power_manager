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


bool signupOK=false;

FirebaseData fbdo,fbdo_stream;
FirebaseAuth auth;
FirebaseConfig config;
unsigned char device_id;



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
  void get_wifi_ssid(char * ssid);
  void set_wifi_psk(const char * psk, int  len);
  void get_wifi_psk(char * psk);
  void set_firebase_url(const char * url, int  len);
  void get_firebase_url(char * url);
  void set_firebase_api_key(const char * key, int  len);
  void get_firebase_api_key(char * key);
  unsigned int get_device_id();
  void set_device_id(unsigned int id);
  void clear_all_memory(void);
}ESP32_Flash;

static void setup_network(void);
static void check_serial_request(void);
static void check_firebase_request(void);
static bool check_serial();

void setup() 
{
  Serial.begin(115200);
  Serial2.begin(115200,SERIAL_8N1,RXD2,TXD2);
  ESP32_Flash.memory_init();
  setup_network();
}

void loop()
{
  check_firebase_request();
  check_serial_request();
}

static void check_firebase_request(void)
{
  if( Firebase.ready() && signupOK)
  {
    if(!Firebase.RTDB.readStream(&fbdo_stream)) Serial.printf("Stream read error, %s\n\n",fbdo_stream.errorReason().c_str());
    
    if( fbdo_stream.streamAvailable())
    {
      if(fbdo_stream.boolData())
      {
        if(Firebase.RTDB.getInt(&fbdo,"Device_ID"))
        {
          if(fbdo.dataType()=="int")
          {
            if( device_id==fbdo.intData())
            { 
              Serial.println("stream flag is true");
              Firebase.RTDB.setBool(&fbdo,"Update_Flag",false);
              Firebase.RTDB.setString(&fbdo,"Device_Response","ok");
            }
          }
        }
      }
    }
  }
}

static void check_serial_request(void)
{
  if( check_serial())
  {
    char buffer[200];
    strcpy(buffer,uart.buffer);
    DeserializationError error = deserializeJson(Json.doc, buffer);
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
          ESP32_Flash.get_firebase_url(buffer);
          Serial.print("{\"topic\":\"url\",\"value\":\"");
          Serial.printf("%s",buffer);
          Serial.println("\"}");
        }
        else
        {
          if(value!=NULL)
          {
            int len=value.length()+1;
            value.toCharArray(buffer,len);
            ESP32_Flash.set_firebase_url(buffer,len);
            Serial.println("{\"topic\":\"status\",\"value\":\"ok\"}");
          }
        }
      }


      else if(topic=="api_key")
      {
        if(value=="?")
        {
          ESP32_Flash.get_firebase_api_key(buffer);
          Serial.print("{\"topic\":\"api_key\",\"value\":\"");
          Serial.printf("%s",buffer);
          Serial.println("\"}");
        }
        else
        {
          if(value!=NULL)
          {
            int len=value.length()+1;
            value.toCharArray(buffer,len);
            ESP32_Flash.set_firebase_api_key(buffer,len);
            Serial.println("{\"topic\":\"status\",\"value\":\"ok\"}");
          }
        }
      }

      else if(topic=="wifi_ssid")
      {
        if(value=="?")
        {
          ESP32_Flash.get_wifi_ssid(buffer);
          Serial.print("{\"topic\":\"wifi_ssid\",\"value\":\"");
          Serial.printf("%s",buffer);
          Serial.println("\"}");
        }
        else
        {
          if(value!=NULL)
          {
            int len=value.length()+1;
            value.toCharArray(buffer,len);
            ESP32_Flash.set_wifi_ssid(buffer,len);
            Serial.println("{\"topic\":\"wifi_ssid\",\"value\":\"ok\"}");
          }
        }
      }

      else if(topic=="wifi_psk")
      {
        if(value=="?")
        {
          ESP32_Flash.get_wifi_psk(buffer);
          Serial.print("{\"topic\":\"wifi_psk\",\"value\":\"");
          Serial.printf("%s",buffer);
          Serial.println("\"}");
        }
        else
        {
          if(value!=NULL)
          {
            int len=value.length()+1;
            value.toCharArray(buffer,len);
            ESP32_Flash.set_wifi_psk(buffer,len);
            Serial.println("{\"topic\":\"wifi_psk\",\"value\":\"ok\"}");
          }
        }
      }

      else if(topic=="device_id")
      {
        if(value=="?")
        {
          Serial.print("{\"topic\":\"device_id\",\"value\":\"");
          //Serial.printf("%d",(unsigned int)ESP32_Flash.get_device_id());
          Serial.printf("%d",2);
          Serial.println("\"}");
        }
        else
        {
          if(value!=NULL)
          {
            int len=value.length()+1;
            value.toCharArray(buffer,len);
            ESP32_Flash.set_device_id(atoi(buffer));
            Serial.println("{\"topic\":\"device_id\",\"value\":\"ok\"}");
          }
        }
      }


      else
      {
        Serial2.printf("%s",uart.buffer);
      }
      
    }
  }
  if(Serial2.available())Serial.write(Serial2.read());
}

static void setup_network(void)
{
  char wifi_ssid[50];
  char wifi_psk[50];
  char firebase_url[150];
  char firebase_key[150];
  
  ESP32_Flash.get_wifi_ssid(wifi_ssid);
  ESP32_Flash.get_wifi_psk(wifi_psk);
  ESP32_Flash.get_firebase_url(firebase_url);
  ESP32_Flash.get_firebase_api_key(firebase_key);
  device_id=ESP32_Flash.get_device_id();
  

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
  config.api_key="";
  config.database_url="";

  config.api_key+=firebase_key;
  config.database_url+=firebase_url;
  
  if(Firebase.signUp(&config,&auth,"",""))
  {
    Serial.println("signup OK");
    signupOK=true;
  }
  else Serial.printf("%s\n",config.signer.signupError.message.c_str());
  
  config.token_status_callback=tokenStatusCallback;
  Firebase.begin(&config,&auth);
  Firebase.reconnectWiFi(true);
  if( signupOK) if(!Firebase.RTDB.beginStream(&fbdo_stream,"Update_Flag"))
    Serial.printf("Update flag stream error, %s\n\n",fbdo_stream.errorReason().c_str());
}



static bool check_serial()
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

void P_Memory::get_wifi_ssid(char * ssid)
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
void P_Memory::get_wifi_psk(char * psk)
{
  for(int i=0; i<50; i++)
  {
    uint8_t val=EEPROM.read(WIFI_PSK_ADDRESS+i);
    psk[i]=val;
    if(val==0)
    break;
  }
}

void P_Memory::get_firebase_url(char * url)
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
void P_Memory::get_firebase_api_key(char * key)
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
unsigned int P_Memory::get_device_id(void)
{
  unsigned int id;
  unsigned char *ptr=(unsigned char *)&id;
  ptr[0]=EEPROM.read(CONTROLLER_ID_ADDRESS);
  ptr[1]=EEPROM.read(CONTROLLER_ID_ADDRESS+1);
  return id;
}
void P_Memory::set_device_id(unsigned int id)
{
  unsigned char *ptr=(unsigned char *)&id;
  EEPROM.write(CONTROLLER_ID_ADDRESS,ptr[0]);
  EEPROM.write(CONTROLLER_ID_ADDRESS+1,ptr[1]);
  EEPROM.commit();
}

