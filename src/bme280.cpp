#include "bme280.h"
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#define SEALEVELPRESSURE_HPA (1013.25)

Adafruit_BME280 bme1; // I2C

float x1;

void BmeSetup1()
{
    //Serial.println(F("BME280 setup"));

    if (!bme1.begin(0x76, &Wire))
    {
        //Serial.println("Could not find a valid BME280 sensor, check wiring!");
        //while (1);
    }

    // weather monitoring
    bme1.setSampling(Adafruit_BME280::MODE_FORCED,
                    Adafruit_BME280::SAMPLING_X1, // temperature
                    Adafruit_BME280::SAMPLING_X1, // pressure
                    Adafruit_BME280::SAMPLING_X1, // humidity
                    Adafruit_BME280::FILTER_X2);
}

void BmeLoop1()
{
    // Only needed in forced mode! In normal mode, you can remove the next line.
    bme1.takeForcedMeasurement(); // has no effect in normal mode

    x1 = bme1.readTemperature() * 10;
    if (x1 > 0)
        bmeTemp1= int(x1);
    x1 = bme1.readHumidity() * 10;
    if (x1 > 0)
        bmeHum1 = int(x1);
    x1 = bme1.readPressure() / 13.3F;
    if (x1 > 0)
        bmePress1 = int(x1);

    //BmePrintValues();
}

void BmePrintValues1()
{
    Serial.print("Temperature = ");
    Serial.print(bme1.readTemperature());
    Serial.println(" *C");

    Serial.print("Pressure = ");

    Serial.print(bme1.readPressure() / 133.0F);
    Serial.println(" mm");

    Serial.print("Approx. Altitude = ");
    Serial.print(bme1.readAltitude(SEALEVELPRESSURE_HPA));
    Serial.println(" m");

    Serial.print("Humidity = ");
    Serial.print(bme1.readHumidity());
    Serial.println(" %");

    Serial.println();
}