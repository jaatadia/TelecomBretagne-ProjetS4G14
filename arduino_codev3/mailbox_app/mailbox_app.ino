#include "LoRaCoAP.h"
#include <SPI.h>
#include <stdint.h>
#include "DHT.h"  // Download the library here https://learn.adafruit.com/dht/using-a-dhtxx-sensor
#include "Wire.h"

LoraShield lora;
CoAPServer CS;

#define ANALOG_PIN 3
#define THRESHOLD 880
float distance = 0;

///////////////////////
// setup() and loop()
///////////////////////

void setup()
{
  Serial.begin(9600);
  CS.begin(lora, "alpha.bst.t.eu.org"); 
  CS.addRes(String("/dist"), answer_get_distance);
  CS.addRes(String("/ping"), answer_get_ping);
}

void loop()
{
  distance = getDistance();
  Serial.println(distance);
  CS.incoming();
  delay(3000);
}

///////////////////////
// CoAP handlers
///////////////////////

String answer_get_distance (CoAPResource* res, uint8_t format)
{
      String result;
      Serial.println("Received a request for /dist");
      Serial.print("Sensor value is:");
      Serial.println(distance);
      //return "{\n\"dist\": \"" + String(distance) + "\"\n}";
      if (distance < THRESHOLD)
      {
        result = "1";  // At least a letter found
      }
      else
      {
        result = "0";  // No letter
      }
      return result;
}

String answer_get_ping(CoAPResource* res, uint8_t format)
{
      Serial.println("Received a request for /ping");
      Serial.println("pong");
      return "pong";
}

///////////////////////
// Sensors
///////////////////////
float getDistance()
{
  float tmpDist = analogRead(ANALOG_PIN);
  String result;
  //Serial.println(tmpDist); // prints the value read
  return tmpDist;
}


