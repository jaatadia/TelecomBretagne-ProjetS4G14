#include "LoRaCoAP.h"
#include <SPI.h>
#include <stdint.h>
#include "DHT.h"  // Download the library here https://learn.adafruit.com/dht/using-a-dhtxx-sensor
#include "Wire.h"

LoraShield lora;
CoAPServer CS;

void setup()
{
  Serial.begin(9600);
  CS.begin(lora, "beta.bst.t.eu.org"); 
  CS.addRes(String("/ping"), answer_get_ping);
}

void loop()
{
  CS.incoming();
}

///////////////////////
// CoAP handlers
///////////////////////

String answer_get_ping(CoAPResource* res, uint8_t format)
{
      Serial.println("Received a request for /ping");
      Serial.println("pong");
      return "pong";
}

