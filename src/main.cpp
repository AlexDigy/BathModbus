#include <Arduino.h>
#include "main.h"
#include <ModbusRtu.h>
#include "vcc.h"
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include "Adafruit_SGP30.h"

// assign the Arduino pin that must be connected to RE-DE RS485 transceiver
#define TXEN 4

#define pinPir 9
#define pinLed 5
#define pinPhoto A6

#define ID 111

unsigned long lastUpdateTime;
unsigned int loopCounter;
unsigned int UPDATE_TIME = 100;

unsigned int bmeTemp;
unsigned int bmeHum;
unsigned int bmePress;
unsigned int sgpTvoc;
unsigned int sgpEco2;
unsigned int photo;
unsigned int pir = 0;
unsigned int vcc;

bool ledState = false;

Adafruit_BME280 bme; // I2C
Adafruit_SGP30 sgp;  // I2C

float x;

// data array for modbus network sharing
uint16_t au16data[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0,0};

/**
 *  Modbus object declaration
 *  u8id : node id = 0 for master, = 1..247 for slave
 *  port : serial port
 *  u8txenpin : 0 for RS-232 and USB-FTDI
 *               or any pin number > 1 for RS-485
 */
Modbus slave(ID, Serial, TXEN); // this is slave @1 and RS-485

bool byPirIllumination = false;

/* return absolute humidity [mg/m^3] with approximation formula
 * @param temperature [°C]
 * @param humidity [%RH]
 */
uint32_t getAbsoluteHumidity(float temperature, float humidity)
{
  // approximation formula from Sensirion SGP30 Driver Integration chapter 3.15
  const float absoluteHumidity = 216.7f * ((humidity / 100.0f) * 6.112f * exp((17.62f * temperature) / (243.12f + temperature)) / (273.15f + temperature)); // [g/m^3]
  const uint32_t absoluteHumidityScaled = static_cast<uint32_t>(1000.0f * absoluteHumidity);                                                                // [mg/m^3]
  return absoluteHumidityScaled;
}

void setup()
{
  BmeSetup();
  SgpSetup();

  pinMode(pinLed, OUTPUT);
  pinMode(pinPir, INPUT);

  analogReference(INTERNAL);
  // analogWrite(pinLed, 0);

  Serial.begin(57600, SERIAL_8N2); // baud-rate at 19200
  // Serial.begin(9600);
  slave.start();
}

void loop()
{
  checkSensors();

  slave.poll(au16data, 10);

  // Actualiza los pines de Arduino con la tabla de Modbus
  io_poll();
  // delay(1000);
}

void checkSensors()
{
  if (millis() - lastUpdateTime > UPDATE_TIME)
  {
    loopCounter++;

    BmeLoop();
    SgpLoop();
    vcc = readVcc();
    photo = analogRead(pinPhoto);

    if (pir > 0)
    {
      if (byPirIllumination == false && photo < 70)
      {
        byPirIllumination = true;
        // au16data[7]=255;
        analogWrite(pinLed, 255);
      }
      else
      {
        // au16data[7]=0;
        // analogWrite(pinLed, 0);
      }
    }
    else
    {
      analogWrite(pinLed, 0);
      // au16data[7]=0;
      /*if (byPirIllumination)
    {
      au16data[7]=0;
      //analogWrite(pinLed, 0);
    }*/
      byPirIllumination = false;
    }
  }
}

void io_poll()
{
  // digital inputs -> au16data[0]
  // Lee las entradas digitales y las guarda en bits de la primera variable del vector
  // (es lo mismo que hacer una máscara)

  pir = digitalRead(pinPir);
  bitWrite(au16data[0], 0, pir); // Lee el pin 2 de Arduino y lo guarda en el bit 0 de la variable au16data[0]
  // bitWrite(au16data[0], 8, digitalRead(pinLed));

  if (au16data[7] > 255)
    au16data[7] = 255;

  /*  if (bitRead(au16data[0], 8)==1)
  {
    au16data[7]=255;
  }
  else if (au16data[7]==255)
  {
    bitWrite(au16data[0], 8,1);
  }*/
  // digitalWrite(pinLed, bitRead(au16data[0], 8)); // сделать вкл до 255
  // analogWrite(pinLed, au16data[7]); // сделать защиту от переполнения
  //  digital outputs -> au16data[1]
  //  Lee los bits de la segunda variable y los pone en las salidas digitales
  // digitalWrite(6, bitRead(au16data[1], 0)); //Lee el bit 0 de la variable au16data[1] y lo pone en el pin 6 de Arduino

  // Cambia el valor del PWM
  // analogWrite(10, au16data[2]); //El valor de au16data[2] se escribe en la salida de PWM del pin 10 de Arduino. (siendo 0=0% y 255=100%)

  // Lee las entradas analógicas (ADC)
  au16data[1] = loopCounter;
  au16data[2] = bmeTemp; // El valor analógico leido en el pin A0 se guarda en au16data[4]. (siendo 0=0v y 1023=5v)
  au16data[3] = bmeHum;
  au16data[4] = bmePress;
  au16data[5] = photo;
  au16data[6] = vcc;

  au16data[8] = sgpTvoc;
  au16data[9] = sgpEco2;
}

void BmeSetup()
{
  // Serial.println(F("BME280 setup"));

  if (!bme.begin(0x76, &Wire))
  {
    // Serial.println("Could not find a valid BME280 sensor, check wiring!");
    // while (1);
  }

  // weather monitoring
  bme.setSampling(Adafruit_BME280::MODE_FORCED,
                  Adafruit_BME280::SAMPLING_X1, // temperature
                  Adafruit_BME280::SAMPLING_X1, // pressure
                  Adafruit_BME280::SAMPLING_X1, // humidity
                  Adafruit_BME280::FILTER_X2);
}

void BmeLoop()
{
  // Only needed in forced mode! In normal mode, you can remove the next line.
  bme.takeForcedMeasurement(); // has no effect in normal mode

  x = bme.readTemperature() * 10;
  if (x > 0)
    bmeTemp = int(x);
  x = bme.readHumidity() * 10;
  if (x > 0)
    bmeHum = int(x);
  x = bme.readPressure() / 13.3F;
  if (x > 0)
    bmePress = int(x);
}

void SgpSetup()
{
  //Serial.println("SGP30 Setup");

  if (!sgp.begin())
  {
    //Serial.println("Sensor not found :(");
    //while (1)
    //  ;
  }
  //Serial.print("Found SGP30 serial #");
  //Serial.print(sgp.serialnumber[0], HEX);
  //Serial.print(sgp.serialnumber[1], HEX);
  //Serial.println(sgp.serialnumber[2], HEX);

  // If you have a baseline measurement from before you can assign it to start, to 'self-calibrate'
  // sgp.setIAQBaseline(0x8E68, 0x8F41);  // Will vary for each sensor!
}

void SgpLoop()
{
  sgp.setHumidity(getAbsoluteHumidity(bmeTemp / 10.0F, bmeHum / 10.0F));

  if (!sgp.IAQmeasure())
  {
    //Serial.println("Measurement failed");
    return;
  }

  sgpTvoc = sgp.TVOC;
  sgpEco2 = sgp.eCO2;

  //Serial.print("TVOC ");  Serial.print(sgp.TVOC);  Serial.print(" ppb\t");
  //Serial.print("eCO2 ");  Serial.print(sgp.eCO2);  Serial.println(" ppm");

  if (!sgp.IAQmeasureRaw())
  {
    //Serial.println("Raw Measurement failed");
    return;
  }
  //Serial.print("Raw H2 ");  Serial.print(sgp.rawH2);  Serial.print(" \t");
  //Serial.print("Raw Ethanol ");  Serial.print(sgp.rawEthanol);  Serial.println("");
}