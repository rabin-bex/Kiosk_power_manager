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
}uart,uart2;


bool signupOK=false;

FirebaseData fbdo,fbdo_stream;
FirebaseAuth auth;
FirebaseConfig config;
int device_id;

const byte Number_Of_Days_In_Months[]={31,29,31,30,31,30,31,31,30,31,30,31};


//class for permant memory
class P_Memory
{
  #define CONTROLLER_ID_ADDRESS 0
  #define WIFI_SSID_ADDRESS 5
  #define WIFI_PSK_ADDRESS 50
  #define FIREBASE_URL_ADDRESS 100
  #define FIREBASE_API_KEY_ADDRESS 250
  #define EEPROM_SIZE 2048
  #define SCHEDULE_START_ADDRESS 1024
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
  int get_device_id();
  void set_device_id(int id);
  void clear_all_memory(void);
  void storeSchedule(uint8_t month, uint8_t day,uint8_t Start_hour, uint8_t Start_minute, uint8_t End_hour, uint8_t End_minute);
  void getSchedule(uint8_t month, uint8_t day);
}ESP32_Flash;

static void setup_network(void);
static void check_serial_request(void);
static void check_firebase_request(void);
static void check_slave_request(void);
static bool check_serial();
static bool check_serial2();

//result.stringValue
                //String topic=(Firebase.RTDB.getString(&fbdo,"Server_Request/topic")?fbdo.stringData():"";  
                //String value=Firebase.RTDB.getString(&fbdo,"Server_Request/value")?fbdo.stringData():"";  
                //Firebase.RTDB.setString(&fbdo,"Device_Response","ok");

void setup() 
{
  Serial.begin(115200);
  Serial2.begin(115200,SERIAL_8N1,RXD2,TXD2);
  ESP32_Flash.memory_init();
  setup_network();
  // delay(2000);
  // char buf[50];
  // for(int i=1; i<30;i++)
  // {
  // sprintf(buf,"Schedule/Mar/%d",i);
  // Firebase.RTDB.setString(&fbdo,buf,"24:0,24:0,24:0,24:0");
  // delay(500);
  // }
}

void loop()
{
  check_firebase_request();
  check_serial_request();
  check_slave_request();
}

static void check_slave_request(void)
{

}
                
static void check_firebase_request(void)
{
  if( Firebase.ready() && signupOK)
  {
    if(!Firebase.RTDB.readStream(&fbdo_stream)) Serial.printf("Stream read error, %s\n\n",fbdo_stream.errorReason().c_str());
    
    if( fbdo_stream.streamAvailable())
    {
      if(fbdo_stream.intData()==device_id)
      {
        Firebase.RTDB.setInt(&fbdo,"Device_ID",0);
        FirebaseJson json;
        FirebaseJsonData result;
        String topic;
        String value;
        char buffer[200];
        char serial[200];
        if(Firebase.RTDB.getJSON(&fbdo,"Server_Request"))json=fbdo.jsonObject();
        if(json.get(result,"topic"))topic=result.stringValue;
        if(json.get(result,"value"))value=result.stringValue;
        if(topic=="url")
        {
          if(value=="?")
          {
            ESP32_Flash.get_firebase_url(buffer);
            sprintf(serial,"{\"topic\":\"url\",\"value\":\"%s\"}",buffer);
            if(json.setJsonData(serial))Firebase.RTDB.setJSON(&fbdo,"Device_Response",&json);
          }
          else
          {
            if(value!=NULL)
            {
              int len=value.length()+1;
              value.toCharArray(buffer,len);
              ESP32_Flash.set_firebase_url(buffer,len);
              sprintf(serial,"{\"topic\":\"status\",\"value\":\"ok\"}");
              if(json.setJsonData(serial))Firebase.RTDB.setJSON(&fbdo,"Device_Response",&json);
            }
          }
        }
        else if(topic=="api_key")
        {
         if(value=="?")
         {
           ESP32_Flash.get_firebase_api_key(buffer);
           sprintf(serial,"{\"topic\":\"api_key\",\"value\":\"%s\"}",buffer);
           if(json.setJsonData(serial))Firebase.RTDB.setJSON(&fbdo,"Device_Response",&json);
         }
         else
         {
           if(value!=NULL)
           {
             int len=value.length()+1;
             value.toCharArray(buffer,len);
             ESP32_Flash.set_firebase_api_key(buffer,len);
             sprintf(serial,"{\"topic\":\"status\",\"value\":\"ok\"}");
             if(json.setJsonData(serial))Firebase.RTDB.setJSON(&fbdo,"Device_Response",&json);
           }
         }
        }

        else if(topic=="wifi_ssid")
        {
         if(value=="?")
         {
           ESP32_Flash.get_wifi_ssid(buffer);
           sprintf(serial,"{\"topic\":\"wifi_ssid\",\"value\":\"%s\"}",buffer);
           if(json.setJsonData(serial))Firebase.RTDB.setJSON(&fbdo,"Device_Response",&json);
         }
         else
         {
           if(value!=NULL)
           {
             int len=value.length()+1;
             value.toCharArray(buffer,len);
             ESP32_Flash.set_wifi_ssid(buffer,len);
             sprintf(serial,"{\"topic\":\"status\",\"value\":\"ok\"}");
             if(json.setJsonData(serial))Firebase.RTDB.setJSON(&fbdo,"Device_Response",&json);
           }
         }
       }

       else if(topic=="wifi_psk")
       {
         if(value=="?")
         {
           ESP32_Flash.get_wifi_psk(buffer);
           sprintf(serial,"{\"topic\":\"wifi_psk\",\"value\":\"%s\"}",buffer);
           if(json.setJsonData(serial))Firebase.RTDB.setJSON(&fbdo,"Device_Response",&json);
         }
         else
         {
           if(value!=NULL)
           {
             int len=value.length()+1;
             value.toCharArray(buffer,len);
             ESP32_Flash.set_wifi_psk(buffer,len);
             sprintf(serial,"{\"topic\":\"status\",\"value\":\"ok\"}");
             if(json.setJsonData(serial))Firebase.RTDB.setJSON(&fbdo,"Device_Response",&json);
           }
         }
       }

       else if(topic=="device_id")
       {
         if(value=="?")
         {
           sprintf(serial,"{\"topic\":\"device_id\",\"value\":\"%d\"}",ESP32_Flash.get_device_id());
           if(json.setJsonData(serial))Firebase.RTDB.setJSON(&fbdo,"Device_Response",&json);
         } 
        else
        {
          if(value!=NULL)
          {
            int len=value.length()+1;
            value.toCharArray(buffer,len);
            ESP32_Flash.set_device_id(atoi(buffer));
            sprintf(serial,"{\"topic\":\"status\",\"value\":\"ok\"}");
            if(json.setJsonData(serial))Firebase.RTDB.setJSON(&fbdo,"Device_Response",&json);
          }
        }
       }
       else
       {
          json.toString(serial);
          Serial2.println(serial);
          delay(100);
          if(check_serial2())
          {
            if(json.setJsonData(uart2.buffer))Firebase.RTDB.setJSON(&fbdo,"Device_Response",&json);
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
            Serial.println("{\"topic\":\"status\",\"value\":\"ok\"}");
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
            Serial.println("{\"topic\":\"status\",\"value\":\"ok\"}");
          }
        }
      }

      else if(topic=="device_id")
      {
        if(value=="?")
        {
          Serial.print("{\"topic\":\"device_id\",\"value\":\"");
          Serial.printf("%d",ESP32_Flash.get_device_id());
          Serial.println("\"}");
        }
        else
        {
          if(value!=NULL)
          {
            int len=value.length()+1;
            value.toCharArray(buffer,len);
            ESP32_Flash.set_device_id(atoi(buffer));
            Serial.println("{\"topic\":\"status\",\"value\":\"ok\"}");
          }
        }
      }


      else
      {
        Serial2.printf("%s",uart.buffer);
        delay(100);
        if(check_serial2())Serial.println(uart2.buffer);
      }
      
    }
  }
  //if(Serial2.available())Serial.write(Serial2.read());
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
  if( signupOK) if(!Firebase.RTDB.beginStream(&fbdo_stream,"Device_ID"))
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

static bool check_serial2()
{
  if(Serial2.available())
  {
    memset(uart2.buffer,'\0',256);
    uart2.ref=millis();
    uart2.index=0;
    while((unsigned long)(millis()-uart2.ref)<15)
    {
       if(Serial2.available())
       {
         uart2.ref=millis();
         uart2.buffer[uart2.index++]=Serial2.read();      
       }
    }
    return true;
  }
  return false;
}

void P_Memory::storeSchedule(uint8_t month, uint8_t day,uint8_t Start_hour, uint8_t Start_minute, uint8_t End_hour, uint8_t End_minute)
{
  if(month>12 || month==0)return;
  if(day>Number_Of_Days_In_Months[month-1] || day==0)return;    
  int EEPROM_Address=SCHEDULE_START_ADDRESS+month*70+(day-1)*2;
  uint8_t lsb=(Start_hour<<2) | (Start_minute/15);
  uint8_t msb=(End_hour<<2) | (End_minute/15);
  EEPROM.write(EEPROM_Address, lsb);
  EEPROM.write(EEPROM_Address+1, msb);  
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
int P_Memory::get_device_id(void)
{
  unsigned int id;
  unsigned char *ptr=(unsigned char *)&id;
  ptr[0]=EEPROM.read(CONTROLLER_ID_ADDRESS);
  ptr[1]=EEPROM.read(CONTROLLER_ID_ADDRESS+1);
  ptr[2]=EEPROM.read(CONTROLLER_ID_ADDRESS+2);
  ptr[3]=EEPROM.read(CONTROLLER_ID_ADDRESS+3);
  return id;
}
void P_Memory::set_device_id(int id)
{
  unsigned char *ptr=(unsigned char *)&id;
  EEPROM.write(CONTROLLER_ID_ADDRESS,ptr[0]);
  EEPROM.write(CONTROLLER_ID_ADDRESS+1,ptr[1]);
  EEPROM.write(CONTROLLER_ID_ADDRESS+2,ptr[2]);
  EEPROM.write(CONTROLLER_ID_ADDRESS+3,ptr[3]);
  EEPROM.commit();
}

