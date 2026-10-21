#include <WiFiS3.h>         
#include <ArduinoMqttClient.h>


// -----------------------------------------

const char* mqtt_broker = "mqtt-dashboard.com";
const int mqtt_port = 1883;
const char* topic_publish = "train/helloworld";

// -----------------------------------------

const char* ssid = "Sern";
const char* pass = "0983350644";


// -----------------------------------------
const int threshold = 400;
const int beforeLdrPin = A5;
// -----------------------------------------
WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);
// -----------------------------------------


void logStatus(int beforeValue) {
  static unsigned long lastLogTime = 0;
  const unsigned long logInterval = 1000;

  bool timeElapsed  = (millis() - lastLogTime >= logInterval);


}

void wificonnect(const char *ssid,const char *pass) {
  Serial.print("Attempting to connect to WPA SSID: ");
  Serial.println(ssid);
  while (WiFi.begin(ssid, pass) != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("\nWiFi Connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  mqttClient.setId("ArduinoaawoowaigwiiR4Client");
  mqttClient.setUsernamePassword("", "");

  Serial.print("Attempting to connect to the MQTT broker: ");
  Serial.println(mqtt_broker);
  if (!mqttClient.connect(mqtt_broker, mqtt_port)) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.connectError());
    while (1);
  }
  Serial.println("You're connected to the MQTT broker!");


}

void setup() {
  Serial.begin(9600);
  wificonnect(ssid,pass);
}

void loop() {
  mqttClient.poll();

  int beforeValue = analogRead(beforeLdrPin);
  Serial.print("before: ");
  Serial.println(beforeValue);
  
  if (beforeValue >= threshold) {
    Serial.print("Publishing message to topic ");
    mqttClient.beginMessage(topic_publish);
    mqttClient.print("BeforeLDRDetected");
    mqttClient.endMessage();
    Serial.print("BeforeLDRDetected");
    while (beforeValue >= threshold) {
      int beforeValue = analogRead(beforeLdrPin);
      delay(500);
      Serial.print("befores: ");
      Serial.println(beforeValue);
    }
    Serial.print("BeforeLDRNotDetected");
  } 

  logStatus(beforeValue);
  delay(1000);
}