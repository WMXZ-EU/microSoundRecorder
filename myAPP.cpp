/* Audio Logger for Teensy 3.6
 * Copyright (c) 2018, Walter Zimmer
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice, development funding notice, and this permission
 * notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
 
 /*
 * Environmental micro Sound Recorder
 * using Bill Greimans's SdFs on Teensy 3.6 
 * which must be downloaded from https://github.com/greiman/SdFs 
 * and installed as local library
 * 
 * uses PJRC's Teensy Audio library for acquisition queuing 
 * audio tool ADC is modified to sample at other than 44.1 kHz
 * can use Stereo ADC (i.e. both ADCs of a Teensy)
 * single ADC may operate in differential mode
 * 
 * audio tool I2S is modified to sample at other than 44.1 kHz
 * can use quad I2S
 *
 * V1 (27-Feb-2018): original version
 *          Feature: allow scheduled acquisition with hibernate during off times
 *          Feature: allow audio triggered acquisition
 * 
 * DD4WH 2018_05_22
 * added MONO I2C32 bit mode
 * added file write for environmental logger
 * added support for BME280 sensor --> library by bolderflight, thank you!
 * https://github.com/bolderflight/BME280
 * added support for BH1750 light sensor --> library by claws, thank you! 
 * https://github.com/claws/BH1750
 *
 * WMXZ 09-Jun-2018
 * Added support to Tympan audio board  ( https://tympan.org )
 * requires tympan audio library (https://github.com/Tympan/Tympan_Library)
 * compile with Serial
 * 
 */
#include "core_pins.h" // this call also kinetis.h

// edits are to be done in the following config.h file
#include "config.h"

//==================== Tympan audio board interface ========================================
#if ACQ == _I2S_32_TYMPAN
  #include "src/Tympan.h"
  #include "src/control_tlv320aic3206.h"
  TympanPins    tympPins(TYMPAN_REVISION);        //TYMPAN_REV_C or TYMPAN_REV_D
  TympanBase    audioHardware(tympPins);
#endif

//==================== Environmental sensors ========================================

#if USE_ENVIRONMENTAL_SENSORS==1
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
#endif

//==================== Audio interface ========================================
/*
 * standard Audio Interface
 * to avoid loading stock SD library
 * NO Audio.h is called but required header files are called directly
 * the custom multiplex object expects the 'link' to queue update function
 * 
 * PJRC's record_queue is modified to allow variable queue size
 * use if different data type requires modification to AudioStream
 * type "int16_t" is compatible with stock AudioStream
 */

#if (ACQ == _ADC_0) || (ACQ == _ADC_D)
  #include "input_adc.h"
  AudioInputAnalog    acq(ADC_PIN);
  #include "m_queue.h"
  mRecordQueue<int16_t, MQUEU> queue1;
  
  AudioConnection     patchCord1(acq, queue1);

#elif ACQ == _ADC_S
  #include "input_adcs.h"
  AudioInputAnalogStereo  acq(ADC_PIN1,ADC_PIN2);
	#include "m_queue.h"
	mRecordQueue<int16_t, MQUEU> queue1;
	#include "audio_multiplex.h"
  static void myUpdate(void) { queue1.update(); }
  AudioStereoMultiplex    mux1((Fxn_t)myUpdate);
  
  AudioConnection     patchCord1(acq,0, mux1,0);
  AudioConnection     patchCord2(acq,1, mux1,1);
  AudioConnection     patchCord3(mux1, queue1);

#elif (ACQ == _I2S) || (ACQ == _I2S_TYMPAN)
  #include "input_i2s.h"
  AudioInputI2S         acq;
	#include "m_queue.h"
	mRecordQueue<int16_t, MQUEU> queue1;
	#include "audio_multiplex.h"
  static void myUpdate(void) { queue1.update(); }
  AudioStereoMultiplex  mux1((Fxn_t)myUpdate);
  
  AudioConnection     patchCord1(acq,0, mux1,0);
  AudioConnection     patchCord2(acq,1, mux1,1);
  AudioConnection     patchCord3(mux1, queue1);

#elif ACQ == _I2S_32  
  #include "i2s_32.h"
  I2S_32         acq;

  #include "m_queue.h"
  mRecordQueue<int16_t, MQUEU> queue1;
  
  #include "audio_multiplex.h"
  static void myUpdate(void) { queue1.update(); }
  AudioStereoMultiplex  mux1((Fxn_t)myUpdate);

  #if MDEL>=0
    #include "m_delay.h"
    mDelay<2,(MDEL+2)>  delay1(0); // have ten buffers more in queue only to be safe
  #endif
  
  #include "mProcess.h"
  mProcess process1(&snipParameters);

  #if MDEL <0
    AudioConnection     patchCord1(acq,0, mux1,0);
    AudioConnection     patchCord2(acq,1, mux1,1);
    
  #else
    AudioConnection     patchCord1(acq,0, process1,0);
    AudioConnection     patchCord2(acq,1, process1,1);
    //
    AudioConnection     patchCord3(acq,0, delay1,0);
    AudioConnection     patchCord4(acq,1, delay1,1);
    AudioConnection     patchCord5(delay1,0, mux1,0);
    AudioConnection     patchCord6(delay1,1, mux1,1);
  #endif
  AudioConnection     patchCord7(mux1, queue1);
  
#elif ACQ == _I2S_QUAD
  #include "input_i2s_quad.h"
  AudioInputI2SQuad     acq;
  #include "m_queue.h"
  mRecordQueue<int16_t, MQUEU> queue1;
  #include "audio_multiplex.h"
  static void myUpdate(void) { queue1.update(); }
  AudioQuadMultiplex    mux1((Fxn_t)myUpdate);
  
  AudioConnection     patchCord1(acq,0, mux1,0);
  AudioConnection     patchCord2(acq,1, mux1,1);
  AudioConnection     patchCord3(acq,2, mux1,2);
  AudioConnection     patchCord4(acq,3, mux1,3);
  AudioConnection     patchCord5(mux1, queue1);

#elif ACQ == _I2S_32_MONO
  #include "i2s_32.h"
  I2S_32         acq;

  #include "m_queue.h"
  mRecordQueue<int16_t, MQUEU> queue1;
 
  #if MDEL>=0
    #include "m_delay.h"
    mDelay<1,(MDEL+2)>  delay1(0); // have ten buffers more in queue only to be safe
  #endif
  
  #include "mProcess.h"
  mProcess process1(&snipParameters);

  #if MDEL <0
    AudioConnection     patchCord1(acq, queue1);
  #else
    AudioConnection     patchCord1(acq, process1);
    AudioConnection     patchCord3(acq, delay1);
    AudioConnection     patchCord5(delay1, queue1);
  #endif
  
#else
  #error "invalid acquisition device"
#endif

// private 'libraries' included directly into sketch
#include "audio_mods.h"
#include "audio_logger_if.h"
#include "audio_hibernate.h"
#include "m_menu.h"

//---------------------------------- some utilities ------------------------------------

// led only allowed if NO I2S
void ledOn(void)
{
  #if (ACQ == _ADC_0) || (ACQ == _ADC_D) || (ACQ == _ADC_S)
    pinMode(13,OUTPUT);
    digitalWriteFast(13,HIGH);
  #endif
}
void ledOff(void)
{
  #if (ACQ == _ADC_0) || (ACQ == _ADC_D) || (ACQ == _ADC_S)
    digitalWriteFast(13,LOW);
  #endif
}

// following three lines are for adjusting RTC 
extern unsigned long rtc_get(void);
extern void *__rtc_localtime; // Arduino build process sets this
extern void rtc_set(unsigned long t);

//__________________________General Arduino Routines_____________________________________

extern "C" void setup() {
  int16_t nsec;

/*
// this reads the Teensy internal temperature sensor
  analogReadResolution(16);
  humidity = (float)analogRead(70);
  //temperature = -0.0095 * humidity + 132.0;
  // for 10bit resolution
  //temperature = -1.8626 * analogRead(70) + 434.5;
  // for 16bit resolution
  temperature = -0.0293 * analogRead(70) + 440.5;
*/

#if USE_ENVIRONMENTAL_SENSORS==1
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
#endif
  
  //
  // put your setup code here, to run once:
  pinMode(3,INPUT_PULLUP); // needed to enter menu if grounded

#define MAUDIO (MQUEU+MDEL+50)
	AudioMemory (MAUDIO); // 600 blocks use about 200 kB (requires Teensy 3.6)

  // stop I2S early (to be sure)
  I2S_stop();
  
//  ledOn();
//  while(!Serial);
//  while(!Serial && (millis()<3000));
//  ledOff();
//
//  check if RTC clock is about the compile time clock
//  uint32_t t0=rtc_get();
//  uint32_t t1=(uint32_t)&__rtc_localtime;
//  if((t1-t0)>100) rtc_set(t1);

  //
  uSD.init();

  // always load config first
  uSD.loadConfig((uint32_t *)&acqParameters, 8, (int32_t *)&snipParameters, 8);

#if USE_ENVIRONMENTAL_SENSORS==1
  // write temperature, pressure and humidity to SD card
   uSD.writeTemperature(temperature, pressure, humidity, lux);
#endif

  // if pin3 is connected to GND enter menu mode
  int ret;
  if(!digitalReadFast(3))
  { ret=doMenu();
    // should here save parameters to disk if modified
    uSD.storeConfig((uint32_t *)&acqParameters, 8, (int32_t *)&snipParameters, 8);
  }
  //
  // check if it is our time to record
  nsec=checkDutyCycle(&acqParameters, -1);
//  if(nsec>0) setWakeupCallandSleep(nsec); // will not return if we should not continue with acquisition 
  if(nsec>0)
  { 
//    delay(1000);
    setWakeupCallandSleep(nsec); // will not return if we should not continue with acquisition 
  }

  // Now modify objects from audio library
  #if (ACQ == _ADC_0) | (ACQ == _ADC_D) | (ACQ == _ADC_S)
    ADC_modification(F_SAMP,DIFF);
  #elif ((ACQ == _I2S))
    I2S_modification(F_SAMP,32);
  #elif (ACQ == _I2S_QUAD)
    I2S_modification(F_SAMP,16); // I2S_Quad not modified for 32 bit
  #endif
  //
  #if(ACQ == _I2S_32 || ACQ == _I2S_32_MONO)
    I2S_modification(F_SAMP,32);
    // shift I2S data right by 8 bits to move 24 bit ADC data to LSB 
    // the lower 16 bit are always maintained for further processing
    // typical shift value is between 8 and 12 as lower ADC bits are only noise
    int16_t nbits=12; 
    acq.digitalShift(nbits); 
  #endif

  #if(ACQ == _I2S_TYMPAN)
    // initalize typan's tlv320aic3206
    //Enable the Tympan to start the audio flowing!
    audioHardware.enable(); // activate AIC
    //enable the Tympman to detect whether something was plugged inot the pink mic jack
    audioHardware.enableMicDetect(true);
    
    //Choose the desired audio input on the Typman...this will be overridden by the serviceMicDetect() in loop() 
    audioHardware.inputSelect(TYMPAN_INPUT_DEVICE);
  
    //Set the desired input gain level
    audioHardware.setInputGain_dB(input_gain_dB); // set input volume, 0-47.5dB in 0.5dB setps
    
    //Set the state of the LEDs
    audioHardware.setRedLED(HIGH);
    audioHardware.setAmberLED(LOW);
    
  #endif
  
  //are we using the eventTrigger?
  if(snipParameters.thresh>=0) mustClose=0; else mustClose=-1;
  #if MDEL>=0
    if(mustClose<0) delay1.setDelay(0); else delay1.setDelay(MDEL);
  #endif
  
  // set filename prefix
  uSD.setPrefix(acqParameters.name);
  // lets start
  process1.begin(&snipParameters); 
  queue1.begin();
  //
  Serial.println("End of Setup");
}

#define mDebug(X) { static uint32_t t0; if(millis()>t0+1000) {Serial.println(X); t0=millis();}}

volatile uint32_t maxValue=0, maxNoise=0; // possibly be updated outside

extern "C" void loop() {
  // put your main code here, to run repeatedly:
  int16_t nsec;
  uint32_t to=0,t1,t2;
  static uint32_t t3,t4;
  static int16_t state=0; // 0: open new file, -1: last file
  if(queue1.available())
  { // have data on queue
    nsec=checkDutyCycle(&acqParameters, state);
    if(nsec<0) { uSD.setClosing();} // this will be last record in file
    if(nsec>0) { setWakeupCallandSleep(nsec);} // file closed sleep now
    //
    if(state==0)
    { // generate header before file is opened
    #ifndef GEN_WAV_FILE // is declared in audio_logger_if.h
       uint32_t *header=(uint32_t *) headerUpdate();
       uint32_t *ptr=(uint32_t *) outptr;
       // copy to disk buffer
       for(int ii=0;ii<128;ii++) ptr[ii] = header[ii];
       outptr+=256; //(512 bytes)
    #endif
       state=1;
    }
    // fetch data from queue
    int32_t * data = (int32_t *)queue1.readBuffer();
    //
    // copy to disk buffer
    uint32_t *ptr=(uint32_t *) outptr;
    for(int ii=0;ii<64;ii++) ptr[ii] = data[ii];
    // release buffer an queue
    queue1.freeBuffer(); 
    //
    // adjust buffer pointer
    outptr+=128; // (128 shorts)
    //
    // if necessary reset buffer pointer and write to disk
    // buffersize should be always a multiple of 512 bytes
    if(outptr == (diskBuffer+BUFFERSIZE))
    {
      outptr = diskBuffer;
  
      // write to disk ( this handles also opening of files)
      // but only if we have detection
      if((state>=0) && ((snipParameters.thresh<0) || (process1.getSigCount()>0)))
      {
//        Serial.print(".");
        to=micros();
        state=uSD.write(diskBuffer,BUFFERSIZE); // this is blocking
        t1=micros();
        t2=t1-to;
        if(t2<t3) t3=t2;
        if(t2>t4) t4=t2;
      }
    }
    if(!state)
    { Serial.println("closed");
      uSD.storeConfig((uint32_t *)&acqParameters, 8, (int32_t *)&snipParameters, 8);
    }
  }
  else
  {  // queue is empty
  // are we told to close or running out of time?
    // if delay is eneabled must wait for delay to pass by
    if(((mustClose>0)&& (process1.getSigCount()< -MDEL))
       || ((mustClose==0) && (checkDutyCycle(&acqParameters, state)<0)))
    { 
      // write remaining data to disk and close file
      if(state>=0)
      { uint32_t nbuf = (uint32_t)(outptr-diskBuffer);
//        Serial.println(nbuf);
        state=uSD.write(diskBuffer,nbuf); // this is blocking
        state=uSD.close();
        uSD.storeConfig((uint32_t *)&acqParameters, 8, (int32_t *)&snipParameters, 8);
      }
      outptr = diskBuffer;

      // reset mustClose flag
      if(snipParameters.thresh>=0) mustClose=0; else mustClose=-1;
      Serial.println("file closed");
    }
  }

  // some statistics on progress
  static uint32_t loopCount=0;
  static uint32_t t0=0;
  loopCount++;
  if(millis()>t0+1000)
  {  Serial.printf("\tloop: %5d %4d; %5d %5d; %5d; ",
          loopCount, uSD.getNbuf(), t3,t4, 
          AudioMemoryUsageMax());
    AudioMemoryUsageMaxReset();
    t3=1<<31;
    t4=0;
  
  #if MDEL>=0
     Serial.printf("%4d; %10d %10d %4d; %4d %4d %4d; ",
            queue1.dropCount, 
            maxValue, maxNoise, maxValue/maxNoise,
            process1.getSigCount(),process1.getDetCount(),process1.getNoiseCount());
            
      queue1.dropCount=0;
      process1.resetDetCount();
      process1.resetNoiseCount();
  #endif

  #if (ACQ==_ADC_0) | (ACQ==_ADC_D) | (ACQ==_ADC_S)
    Serial.printf("%5d %5d",PDB0_CNT, PDB0_MOD);
  #endif
  
    Serial.println();
    t0=millis();
    loopCount=0;
    maxValue=0;
    maxNoise=0;
 }
  asm("wfi"); // to save some power switch off idle cpu

}

