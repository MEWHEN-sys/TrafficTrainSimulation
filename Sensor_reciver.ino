#include <WiFiS3.h>
#include <ArduinoMqttClient.h>

// -----------------------------------------
const char* mqtt_broker   = "mqtt-dashboard.com";
const int   mqtt_port     = 1883;
const char* topic_publish = "---";
// -----------------------------------------
const char* ssid = "---";
const char* pass = "*****";
// -----------------------------------------
const int threshold      = 200;
const int beforeLdrPin   = A5;

const unsigned long RECONNECT_INTERVAL = 5000;  // ms between reconnect attempts
const unsigned long SENSOR_INTERVAL    = 50;    // ms between sensor reads
// -----------------------------------------

WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

unsigned long lastReconnectAttempt = 0;
unsigned long lastSensorRead = 0;
bool wasDetected = false;

// -----------------------------------------
// Non-blocking reconnect: call every loop()
// -----------------------------------------
void maintainConnection() {
  // Already fine -> nothing to do
  if (WiFi.status() == WL_CONNECTED && mqttClient.connected()) return;

  // Rate-limit attempts
  unsigned long now = millis();
  if (now - lastReconnectAttempt < RECONNECT_INTERVAL) return;
  lastReconnectAttempt = now;

  // Step 1: WiFi
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi lost, reconnecting...");
    WiFi.begin(ssid, pass);   // one attempt only, no loop
    return;                   // check result on the next interval
  }

  // Step 2: MQTT (WiFi is up)
  Serial.println("Connecting to MQTT...");
  if (mqttClient.connect(mqtt_broker, mqtt_port)) {
    Serial.println("MQTT connected!");
    publishMsg("SensorReady");
  } else {
    Serial.print("MQTT failed, error = ");
    Serial.println(mqttClient.connectError());
  }
}

// Only publish if actually connected; otherwise skip silently
void publishMsg(const char* msg) {
  if (!mqttClient.connected()) return;
  mqttClient.beginMessage(topic_publish);
  mqttClient.print(msg);
  mqttClient.endMessage();
  Serial.print("Published: ");
  Serial.println(msg);
}

void setup() {
  Serial.begin(9600);
  mqttClient.setId("ArduinoaawoowaigwiiR4Client");
  mqttClient.setUsernamePassword("", "");
  mqttClient.setConnectionTimeout(2000);  // cap how long connect() can block
  lastReconnectAttempt = millis() - RECONNECT_INTERVAL;  // try immediately
}

void loop() {
  maintainConnection();
  if (mqttClient.connected()) mqttClient.poll();

  int beforeValue = analogRead(beforeLdrPin);
  Serial.print("before: ");
  Serial.println(beforeValue);

  if (beforeValue >= threshold) {
    Serial.print("Publishing message to topic ");
    mqttClient.beginMessage("-----", false, 0, false);
    mqttClient.print("BeforeLDRDetected");
    mqttClient.endMessage();
    Serial.println("BeforeLDRDetected");

    while (beforeValue >= threshold) {
      maintainConnection();                       // keep reconnecting while waiting
      if (mqttClient.connected()) mqttClient.poll();

      beforeValue = analogRead(beforeLdrPin);
      delay(50);
      Serial.print("befores: ");
      Serial.print(beforeValue);
      Serial.print(" >= ");
      Serial.println(threshold);
    }
    delay(120);
    mqttClient.beginMessage("---", false, 0, false);
    mqttClient.print("BeforeLDRNotDetected");
    mqttClient.endMessage();
    Serial.println("BeforeLDRNotDetected");
  }

  delay(25);
}