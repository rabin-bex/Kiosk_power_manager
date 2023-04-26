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


struct schedule
{
  uint8_t Start_hour;
  uint8_t Start_minute;
  uint8_t End_hour;
  uint8_t End_minute;
};



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
  void storeSchedule(uint8_t month, uint8_t day,schedule sc);
  schedule getSchedule(uint8_t month, uint8_t day);
}ESP32_Flash;

static void setup_network(void);
static void check_serial_request(void);
static void check_firebase_request(void);
static void check_slave_request(void);
static bool check_serial();
static bool check_serial2();
static bool check_month(String val,int * index);
static bool check_day(String val,uint8_t* month, uint8_t* day);
static bool check_query(String val,uint8_t* month, uint8_t* day);
static bool parse_schedule(String value,uint8_t month, uint8_t day,schedule* sc);

void setup() 
{
  Serial.begin(115200);
  Serial2.begin(115200,SERIAL_8N1,RXD2,TXD2);
  ESP32_Flash.memory_init();
  setup_network();
  delay(2000);
  // char buf[50];
  // for(int i=1; i<32;i++)
  // {
  // sprintf(buf,"Schedule/Jan/%d",i);
  // Firebase.RTDB.setString(&fbdo,buf,"0:0,0:0,24:0,24:0");
  // delay(300);
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
   if(check_serial2)
   {
    ;
   }
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
        else if(topic=="schedule")
        {
          char buf[50];
          String val;
          schedule sc;
          int index=0;
          uint8_t m,d;
          ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
          if(value=="Year")
          {
            char month[25];
            for( int i=0; i<12; i++)
            {
              switch(i)
              {
                case 0:
                strcpy(month,"Schedule/Jan/");
                break;
                case 1:
                strcpy(month,"Schedule/Feb/");
                break;
                case 2:
                strcpy(month,"Schedule/Mar/");
                break;
                case 3:
                strcpy(month,"Schedule/Apr/");
                break;
                case 4:
                strcpy(month,"Schedule/May/");
                break;
                case 5:
                strcpy(month,"Schedule/Jun/");
                break;
                case 6:
                strcpy(month,"Schedule/Jul/");
                break;
                case 7:
                strcpy(month,"Schedule/Aug/");
                break;
                case 8:
                strcpy(month,"Schedule/Sep/");
                break;
                case 9:
                strcpy(month,"Schedule/Oct/");
                break;
                case 10:
                strcpy(month,"Schedule/Nov/");
                break;
                case 11:
                strcpy(month,"Schedule/Dec/");
                break;
              }
              Serial.printf("Month:%s\n",month);
              for(int j=1; j<Number_Of_Days_In_Months[i]+1;j++)
              {
                sprintf(buf,"%s%d",month,j);
                if(Firebase.RTDB.getString(&fbdo,buf))
                {
                  val=fbdo.stringData();
                  if(parse_schedule(val,i+1,j,&sc))
                  {
                    sprintf(buf,"{\"topic\":\"schedule\",\"value\":\"%d/%d,%d:%d,%d:%d\"}",i+1,j,sc.Start_hour,sc.Start_minute,sc.End_hour,sc.End_minute);
                    Serial2.printf("%s",buf);
                    delay(100);
                    if(check_serial2())
                    Serial.printf("%s\n",uart2.buffer);
                  }
                }
                delay(200);
              }
           }
           
           sprintf(serial,"{\"topic\":\"status\",\"value\":\"ok\"}");
           if(json.setJsonData(serial))Firebase.RTDB.setJSON(&fbdo,"Device_Response",&json);
          }
          ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
         else if(check_day(value,&m,&d))
         {
           char month[25];
           switch(m)
           {
            case 1:
            strcpy(month,"Schedule/Jan/");
            break;
            case 2:
            strcpy(month,"Schedule/Feb/");
            break;
            case 3:
            strcpy(month,"Schedule/Mar/");
            break;
            case 4:
            strcpy(month,"Schedule/Apr/");
            break;
            case 5:
            strcpy(month,"Schedule/May/");
            break;
            case 6:
            strcpy(month,"Schedule/Jun/");
            break;
            case 7:
            strcpy(month,"Schedule/Jul/");
            break;
            case 8:
            strcpy(month,"Schedule/Aug/");
            break;
            case 9:
            strcpy(month,"Schedule/Sep/");
            break;
            case 10:
            strcpy(month,"Schedule/Oct/");
            break;
            case 11:
            strcpy(month,"Schedule/Nov/");
            break;
            case 12:
            strcpy(month,"Schedule/Dec/");
            break;
           }
           sprintf(buf,"%s%d",month,d);
           if(Firebase.RTDB.getString(&fbdo,buf))
           {
            val=fbdo.stringData();
            if(parse_schedule(val,m,d,&sc))
            {
               sprintf(buf,"{\"topic\":\"schedule\",\"value\":\"%d/%d,%d:%d,%d:%d\"}",m,d,sc.Start_hour,sc.Start_minute,sc.End_hour,sc.End_minute);
               Serial2.printf("%s",buf);
               delay(100);
               if(check_serial2())
               if(json.setJsonData(uart2.buffer))Firebase.RTDB.setJSON(&fbdo,"Device_Response",&json);
            }
          }
           
         }
          ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
          else if(check_month(value,&index))
          {
            int success_count=0;
            for(int i=1; i<Number_Of_Days_In_Months[index]+1;i++)
            {
              sprintf(buf,"Schedule/%s/%d",value,i);
              if(Firebase.RTDB.getString(&fbdo,buf))
              {
                val=fbdo.stringData();
                if(parse_schedule(val,index+1,i,&sc))
                {
                  success_count++;
                  sprintf(buf,"{\"topic\":\"schedule\",\"value\":\"%d/%d,%d:%d,%d:%d\"}",index+1,i,sc.Start_hour,sc.Start_minute,sc.End_hour,sc.End_minute);
                  Serial2.printf("%s",buf);
                  delay(100);
                  if(check_serial2())
                  Serial.printf("%s\n",uart2.buffer);
                }
              }
              delay(200);
            }
            if(success_count==Number_Of_Days_In_Months[index])
            {
              sprintf(serial,"{\"topic\":\"status\",\"value\":\"ok\"}");
              if(json.setJsonData(serial))Firebase.RTDB.setJSON(&fbdo,"Device_Response",&json);
            }
          }
         //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
         else if(check_query(value,&m,&d)) 
          {
            Serial2.printf("{\"topic\":\"schedule\",\"value\":\"%d/%d,?\"}",m,d);
            delay(200);
            if(check_serial2())
            {
              DeserializationError error = deserializeJson(Json.doc, uart2.buffer);
              if (error) 
              {
                Serial.println("{\"topic\":\"status\",\"value\":\"error\"}");
              }
              else
              {
                 String topic=Json.doc["topic"];
                 String value=Json.doc["value"];

                 if(topic=="schedule")
                 {
                  schedule sc=ESP32_Flash.getSchedule(m,d);
                  sprintf(serial,"{\"topic\":\"schedule\",\"value\":\"%s,%d:%d,%d:%d\"}",value,sc.Start_hour,sc.Start_minute,sc.End_hour,sc.End_minute);
                  if(json.setJsonData(serial))Firebase.RTDB.setJSON(&fbdo,"Device_Response",&json);
                 }
              }

            }
          }
          //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        
        }
        else if(topic=="api_key")
        {
          if (value == "?")
          {
            ESP32_Flash.get_firebase_api_key(buffer);
            sprintf(serial, "{\"topic\":\"api_key\",\"value\":\"%s\"}", buffer);
            if (json.setJsonData(serial))
              Firebase.RTDB.setJSON(&fbdo, "Device_Response", &json);
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
          label:
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

static bool check_query(String value,uint8_t* month, uint8_t* day)
{
  uint8_t start=0;
  uint8_t end=0;
  int len=value.length();
  String sub_val;
  end=value.indexOf(',',start);
  if(end<len)
  {
    sub_val=value.substring(start,end);
    if(sub_val!="?")
    return false;
  }
  start=end+1;
  end=value.indexOf('/',start);
  if(end<len)
  {
    *month=value.substring(start,end).toInt();
    if(*month<1 && *month>12)
    return false;
  }
  start=end+1;
  *day=value.substring(start,len).toInt();
  if(*day>Number_Of_Days_In_Months[*month-1] && *day<1 )return false;
  return true;
}



static bool check_day(String value,uint8_t *month, uint8_t *day)
{
  uint8_t start=0;
  uint8_t end=0;
  int len=value.length();
  String sub_val;
  end=value.indexOf(',',start);
  if(end<len)
  {
    sub_val=value.substring(start,end);
    if(sub_val!="Day")
    return false;
  }
  start=end+1;
  end=value.indexOf('/',start);
  if(end<len)
  {
    *month=value.substring(start,end).toInt();
    if(*month<1 && *month>12)
    return false;
  }
  start=end+1;
  *day=value.substring(start,len).toInt();
  if(*day>Number_Of_Days_In_Months[*month-1] && *day<1 )return false;
  return true;
}

static bool parse_schedule(String value,uint8_t month, uint8_t day,schedule* sc)
{
  uint8_t start=0;
  uint8_t end=0;
  schedule sc2;
  int len=value.length();
  end=value.indexOf(':',start);
  if(end<len)
  {
    sc->Start_hour=value.substring(start,end).toInt();
    if(sc->Start_hour>23)return false;
  }else return false;
  start=end+1;
  end=value.indexOf(',',start);
  if(end<len)
  {
    sc->Start_minute=value.substring(start,end).toInt();
    if(sc->Start_minute>59)return false;
  }else return false;
  start=end+1;
  end=value.indexOf(':',start);
  if(end<len)
  {
    sc->End_hour=value.substring(start,end).toInt();
    if(sc->End_hour>24)return false;
  }else return false;
  start=end+1;
  end=value.indexOf(',',start);
  if(end<len)
  {
    sc->End_minute=value.substring(start,end).toInt();
    if(sc->End_minute>59)return false;
  }else return false;
  start=end+1;
  end=value.indexOf(':',start);
  if(end<len)
  {
    sc2.Start_hour=value.substring(start,end).toInt();
    if(sc2.Start_hour>24)return false;
  }else return false;
  start=end+1;
  end=value.indexOf(',',start);
  if(end<len)
  {
    sc2.Start_minute=value.substring(start,end).toInt();
    if(sc2.Start_minute>59)return false;
  }else return false;
  start=end+1;
  end=value.indexOf(':',start);
  if(end<len)
  {
    sc2.End_hour=value.substring(start,end).toInt();
    if(sc2.End_hour>24)return false;
  }else return false;
  start=end+1;
  sc2.End_minute=value.substring(start,len).toInt();
  if(sc2.End_minute>59) return false;
  ESP32_Flash.storeSchedule(month,day,sc2);
  return true;
}

static bool check_month(String val,int * index)
{
  if( val=="Jan" )*index=0;
  else if( val=="Feb" )*index=1;
  else if( val=="Mar" )*index=2;
  else if( val=="Apr" )*index=3;
  else if( val=="May" )*index=4;
  else if( val=="Jun" )*index=5;
  else if( val=="Jul" )*index=6;
  else if( val=="Aug" )*index=7;
  else if( val=="Sep" )*index=8;
  else if( val=="Oct" )*index=9;
  else if( val=="Nov" )*index=10;
  else if( val=="Dec" )*index=11;
  else return false;
  return true;
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
        delay(300);
        if(check_serial2())Serial.println(uart2.buffer);
      }
      
    }
  }
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

void P_Memory::storeSchedule(uint8_t month, uint8_t day, schedule sc)
{
  if(month>12 || month==0)return;
  if(day>Number_Of_Days_In_Months[month-1] || day==0)return;    
  int EEPROM_Address=SCHEDULE_START_ADDRESS+month*70+(day-1)*2;
  uint8_t lsb=(sc.Start_hour<<2) | (sc.Start_minute/15);
  uint8_t msb=(sc.End_hour<<2) | (sc.End_minute/15);
  EEPROM.write(EEPROM_Address, lsb);
  EEPROM.write(EEPROM_Address+1, msb);  
  EEPROM.commit();
}

schedule P_Memory::getSchedule(uint8_t month, uint8_t day)
{
  schedule sc{ .Start_hour=0, .Start_minute=0,.End_hour=0,.End_minute=0};
  if(month>12 || month==0)return sc;
  if(day>Number_Of_Days_In_Months[month-1] || day==0)return sc;    
  int EEPROM_Address=SCHEDULE_START_ADDRESS+month*70+(day-1)*2;
  uint8_t lsb=EEPROM.read(EEPROM_Address);
  uint8_t msb=EEPROM.read(EEPROM_Address+1);
  sc.Start_hour=lsb>>2;
  sc.Start_minute=(lsb & 0x03)*15;   
  sc.End_hour=msb>>2;
  sc.End_minute=(msb & 0x03)*15;
  return sc;
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

