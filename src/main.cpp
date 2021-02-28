#include <Arduino.h>

#include <Vault.h>

#include <SimpleTimer.h>    //https://github.com/marcelloromani/Arduino-SimpleTimer/tree/master/SimpleTimer
#include <ESP8266WiFi.h>    //if you get an error here you need to install the ESP8266 board manager 
#include <ESP8266mDNS.h>    //if you get an error here you need to install the ESP8266 board manager 
#include <PubSubClient.h>   //https://github.com/knolleary/pubsubclient
#include <ArduinoOTA.h>     //https://github.com/esp8266/Arduino/tree/master/libraries/ArduinoOTA

/******************************************************************************/

#define USER_MQTT_CLIENT_NAME     "leds1"  

/******************************************************************************/

WiFiClient espClient;
PubSubClient mqtt(espClient);
SimpleTimer timer;

const byte BUFFER_SIZE = 50;

bool boot = true;
char messageBuffer[BUFFER_SIZE];

const char* ssid = USER_SSID; 
const char* password = USER_PASSWORD;
const char* mqtt_server = USER_MQTT_SERVER;
const int mqtt_port = USER_MQTT_PORT;
const char *mqtt_user = USER_MQTT_USERNAME;
const char *mqtt_pass = USER_MQTT_PASSWORD;
const char *mqtt_client_name = USER_MQTT_CLIENT_NAME; 

/******************************************************************************/

const byte LEDPIN = D3;
int ledLevel = 0;

void wifi_setup() {
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void mqtt_connect() {
  int retries = 0;
  while (!mqtt.connected()) {
    if(retries < 150) {
      Serial.print("Attempting MQTT connection...");
      if (mqtt.connect(mqtt_client_name, mqtt_user, mqtt_pass)) {
        Serial.println("connected");
        mqtt.publish(USER_MQTT_CLIENT_NAME"/checkIn", boot ? "Reconnected" : "Rebooted" ); 
        mqtt.subscribe(USER_MQTT_CLIENT_NAME"/dim");
        boot = true;
      } 
      else {
        Serial.print("failed, rc=");
        Serial.print(mqtt.state());
        Serial.println(" try again in 5 seconds");
        retries++;
        delay(5000);
      }
    }
    
    if(retries > 149) {
      ESP.restart();
    }
  }
}

void checkIn() {
  mqtt.publish(USER_MQTT_CLIENT_NAME"/checkIn","OK"); 
}

void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  String newTopic = topic;
  Serial.print(topic);
  Serial.print("] ");
  
  payload[length] = '\0';
  String newPayload = String((char *)payload);
  long intPayload = newPayload.toInt();

  Serial.println(newPayload);
  Serial.println();
 
  if (newTopic == USER_MQTT_CLIENT_NAME"/dim") {
    ledLevel = intPayload;
    analogWrite(LEDPIN, ledLevel);
  }
}


void setup() {
  pinMode(LEDPIN, OUTPUT); 
  
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  wifi_setup();
  
  mqtt.setServer(mqtt_server, mqtt_port);
  mqtt.setCallback(mqtt_callback);
  
  ArduinoOTA.setHostname(USER_MQTT_CLIENT_NAME);
  ArduinoOTA.begin(); 
  delay(10);
  
  timer.setInterval(60000, checkIn);
}


void loop() {
  if (!mqtt.connected()) {
    mqtt_connect();
  }
  mqtt.loop();
  
  ArduinoOTA.handle();
  
  timer.run();
}