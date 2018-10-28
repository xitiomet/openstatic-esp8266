#include <NTPClient.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266mDNS.h>
#include <EEPROM.h>
#include <Arduino.h>
#include "webfiles.h"

extern "C" {
#include "user_interface.h"
}

// for push clock
unsigned long t0 = millis();

char serialInputBuffer[200];

// ------ HARDCODED SETTINGS ------
const char eeprom_marker = '@';
// --------------------------------

ESP8266WebServer server ( 80 );
WiFiUDP ntpUDP;
WiFiUDP eventUDP;
NTPClient timeClient(ntpUDP, "time.nist.gov", 0, 3600000);

wl_status_t wl_status;
struct settings_t
{
  WiFiMode_t wifi_mode;
  char ssid[64];
  char password[32];
  char hostname[64];
  char push_url[256];
  int push_interval;
  int time_offset;
  int multicast_port;
  int serial_baudrate;
  boolean debug;
};

struct keyvalue_t
{
  char *key;
  char *value;
  int key_size = 0;
  int value_size = 0;
  boolean key_alloc = false;
  boolean value_alloc = false;
};

settings_t settings;
keyvalue_t storage[128];

const int led = 13;

boolean gpio0_triggered = false;

// find a storage index for a particular key
int storageIndex(const char *key)
{
  for (int i = 0; i < 128; i++)
  {
    if (storage[i].key_alloc)
    {
      if (strcmp(key, storage[i].key) == 0)
      {
        return i;
      }
    }
  }
  return -1;
}

// find a free storage slot
int getFreeStorageSlot()
{
  for (int i = 0; i < 128; i++)
  {
    if (!storage[i].key_alloc)
    {
      return i;
    }
  }
  return -1;
}

// update the value of a keyvalue_t object (memory management included)
// this should only really be called by the store function
void updateValue(keyvalue_t *ptr, const char *value)
{
  if (ptr->value_alloc)
  {
    debug("MEM", "free memory for existing value");
    free(ptr->value);
  }
  ptr->value_size = strlen(value) + 1;
  ptr->value = (char*)malloc(sizeof(char) * ptr->value_size);
  ptr->value_alloc = true;
  strcpy(ptr->value, value);
}

// remove a storage set by its key.
void unset(const char *key)
{
  int si = storageIndex(key);
  if (si >= 0)
  {
    debug("MEM", "unset, exists deleting");
    keyvalue_t *ptr = &storage[si];
    if (ptr->key_alloc)
    {
      debug("MEM", "free memory for existing key");
      free(ptr->key);
      ptr->key_size = 0;
      ptr->key_alloc = false;
    }
    if (ptr->value_alloc)
    {
      debug("MEM", "free memory for existing value");
      ptr->value_size = 0;
      ptr->value_alloc = false;
      free(ptr->value);
    }
  } 
}

// save a key value pair to storage
void store(const char *key, const char *value)
{
  // we are storing a value that should trigger an event
  if (strcmp(key, "gpio2") == 0)
  {
    if (strcmp(value, "false") == 0)
    {
      digitalWrite(2, LOW);
    } else if (strcmp(value, "true") == 0) {
      digitalWrite(2, HIGH);
    }
  }

  // here is the real storage code
  int si = storageIndex(key);
  if (si >= 0)
  {
    debug("MEM", "Key exists updating");
    // keypair already exists just update the value
    updateValue(&storage[si], value);
  } else {
    debug("MEM", "Pair needs to be created");
    int nextFreeStorageSpot = getFreeStorageSlot();
    if (!storage[nextFreeStorageSpot].key_alloc)
    {
      debug("MEM", "Key needs to be created");
      storage[nextFreeStorageSpot].key_size = strlen(key)+1;
      debug("KeySize", String(storage[nextFreeStorageSpot].key_size).c_str());
      debug("MEM", "calling malloc() for key");
      storage[nextFreeStorageSpot].key = (char*)malloc(sizeof(char) * storage[nextFreeStorageSpot].key_size);
      storage[nextFreeStorageSpot].key_alloc = true;
    }
    debug("MEM", "strcpy() for key");
    strcpy(storage[nextFreeStorageSpot].key, key);
    debug("MEM", "time to update value");
    updateValue(&storage[nextFreeStorageSpot], value);
  }
}

// recall a key value pair from storage using the key name
char* recall(const char *key)
{
  int si = storageIndex(key);
  if (si >= 0)
  {
    return storage[si].value;
  } else {
    return "";
  }
}

// save a new value to settings
void updateSetting(String variable, String value)
{
  if (variable.equals("ssid"))
  {
    strcpy(settings.ssid, value.c_str());
  } else if (variable.equals("password")) {
    strcpy(settings.password, value.c_str());
  } else if (variable.equals("hostname")) {
    strcpy(settings.hostname, value.c_str());
  } else if (variable.equals("push_url")) {
    strcpy(settings.push_url, value.c_str());
  } else if (variable.equals("push_interval")) {
    settings.push_interval = value.toInt();
  } else if (variable.equals("timezone_offset")) {
    settings.time_offset = value.toInt();
    loadTimeOffset();
  } else if (variable.equals("multicast_port")) {
    settings.multicast_port = value.toInt();
  } else if (variable.equals("serial_baudrate")) {
    settings.serial_baudrate = value.toInt();
    Serial.flush();
    Serial.begin(settings.serial_baudrate);
  } else if (variable.equals("debug")) {
    if (value.equals("true"))
      settings.debug = true;
    else
      settings.debug = false;
  } else if (variable.equals("wifi_mode")) {
    settings.wifi_mode = (WiFiMode_t) value.toInt();
  }
}

// All debug notices should call this, since it can be controlled
void debug(String bumper, String text)
{
  if (settings.debug)
  {
    Serial.print("#debug# ");
    Serial.print(bumper);
    Serial.print(" : ");
    Serial.println(text);
  }
}

long getTimeOffset()
{
  return settings.time_offset * 3600;
}

void loadTimeOffset()
{
    timeClient.setTimeOffset(getTimeOffset());
}

// write settings object to eeprom
void saveSettings()
{
  EEPROM.write(0, eeprom_marker);
  EEPROM.put(1, settings);
  EEPROM.commit();
  debug("EEPROM", "Writing");
}

// load settings if available otherwise create them
void loadSettings()
{
  byte init_byte = EEPROM.read(0);
  if (init_byte == eeprom_marker) // marker that the eeprom has been initialized
  {
    debug("EEPROM", "Reading!");
    EEPROM.get(1, settings);
    //Serial.println((char*)settings);
  } else {
    // no settings exist, we must create them
    debug("EEPROM", "Writing Initial Settings!");
    EEPROM.write(0, eeprom_marker);
    settings_t x = {WIFI_STA, "XiN1","house777","openstatic_esp8266", "http://openstatic.org/webhook/", 0, -5, 2311, 9600, false};
    EEPROM.put(1, x);
    EEPROM.commit();
    debug("EEPROM", "Reading!");
    EEPROM.get(1, settings);
  }
  loadTimeOffset();
}

// this is the only right way to turn storage data into json
void generateJSONObject(char* bfr, int s)
{
  DynamicJsonBuffer jsonBuffer;
  JsonObject& rootObject = jsonBuffer.createObject();
  rootObject["_unixts"] = timeClient.getEpochTime();
  rootObject["_utc"] = timeClient.getEpochTime() - getTimeOffset();
  rootObject["_time"] = timeClient.getFormattedTime();
  rootObject["_hostname"] = settings.hostname;
  rootObject["_freemem"] = system_get_free_heap_size();
  for (int i = 0; i < 128; i++)
  {
    if (storage[i].key_alloc && storage[i].value_alloc)
    {
      if (isFloat(storage[i].value))
      {
        rootObject[storage[i].key] = atof(storage[i].value);
      } else if (strcmp(storage[i].value, "true") == 0) {
        rootObject[storage[i].key] = true;
      } else if (strcmp(storage[i].value, "false") == 0) {
        rootObject[storage[i].key] = false;
      } else {
        rootObject[storage[i].key] = storage[i].value;
      }
    }
  }
  rootObject.printTo(bfr, s);
}

// time to transmit our data to a server!
void doPush()
{
   digitalWrite ( led, 1 );
   HTTPClient http;
   http.begin(settings.push_url); //HTTP
   http.addHeader("Content-Type", "text/javascript");
   char out[2048];
   generateJSONObject(out, 2048);
   int httpCode = http.POST(out);
   if(httpCode > 0)
   {
     if(httpCode == HTTP_CODE_OK)
     {
        String payload = http.getString();
        debug("PUSH RESPONSE", payload);
     }
   }
   http.end();
   digitalWrite ( led, 0 );
}

void udpBroadcast(const char * str)
{
  if (wl_status == WL_CONNECTED && settings.multicast_port > 0)
  {
    IPAddress broadcastIp = WiFi.localIP();
    broadcastIp[3] = 255;
    debug("UDP_DATA", str);
    eventUDP.beginPacket(broadcastIp, settings.multicast_port);
    eventUDP.print(settings.hostname);
    eventUDP.print("/");
    eventUDP.println(str);
    eventUDP.endPacket();
  } else {
    debug("UDP", "Broadcast Attempted, not connected");
  }
}

void handleNotFound()
{
	String message = "File Not Found\n\n";
	message += "URI: ";
	message += server.uri();
	message += "\nMethod: ";
	message += ( server.method() == HTTP_GET ) ? "GET" : "POST";
	message += "\nArguments: ";
	message += server.args();
	message += "\n";
	for ( uint8_t i = 0; i < server.args(); i++ ) {
		message += " " + server.argName ( i ) + ": " + server.arg ( i ) + "\n";
	}
	server.send ( 404, "text/plain", message );
}

String renderTemplate(String body)
{
  String return_body = body;
  return_body.replace("<%=ssid%>", String(settings.ssid));
  return_body.replace("<%=password%>", String(settings.password));
  return_body.replace("<%=hostname%>", String(settings.hostname));
  return_body.replace("<%=push_url%>", String(settings.push_url));
  return_body.replace("<%=push_interval%>", String(settings.push_interval));
  return_body.replace("<%=timezone_offset%>", String(settings.time_offset));
  return_body.replace("<%=multicast_port%>", String(settings.multicast_port));
  return_body.replace("<%=serial_baudrate%>", String(settings.serial_baudrate));
  return_body.replace("<%=debug.checked%>", settings.debug ? "checked" : "");
  if (settings.wifi_mode == WIFI_OFF)
    return_body.replace("<%=wifi_mode_off.selected%>", " selected");
  else
    return_body.replace("<%=wifi_mode_off.selected%>", "");
  if (settings.wifi_mode == WIFI_STA)
    return_body.replace("<%=wifi_mode_station.selected%>", " selected");
  else
    return_body.replace("<%=wifi_mode_station.selected%>", "");
  if (settings.wifi_mode == WIFI_AP)
    return_body.replace("<%=wifi_mode_access.selected%>", " selected");
  else
    return_body.replace("<%=wifi_mode_access.selected%>", "");
  return return_body;
}

// what we respond to /api.json with....
void jsonRespond( void )
{
  digitalWrite ( led, 1 );
  char out[2048];
  generateJSONObject(out, 2048);
  server.send(200, "text/javascript", out);
  digitalWrite ( led, 0 );
}

// what we respond to /api.json with....
void setRespond( void )
{
  char* msg = "OK";
  digitalWrite ( led, 1 );
  DynamicJsonBuffer jsonBuffer;
  JsonObject& rootObject = jsonBuffer.createObject();
  for ( uint8_t i = 0; i < server.args(); i++ )
  {
    String argname = server.argName(i);
    String argvalue = server.arg(i);
    const char* c_name = argname.c_str();
    const char* c_value = argvalue.c_str();
    if (argname.substring(0,1).equals("$"))
    {
      msg = "FAILED: Cannot change settings this way!";
    } else {
      Serial.print(argname);
      Serial.print("=");
      Serial.println(argvalue);
      store(c_name, c_value);
    }
  }
  char out[2048];
  rootObject["_unixts"] = timeClient.getEpochTime();
  rootObject["_utc"] = timeClient.getEpochTime() - getTimeOffset();
  rootObject["_time"] = timeClient.getFormattedTime();
  rootObject["_hostname"] = settings.hostname;
  rootObject["status"] = msg;
  rootObject.printTo(out, 2048);
  server.send(200, "text/javascript", out);
  digitalWrite ( led, 0 );
}

void initWifi()
{
  WiFi.mode ( settings.wifi_mode );
  WiFi.hostname( settings.hostname );
  if (settings.wifi_mode == WIFI_STA)
  {
    WiFi.begin ( settings.ssid, settings.password );
    // Wait for connection
    int tries = 0;
    while ( WiFi.status() != WL_CONNECTED && tries < 10)
    {
      delay ( 500 );
      debug("WIFI Attempt", String(tries)); 
      tries++;
    }
  } else if (settings.wifi_mode == WIFI_AP) {
    if (strcmp(settings.password, "") == 0)
    {
      WiFi.softAP(settings.ssid);
    } else {
      WiFi.softAP(settings.ssid, settings.password);
    }
  }
}

void setup ( void )
{
  EEPROM.begin(512);
	pinMode ( led, OUTPUT );
  pinMode( 2, OUTPUT );
  pinMode( 0, INPUT );

	digitalWrite ( led, 0 );
  loadSettings();
  Serial.begin ( settings.serial_baudrate );
  Serial.println ( "" );
  
	initWifi();

	querySSID();
	queryIP();

	server.on ( "/", []() 
	{
    debug("WebServer", "Requested index.html");
	  server.send ( 200, "text/html", renderTemplate(INDEX_MIN_HTML));
	});
  server.on ( "/setup.html", HTTP_POST, []()
  {
    debug("WebServer", "POST to setup.html");
    boolean debug_checked = false;
    for ( uint8_t i = 0; i < server.args(); i++ )
    {
      String argname = server.argName(i);
      String argvalue = server.arg(i);
      if (argname.equals("debug"))
      {
        debug_checked = true;
      } else if (!argname.equals("do")) {
        Serial.print("$");
        Serial.print(argname);
        Serial.print("=");
        Serial.println(argvalue);
        updateSetting(argname, argvalue);
      }
    }
    Serial.print("$debug=");
    if (debug_checked)
    {
      updateSetting("debug", "true");
      Serial.println("true");
    } else {
      updateSetting("debug", "false");
      Serial.println("false");
    }
    saveSettings();
    server.send( 200, "text/html", renderTemplate(SETUP_MIN_HTML));
  });
  server.on("/setup.html", HTTP_GET, []() 
  {
    debug("WebServer", "Requested setup.html");
    server.send ( 200, "text/html", renderTemplate(SETUP_MIN_HTML));
  });
  server.on("/api.json", jsonRespond );
  server.on("/set.json", setRespond );
	server.on("/index.js", []() 
	{
    debug("WebServer", "Requested index.js");
    server.send ( 200, "text/javascript", INDEX_MIN_JS);
  });
	server.onNotFound ( handleNotFound );
	server.begin();
  timeClient.begin();
  if (settings.multicast_port > 0)
  {
    eventUDP.begin(settings.multicast_port);
  }
}

// Determine if a string contains a floating point number
boolean isFloat(char * str)
{
   for(byte i = 0; i < strlen(str); i++)
   {
      char c = str[i];
      if(!isDigit(c) && c != '.' && c != '-') return false;
   }
   return true;
}

// this is where we handle incoming commands via multicast
void processMulticast(String data)
{
  int eq_loc = data.indexOf("=");
  if (eq_loc != -1)
  {
    String variable = data.substring(0,eq_loc);
    //multicast variables must have a path
    int sl_loc = variable.indexOf("/");
    if (sl_loc != -1)
    {
      String value = data.substring(eq_loc+1);
      value.trim();
      const char *variable_c = variable.c_str();
      const char *value_c = value.c_str();
      if (value.equals("")) {
        unset(variable_c);
        Serial.print(variable);
        Serial.println("=");
      } else {
        store(variable_c, value_c);
        Serial.print(variable);
        Serial.print("=");
        Serial.println(value);
      }
    }
  }
}
// this is where we handle incoming commands via serial
void processSerial(String data)
{
  int eq_loc = data.indexOf("=");
  if (data.startsWith("*"))
  {
    // a packet intended for multicast stuff
    data = data.substring(1, data.length());
    udpBroadcast(data.c_str());
  } else if (eq_loc != -1) {
    String variable = data.substring(0,eq_loc);
    String value = data.substring(eq_loc+1);
    const char *variable_c = variable.c_str();
    const char *value_c = value.c_str();
    if (variable.substring(0,1).equals("$"))
    {
      variable = variable.substring(1);
      Serial.print("$");
      Serial.print(variable);
      Serial.print("=");
      updateSetting(variable, value);
      Serial.println(value);
      saveSettings();
    } else if (value.equals("")) {
      unset(variable_c);
    } else {
      store(variable_c, value_c);
    }
  } else if (data.endsWith("?")) {
    data.toLowerCase();
    String variable = data.substring(0, data.length()-1);
    const char *variable_c = variable.c_str();
    if (variable.equals("_ip"))
    {
      queryIP();
    } else if (variable.equals("_ssid")) {
      querySSID();
    } else if (variable.equals("_time")) {
      queryTime();
    } else if (variable.equals("_epoch")) {
      queryEpoch();
    } else if (storageIndex(variable_c) >= 0) { // allow query of root obj
      Serial.print(variable);
      Serial.print("=");
      Serial.println(recall(variable_c));
    } else if (variable.substring(0,1).equals("$")) { // allow querying of settings
      variable = variable.substring(1);
      Serial.print("$");
      Serial.print(variable);
      Serial.print("=");
      if (variable.equals("ssid"))
      {
        Serial.println(settings.ssid);
      } else if (variable.equals("password")) {
        Serial.println(settings.password);
      } else if (variable.equals("wifi_mode")) {
        Serial.println(settings.wifi_mode);
      } else if (variable.equals("hostname")) {
        Serial.println(settings.hostname);
      } else if (variable.equals("push_url")) {
        Serial.println(settings.push_url);
      } else if (variable.equals("push_interval")) {
        Serial.println(settings.push_interval);
      } else if (variable.equals("timeszone_offset")) {
        Serial.println(settings.time_offset);
      } else if (variable.equals("multicast_port")) {
        Serial.println(settings.multicast_port);
      } else if (variable.equals("serial_baudrate")) {
        Serial.println(settings.serial_baudrate);
      } else if (variable.equals("debug")) {
        if (settings.debug)
          Serial.println("true");
        else
          Serial.println("false");
      }
    }
  }
}

void queryTime()
{
  Serial.print ( "_TIME=" );
  Serial.println(timeClient.getFormattedTime());
}

void queryEpoch()
{
  Serial.print ( "_EPOCH=" );
  Serial.println(timeClient.getEpochTime());
}

void queryIP()
{
  Serial.print ( "_IP=" );
  if (settings.wifi_mode == WIFI_STA)
  {
    Serial.println ( WiFi.localIP() );
  } else if (settings.wifi_mode == WIFI_AP) {
    Serial.println( WiFi.softAPIP() );
  } else {
    Serial.println("0.0.0.0");
  }
}

void querySSID()
{
  Serial.print ( "_SSID=" );
  Serial.println ( settings.ssid );
}

void str_append(char* s, char c)
{
  int len = strlen(s);
  s[len] = c;
  s[len+1] = '\0';
}

// stuff to do whenever there is serial data available
void serialEvent()
{
  while (Serial.available())
  {
    char inChar = (char)Serial.read(); 
    if (inChar == '\n' || inChar == '\r') {
      if (strlen(serialInputBuffer) > 0)
      {
        processSerial(String(serialInputBuffer));
        serialInputBuffer[0] = '\0';
      }
    } else {
      str_append(serialInputBuffer, inChar); 
    }
  }
}

void loop ( void )
{
  // Check Wifi Status
  wl_status = WiFi.status();

  // only do wifi stuff if we are connected
  if (wl_status == WL_CONNECTED)
  {
	  server.handleClient();
    timeClient.update();
    if (settings.push_interval > 0)
    {
      if ((millis() - t0) > (settings.push_interval * 1000))
      {
        t0 = millis();
        doPush();
      }
    }
  }

  // read GPIO 0 State
  int gpio0State = digitalRead(0);
  if (gpio0State == HIGH)
  {
    if (gpio0_triggered)
    {
      gpio0_triggered = false;
      store("gpio0", "false");
      Serial.println("gpio0=false");
    }
  } else if (gpio0State == LOW) {
    if (!gpio0_triggered)
    {
      gpio0_triggered = true;
      store("gpio0", "true");
      Serial.println("gpio0=true");
    }
  }

  // Handle Serial Events
  if (Serial.available())
  {
    serialEvent();
  }

  // Handle Multicast Packets
  if (settings.multicast_port > 0 && wl_status == WL_CONNECTED)
  {
    char incomingPacket[255];
    int packetSize = eventUDP.parsePacket();
    if (packetSize)
    {
      // receive incoming UDP packets
      int len = eventUDP.read(incomingPacket, 255);
      if (len > 0)
      {
        incomingPacket[len] = 0;
        String data = String(incomingPacket);
        processMulticast(data);
      }
    }
  }
}
