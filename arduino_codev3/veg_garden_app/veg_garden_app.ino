#include "LoRaCoAP.h"
#include <SPI.h>
#include <stdint.h>
#include "DHT.h"  // Download the library here https://learn.adafruit.com/dht/using-a-dhtxx-sensor
#include "Wire.h"

LoraShield lora;
CoAPServer CS;

#define DHTPIN 2     
#define DHTTYPE DHT22
#define SENSOR_ERROR -1000

int LED = 7;
int photocellPin = 0; // the cell and 10K pulldown are connected to a0

DHT dht(DHTPIN, DHTTYPE);

float temperature;
float luminosity;
float humidity;

///////////////////////
// setup() and loop()
///////////////////////

void setup()
{
  Serial.begin(9600);
  pinMode(LED, OUTPUT);
  CS.begin(lora, "beta.bst.t.eu.org"); 
  CS.addRes(String("/sensors"), answer_get_sensors);
  CS.addRes(String("/ping"), answer_get_ping);
  CS.addRes(String("/ledOff"), answer_get_ledOff);
  CS.addRes(String("/ledOn"), answer_get_ledOn);
}

void loop()
{
  temperature = getTemperature();
  luminosity = getLuminosity();
  humidity = getHumidity();
  CS.incoming();
  delay(1000);
}

///////////////////////
// CoAP handlers
///////////////////////
String answer_get_sensors(CoAPResource* res, uint8_t format)
{
  Serial.println("Received a request for /sensors");
  Serial.println("Sensors values are:");
  Serial.print("tmp: ");
  Serial.println(String(temperature));
  Serial.print("lum: ");
  Serial.println(String(luminosity));
  Serial.print("hum: ");
  Serial.println(String(humidity));
  //return "po";
  //return  "{\"temp\":\"" + String(temperature)+"\",\"lum\":\""+String(luminosity)+"\",\"hum\":\""+String(humidity)+"\"}";
  return  "{temp:" + String(temperature)+",lum:"+String(luminosity)+",hum:"+String(humidity)+"}";
 }

String answer_get_ping(CoAPResource* res, uint8_t format)
{
  Serial.println("Received a request for /ping");
  Serial.println("pong");
  return "pong";
}

String answer_get_ledOn(CoAPResource* res, uint8_t format)
{
  digitalWrite(LED,HIGH);
  return "led on !";
}

String answer_get_ledOff(CoAPResource* res, uint8_t format)
{
  digitalWrite(LED,LOW);
  return "led off !";
}

///////////////////////
// Sensors
///////////////////////
float getTemperature()
{
  float tmpTemp = dht.readTemperature();
  if (isnan(tmpTemp))
  {
    return SENSOR_ERROR;
    Serial.println(-1);
  }
  Serial.print("Temperature: ");
  Serial.println(tmpTemp);
  return tmpTemp;
}

float getHumidity()
{
  float tmpHum = dht.readHumidity();
  if (isnan(tmpHum))
  {
    return SENSOR_ERROR;
    Serial.println(-1);
  }
  Serial.print("Humidity: ");
  Serial.println(tmpHum);
  return tmpHum;
}

float getLuminosity()
{
  int photocellReading = analogRead(photocellPin);
  Serial.print("Analog reading = ");
  Serial.print(photocellReading); // the raw analog reading
  // We'll have a few threshholds, qualitatively determined
  if (photocellReading < 10) {
    Serial.println(" - Noir");
  } else if (photocellReading < 200) {
    Serial.println(" - Sombre");
  } else if (photocellReading < 500) {
    Serial.println(" - Lumiere");
  } else if (photocellReading < 800) {
    Serial.println(" - Lumineux");
  } else {
    Serial.println(" - Tres lumineux");
  }
  return photocellReading;
}


