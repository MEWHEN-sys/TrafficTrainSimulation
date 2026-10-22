#define BLYNK_TEMPLATE_ID "YOUR_TEMPLATE_ID"
#define BLYNK_TEMPLATE_NAME "Quickstart Template"
#define BLYNK_AUTH_TOKEN    "YOUR_BLYNK_AUTH_TOKEN"

const char* ssid = "";
const char* pass = "";

#define BLYNK_PRINT Serial

#include <Servo.h>
#include <WiFiS3.h>
#include <BlynkSimpleWifi.h>
#include <ArduinoMqttClient.h>


// -----------------------------------------

const char* mqtt_broker = "";
const int mqtt_port = ;
const char* topic_subscribe = "";

// -----------------------------------------
const int threshold = 800;
const int afterLdrPin = A4;
// -----------------------------------------

const int buzzer = 5;

// -----------------------------------------
const int yellowLight = 7;
const int greenLight = 4;
const int redLight = 3;

// -----------------------------------------
const int barrier = 2;
int automatic = 1;
Servo myServo;
// -----------------------------------------
WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);
// -----------------------------------------


enum STATE {
  OPEN,
  WARNING,
  CLOSE
};

STATE status = STATE::OPEN;
STATE* statusPtr = &status;
STATE prevState = STATE::OPEN;   // gate state to return to after a manual-mode warning

const unsigned long warnCloseDelay = 2;  // ms of warning before the gate closes itself in manual mode
unsigned long warnStartTime = 0;
bool warnAutoClosed = false;

bool beforeLdr = false;

const char* stateName(STATE s);
void logStatus(STATE state, bool beforeValue, int afterValue);
void openGateAuto(STATE *state);
void warnAuto(STATE *state);
void closeGateAuto(STATE *state);
void warnManual(STATE *state);
void clear(STATE *state);
void openGate();
void closeGate();
void onMqttMessage(int messageSize);


BLYNK_CONNECTED() {
  Blynk.syncVirtual(V0, V1);
}

BLYNK_WRITE(V0) {
  if (automatic == 1) return;
  int value = param.asInt();
  if (value == 0) {
    Serial.println("Close gate");
    closeGate();
  } else if (value == 1) {
    Serial.println("Open gate");
    openGate();
  }
}


BLYNK_WRITE(V1) {
  int value = param.asInt();
  automatic = value;
  if (automatic == 0) {
    if (status == STATE::WARNING) {
      // Warning was running in auto mode (gate is closed), keep it running
      prevState = STATE::CLOSE;
      warnAutoClosed = true;   // gate is already closed
    } else {
      clear(statusPtr);
    }
  }
}

const char* stateName(STATE s) {
  switch (s) {
    case STATE::OPEN:    return "OPEN";
    case STATE::WARNING: return "WARNING";
    case STATE::CLOSE:   return "CLOSE";
    default:             return "UNKNOWN";
  }
}


void logStatus(STATE state, bool beforeValue, int afterValue) {
  static STATE lastState = (STATE)-1;
  static unsigned long lastLogTime = 0;
  const unsigned long logInterval = 1000;

  bool stateChanged = (state != lastState);
  bool timeElapsed  = (millis() - lastLogTime >= logInterval);

  if (stateChanged || timeElapsed) {
    // Blynk.virtualWrite(V3, String("[") + millis() + " ms] State: " + stateName(state) + " | Before LDR: " + beforeValue + " | After LDR: " + afterValue + " | Threshold: " + threshold);
    // Serial.print("Sent to Blynk: ");
    Serial.print("[");
    Serial.print(millis());
    Serial.print(" ms] State: ");
    Serial.print(stateName(state));
    Serial.print(" | Before LDR: ");
    Serial.print(beforeValue);
    Serial.print(" | After LDR: ");
    Serial.print(afterValue);
    Serial.print(" | Threshold: ");
    Serial.println(threshold);
    lastState = state;
    // Blynk.virtualWrite(V2, stateName(state));
    lastLogTime = millis();
  }
}


void clear(STATE *state) {
  digitalWrite(greenLight, LOW);
  digitalWrite(redLight, LOW);
  myServo.write(0);
  *state = STATE::OPEN;
}

void openGateAuto(STATE *state) {
  if (beforeLdr == 1) {
    digitalWrite(greenLight, LOW);
    *state = STATE::WARNING;
    beforeLdr = false;
    return;
  }
  digitalWrite(greenLight, HIGH);
  myServo.write(90);
  delay(50);
}

void warnAuto(STATE *state) {
  int lightValue = analogRead(afterLdrPin);

  if (lightValue >= threshold) {
    digitalWrite(buzzer, HIGH);
    digitalWrite(yellowLight, LOW);
    *state = STATE::CLOSE;
    return;
  }
  digitalWrite(buzzer, LOW);
  myServo.write(0);
  digitalWrite(yellowLight, HIGH);
  delay(50);
}

// Manual mode: warning (yellow light + buzzer) still works, barrier is not moved automatically
void warnManual(STATE *state) {
  // Train detected by the first sensor (MQTT)
  if (beforeLdr) {
    beforeLdr = false;
    if (*state != STATE::WARNING) {
      prevState = *state;
      *state = STATE::WARNING;
      warnStartTime = millis();
      warnAutoClosed = false;
    }
  }

  if (*state != STATE::WARNING) return;

  // Train reached the second sensor: end the warning, restore the gate state
  if (analogRead(afterLdrPin) >= threshold) {
    digitalWrite(buzzer, HIGH);
    digitalWrite(yellowLight, LOW);
    warnAutoClosed = false;
    *state = prevState;
    return;
  }

  // Enough warning time has passed: close the gate automatically (once)
  if (!warnAutoClosed && millis() - warnStartTime >= warnCloseDelay) {
    closeGate();               // sets prevState = CLOSE because status is WARNING
    warnAutoClosed = true;
  }

  // Warning active: yellow light and buzzer, servo is NOT touched
  digitalWrite(buzzer, LOW);
  digitalWrite(yellowLight, HIGH);
}

void closeGate() {
  digitalWrite(greenLight, LOW);
  digitalWrite(redLight, HIGH);
  myServo.write(0);

  if (status == STATE::WARNING) prevState = STATE::CLOSE;  // keep the warning running
  else status = STATE::CLOSE;
}

void openGate() {
  digitalWrite(redLight, LOW);
  digitalWrite(greenLight, HIGH);
  myServo.write(90);

  if (status == STATE::WARNING) prevState = STATE::OPEN;
  else status = STATE::OPEN;
}


void closeGateAuto(STATE *state) {
  int lightValue = analogRead(afterLdrPin);
  if (lightValue <= threshold) {
    digitalWrite(redLight, LOW);
    *state = STATE::OPEN;
    return;
  }
  digitalWrite(redLight, HIGH);
  myServo.write(0);
  delay(50);
}

void wificonnect(const char *ssid, const char *pass) {
  Serial.print("Attempting to connect to WPA SSID: ");
  Serial.println(ssid);
  while (WiFi.begin(ssid, pass) != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("\nWiFi Connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  mqttClient.setId("ArduinoR4Client");
  mqttClient.setUsernamePassword("", "");

  mqttClient.onMessage(onMqttMessage);

  Serial.print("Attempting to connect to the MQTT broker: ");
  Serial.println(mqtt_broker);
  if (!mqttClient.connect(mqtt_broker, mqtt_port)) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.connectError());
    while (1);
  }
  Serial.println("You're connected to the MQTT broker!");

  mqttClient.subscribe(topic_subscribe, 0);
  Serial.print("Subscribed to topic: ");
  Serial.println(topic_subscribe);
}

void onMqttMessage(int messageSize) {
  Serial.print("Received a message with topic '");
  Serial.print(mqttClient.messageTopic());
  Serial.print("', length ");
  Serial.print(messageSize);
  Serial.println(" bytes:");

  String message;
  while (mqttClient.available()) {
    char c = (char)mqttClient.read();
    message += c;
    Serial.print(c);
  }
  Serial.println();

  if (message == "BeforeLDRDetected") {
    beforeLdr = true;
  }
  Serial.print("BeforeLdr is now :");
  Serial.println(beforeLdr);
}

void setup() {
  Serial.begin(9600);
  myServo.attach(barrier);
  pinMode(afterLdrPin, INPUT);
  pinMode(greenLight, OUTPUT);
  pinMode(yellowLight, OUTPUT);
  pinMode(buzzer, OUTPUT);
  digitalWrite(buzzer, HIGH);
  pinMode(redLight, OUTPUT);
  wificonnect(ssid, pass);
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
}


void loop() {
  Blynk.run();
  mqttClient.poll();
  int afterValue = analogRead(afterLdrPin);

  if (automatic == 1) {
    switch (status) {
      case STATE::OPEN:    openGateAuto(statusPtr);  break;
      case STATE::WARNING: warnAuto(statusPtr);      break;
      case STATE::CLOSE:   closeGateAuto(statusPtr); break;
    }
  } else {
    warnManual(statusPtr);
  }

  logStatus(status, beforeLdr, afterValue);
  delay(25);
}
