#ifndef _ENVIRO_H
#define ENVIRO_H
  // test: write environmental variables into a text file
  // will be substituted with real sensor logging
    float temperature = 20.4;
    float pressure = 1014.1;
    float humidity = 90.3;
    uint16_t lux = 99;
  
  // temperature sensor /////////////////////////////////////
  #include "BME280.h"
  
  // connected to I2C
  // pin 18 - SDA
  // pin 19 - SCL 
  /* A BME280 object with I2C address 0x76 (SDO to GND) */
  /* on Teensy I2C bus 0 */
  BME280 bme(Wire,0x76);
  ////////////////////////////////////////////////////////////
  
  // Light sensor /////////////////////////////////////
  #include <BH1750.h>
  
  BH1750 lightMeter;
  // connected to I2C, adress is 0x23 [if ADDR pin is NC or GND]
  // pin 18 - SDA
  // pin 19 - SCL
  
  void  enviro_setup(void)
  {
    // begin communication with temperature/humidity sensor
    // BME280 and set to default
    // sampling, iirc, and standby settings
    if (bme.begin() < 0) {
      Serial.println("Error communicating with sensor, check wiring and I2C address");
  //    while(1){}
    }
  
    // adjust settings for temperature/humidity/pressure sensor
    bme.setIirCoefficient(BME280::IIRC_OFF); // switch OFF IIR filter of sensor
    bme.setForcedMode(); // set forced mode --> for long periods between measurements
    // read the sensor
    bme.readSensor();
  
    temperature = bme.getTemperature_C();
    pressure = bme.getPressure_Pa()/100.0f;
    humidity = bme.getHumidity_RH();
  
    lightMeter.begin(BH1750::ONE_TIME_HIGH_RES_MODE);
  
    lux = lightMeter.readLightLevel();
  /*  // displaying the data
    Serial.print(bme.getPressure_Pa()/100.0f,1);
    Serial.print("\t");
    Serial.print(bme.getTemperature_C(),2);
    Serial.print("\t");
    Serial.println(bme.getHumidity_RH(),2);
  */  
  }
#endif
