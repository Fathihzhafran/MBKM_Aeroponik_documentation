#include <ModbusMaster.h>
#include <ArduinoJson.h>

ModbusMaster nodes[6];
const int sensorIDs[6] = {7, 8, 9, 10, 11, 12};

int trig[2];

int OUT_1, OUT_2, OUT_3, OUT_4;
int IN_1, IN_2;
-
uint16_t holdingRegisterOutput[2];
uint16_t holdingRegisterInput[2];
uint16_t holdingRegister[20];

StaticJsonDocument<512> jsonDoc; // Adjust the size according to your needs

void preTransmission() {
  digitalWrite(DM, HIGH);
}

void postTransmission() {
  digitalWrite(DM, LOW);
}

void setup() {
  pinMode(DM, OUTPUT);
  digitalWrite(DM, LOW);

  Serial.begin(9600);
  Serial1.begin(9600);

  for (int i = 0; i < 6; i++) {
    nodes[i].begin(sensorIDs[i], Serial1);
    nodes[i].preTransmission(preTransmission);
    nodes[i].postTransmission(postTransmission);
  }

  OUT_1 = OUT_2 = OUT_3 = OUT_4 = 0;
  IN_1 = IN_2 = 0;

  pinMode(R1, OUTPUT);
  pinMode(R2, OUTPUT);
  pinMode(R3, OUTPUT);
  pinMode(R4, OUTPUT);
  pinMode(S1, INPUT);
  pinMode(S2, INPUT);
}

void updateTriggers() {
  trig[0] = digitalRead(S1) == LOW;
  trig[1] = digitalRead(S2) == LOW;

  holdingRegisterInput[0] = trig[0];
  holdingRegisterInput[1] = trig[1];

  holdingRegister[2] = holdingRegisterInput[0];
  holdingRegister[3] = holdingRegisterInput[1];
}

void readSendDatasensor() {
  jsonDoc.clear(); // Clear previous data
  JsonArray dataArray = jsonDoc.to<JsonArray>(); // Create a new JSON array

  for (int i = 0; i < 6; i++) {
    uint8_t result;

    if (sensorIDs[i] == 1 || sensorIDs[i] == 2) {
      result = nodes[i].readInputRegisters(0x0000, sensorIDs[i]);
      if (result == nodes[i].ku8MBSuccess) {
        float tempValue = float(nodes[i].getResponseBuffer(1) / 10.0F);
        float humidValue = float(nodes[i].getResponseBuffer(0) / 10.0F);
        
        if (tempValue == 0.0) {
          dataArray.add(0); // If temperature value is 0, add 0 to the array
        } else {
          dataArray.add(tempValue);
        }
        
        if (humidValue == 0.0) {
          dataArray.add(0); // If humidity value is 0, add 0 to the array
        } else {
          dataArray.add(humidValue);
        }
      }
    }

    updateTriggers(); // Update triggers dynamically during sensor read
    delay(50); // Adjust delay time between requests to different sensors if needed
  
  }

  // Add trigger values to the dataArray
  dataArray.add(trig[0]);
  dataArray.add(trig[1]);

  serializeJson(dataArray, Serial);
  Serial.println(); // New line for better readability
}

void loop() {
  IN_1 = digitalRead(S1) == LOW;
  IN_2 = digitalRead(S2) == LOW;

  trig[0] = IN_1;
  trig[1] = IN_2;


  holdingRegisterOutput[0] = OUT_1;
  holdingRegisterOutput[1] = OUT_2;
  holdingRegisterInput[0] = IN_1;
  holdingRegisterInput[1] = IN_2;

  holdingRegister[0] = holdingRegisterOutput[0];
  holdingRegister[1] = holdingRegisterOutput[1];
  holdingRegister[2] = holdingRegisterInput[0];
  holdingRegister[3] = holdingRegisterInput[1];

  digitalWrite(R1, holdingRegister[0]);
  digitalWrite(R2, holdingRegister[1]);

  updateTriggers();

  delay(50); // Delay to approximate the original intervalModbus time (50ms)
}
