#define BLYNK_TEMPLATE_ID   "YOUR_TEMPLATE_ID"
#define BLYNK_TEMPLATE_NAME "YOUR_TEMPLATE_NAME"
#define BLYNK_AUTH_TOKEN    "YOUR_AUTH_TOKEN"

const char* ssid = "YOUR_WIFI_SSID";
const char* pass = "YOUR_WIFI_PASSWORD";

#define BLYNK_PRINT Serial

#include <Servo.h>
#include <WiFiS3.h>         
#include <BlynkSimpleWifi.h> 

const int threshold = 800;
const int beforeLdrPin = A0;
const int afterLdrPin = A1;
const int greenLight = 4;
const int redLight = 3;
const int barrier = 2;
int automatic = 0;
Servo myServo;

enum STATE {
  OPEN,
  WARNING,
  CLOSE
};

STATE status = STATE::OPEN;
STATE* statusPtr = &status; 

const char* stateName(STATE s);
void logStatus(STATE state, int beforeValue, int afterValue);
void openGateAuto(STATE *state);
void warnAuto(STATE *state);
void closeGateAuto(STATE *state);
void clear(STATE *state);
void openGate();
void closeGate();


BLYNK_CONNECTED() {
  Blynk.syncVirtual(V0, V1);
}

BLYNK_WRITE(V0) {
  if (automatic == 1) return;
  int value = param.asInt();
  if (value == 0) {
    closeGate();
  } else if (value == 1) {
    openGate();
  }
}


BLYNK_WRITE(V1) {
  int value = param.asInt();
  automatic = value;
  if (automatic == 0) {
    clear(statusPtr);
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


void logStatus(STATE state, int beforeValue, int afterValue) {
  static STATE lastState = (STATE)-1;
  static unsigned long lastLogTime = 0;
  const unsigned long logInterval = 1000;

  bool stateChanged = (state != lastState);
  bool timeElapsed  = (millis() - lastLogTime >= logInterval);

  if (stateChanged && timeElapsed) {
    Blynk.virtualWrite(V3, String("[") + millis() + " ms] State: " + stateName(state) + " | Before LDR: " + beforeValue + " | After LDR: " + afterValue + " | Threshold: " + threshold); 
    Serial.print("Sent to Blynk: ");
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
     Blynk.virtualWrite(V2, stateName(state));
    lastLogTime = millis();

  }
}


void clear(STATE *state) {
  digitalWrite(greenLight, LOW);
  digitalWrite(redLight, LOW);
  myServo.write(180);
  *state = STATE::OPEN;
}

void openGateAuto(STATE *state) {
  int lightValue = analogRead(beforeLdrPin);
  if (lightValue <= threshold) {
    digitalWrite(greenLight, LOW);
    *state = STATE::WARNING;
    return;
  }
  digitalWrite(greenLight, HIGH);
  myServo.write(90);
}

void warnAuto(STATE *state) {
  int lightValue = analogRead(afterLdrPin);
  digitalWrite(redLight, HIGH);
  myServo.write(0);
  if (lightValue <= threshold) {
    *state = STATE::CLOSE;
    return;
  }
}

void closeGate() {
  digitalWrite(greenLight, LOW);
  digitalWrite(redLight, HIGH);
  myServo.write(0);
}

void openGate() {
  digitalWrite(redLight, LOW);
  digitalWrite(greenLight, HIGH);
  myServo.write(90);
}


void closeGateAuto(STATE *state) {
  int lightValue = analogRead(afterLdrPin);
  if (lightValue >= threshold) {
    digitalWrite(redLight, LOW);
    *state = STATE::OPEN;
    return;
  }
  digitalWrite(redLight, HIGH);
  myServo.write(0);
}


void setup() {
  Serial.begin(9600);
  myServo.attach(barrier);
  pinMode(beforeLdrPin, INPUT);
  pinMode(afterLdrPin, INPUT);
  pinMode(greenLight, OUTPUT);
  pinMode(redLight, OUTPUT);
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  
}

void loop() {
  Blynk.run();
  int beforeValue = analogRead(beforeLdrPin);
  int afterValue  = analogRead(afterLdrPin);

  
  if (automatic == 1) {
    switch (status) {
      case STATE::OPEN:
        openGateAuto(statusPtr);
        break;
      case STATE::WARNING:
        warnAuto(statusPtr);
        break;
      case STATE::CLOSE:
        closeGateAuto(statusPtr);
        break;
      }
  }
  logStatus(status, beforeValue, afterValue);
  delay(50);
  

}
