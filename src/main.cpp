#include <Arduino.h>
//#include <WebServer.h>
#include <WiFiManager.h>
#if defined(ESP8266)
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif
#if defined(ESP8266)
#include <ESP8266WebServer.h>
#else
#include <WebServer.h>
#endif
#include <PubSubClient.h>
#include <Chrono.h> 

const char* mqtt_server = "10.25.76.10"; //mqtt server
const char* consumptionTopic = "ESP_Easy/Eau/Total";
const char* mqttClientId = "showerCounter";

WebServer server(80);
WiFiManager wm;
WiFiClient espClient;
PubSubClient client(espClient);
Chrono myChrono(false); 

int waterThreshold = 100; // 100 litres
int initialIndex = -1;
unsigned long durationThreshold = 10 * 60000; /// 10 minutes
boolean showerStarted = false;
int currentIndex = -1;
unsigned long showerDuration = 0;

// HTML & CSS contents which display on web server
String HTML = "<!DOCTYPE html>\
<html>\
  <body>\
    <h1>Shower Counter</h1>\
    <p></p>\
    <p>&#128522;</p>\
  </body>\
</html>";
// Handle root url (/)
void handle_root() {
  server.send(200, "text/html", HTML);
}

void callback(char* topic, byte* payload, unsigned int length) {   //callback includes topic and payload ( from which (topic) the payload is comming)
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  payload[length] = '\0';
  currentIndex = atoi((char *)payload);
}

unsigned long getSeconds(unsigned long timeInMillis) {
  return (timeInMillis / 10000) % 60;
}

unsigned long getMinutes(unsigned long timeInMillis) {
  return getSeconds(timeInMillis)/60;
}

void reconnectMqtt() {
  while (!client.connected()) {
    Serial.println("Attempting MQTT connection...");
    if (client.connect(mqttClientId)) {
      Serial.println("connected");
      client.subscribe(consumptionTopic);

    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void connectMqtt()
{
  client.connect(mqttClientId); 
  {
    Serial.println("connected to MQTT");
    client.subscribe(consumptionTopic);

    if (!client.connected())
    {
      reconnectMqtt();
    }
  }
}

void disconnectMqtt() {
  if (client.connected()) {
    client.disconnect();
  }
}


void setup() {
  
  Serial.begin(115200);

  //reset saved settings
  //wm.resetSettings();
  wm.autoConnect();
  Serial.println("ShowerCounter is connected to Wi-Fi network");
  
  server.on("/", handle_root);

  server.begin();
  Serial.println("HTTP server started");

  client.setServer(mqtt_server, 1883);//connecting to mqtt server
  client.setCallback(callback);
  //delay(5000);
  
  delay(100);   
}

void startShower() {
  showerStarted = true;

  connectMqtt();

  while (currentIndex == -1) {
      sleep(1000);
  }
  initialIndex = currentIndex;


  if (myChrono.isRunning()) {
    myChrono.stop();
  }
  myChrono.start();
}

void stopShower() {
  disconnectMqtt();
  if (myChrono.isRunning()) {
    myChrono.stop();
  }
  // TODO: sleep + autostop ? Chrono to autostop ?
}

void stop() {
  // TODO: Go to sleep mode

}

void updateDisplay() {
  Serial.printf("Time: %lu:%lu\n", getMinutes(showerDuration), getSeconds(showerDuration));
  Serial.printf("Consumption: %d\n", currentIndex-initialIndex);
  // TODO
}

void loop() {
    server.handleClient();
    if (showerStarted) {
      if (!client.connected())
      {
        reconnectMqtt();
      }
      client.loop();
      updateDisplay();
      if ((currentIndex-initialIndex) >= waterThreshold || showerDuration >= durationThreshold) {
        Serial.printf("Threshold reached: water=%d / duration=%lu\n", (currentIndex-initialIndex), showerDuration);
        // Display bad icon
        // Buzzer
      }
    }
}

