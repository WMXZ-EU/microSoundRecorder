/* Sound Recorder for Teensy 3.6
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
 * Added support for Tympan audio board  ( https://tympan.org )
 * requires tympan audio library (https://github.com/Tympan/Tympan_Library)
 * compile with Serial
 * 
 * WMXZ 23-Jun-2018
 * Added support for TDM I2S interface
 * 
 * WMXZ 01-Jul-2018
 * aligned acoustic interfaces
 * moved enviromental interface to own include file
 */
#include "core_pins.h" // this call also kinetis.h

// edits are to be done in the following config.h file
#include "config.h"

#if defined(__MK20DX256__)
  #define MAX_Q 100 // number of buffers in aquisition queue
#elif defined(__MK64FX512__)
  #define MAX_Q 250 // number of buffers in aquisition queue
#elif defined(__MK66FX1M0__)
  #define MAX_Q 550 // number of buffers in aquisition queue
#else
  #define MAX_Q 53 // number of buffers in aquisition queue
#endif

//==================== Tympan audio board interface ========================================
#if ACQ == _I2S_TYMPAN
  #include "src/Tympan.h"
  #include "src/control_tlv320aic3206.h"
  TympanPins    tympPins(TYMPAN_REVISION);        //TYMPAN_REV_C or TYMPAN_REV_D
  TympanBase    audioHardware(tympPins);
#endif

//==================== Environmental sensors ========================================
#if USE_ENVIRONMENTAL_SENSORS==1
  #include "enviro.h"
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
/*-------------------------- mono (single channel) -----------------------------*/
#if (ACQ == _ADC_0) || (ACQ == _ADC_D) || (ACQ == _I2S_32_MONO)
  #define NCH 1


  #if (ACQ == _ADC_0) || (ACQ == _ADC_D)
    #include "input_adc.h"
    AudioInputAnalog    acq(ADC_PIN);
  #elif (ACQ == _I2S_32_MONO)
    #include "i2s_32.h"
    I2S_32         acq;
  #endif

  #define MQ (MAX_Q/NCH)
  #include "m_queue.h"
  mRecordQueue<MQ> queue[NCH];
  
  #if MDEL>=0 
    #include "m_delay.h" 
    mDelay<NCH,(MDEL+2)>  delay1(0); // have two buffers more in queue only to be safe 
  #endif 
    
  #include "mProcess.h" 
  mProcess process1(&snipParameters); 
 
  AudioConnection     patchCord1(acq, process1); 
  #if MDEL <0 
    AudioConnection     patchCord2(acq, queue[0]); 
  #else 
    AudioConnection     patchCord2(acq, delay1); 
    AudioConnection     patchCord3(delay1, queue[0]); 
  #endif 

/*-------------------------- stereo (dual channel) -----------------------------*/
#elif (ACQ == _ADC_S) || (ACQ == _I2S) || (ACQ == _I2S_32) || (ACQ == _I2S_TYMPAN)
  #define NCH 2

  #if (ACQ == _ADC_S)
    #include "input_adcs.h"
    AudioInputAnalogStereo  acq(ADC_PIN1,ADC_PIN2);

  #elif (ACQ == _I2S) || (ACQ == _I2S_TYMPAN)
    #include "input_i2s.h"
    AudioInputI2S         acq;

  #elif (ACQ == _I2S_32)
    #include "i2s_32.h"
    I2S_32         acq;
  #endif

  #define MQ (MAX_Q/NCH)
  #include "m_queue.h"
  mRecordQueue<MQ> queue[NCH];

  #if MDEL>=0 
    #include "m_delay.h" 
    mDelay<NCH,(MDEL+2)>  delay1(2); // have two buffers more in queue only to be safe 
  #endif 

  #include "mProcess.h"
  mProcess process1(&snipParameters);

  AudioConnection     patchCord1(acq,0, process1,0);
  AudioConnection     patchCord2(acq,1, process1,1);
    //
  #if MDEL <0
    AudioConnection     patchCord3(acq,0, queue[0],0);
    AudioConnection     patchCord4(acq,1, queue[1],0);
    
  #else
    AudioConnection     patchCord3(acq,0, delay1,0);
    AudioConnection     patchCord4(acq,1, delay1,1);
    AudioConnection     patchCord5(delay1,0, queue[0],0);
    AudioConnection     patchCord6(delay1,1, queue[1],0);
  #endif

/*-------------------------- (quad channel) -----------------------------*/
#elif ACQ == _I2S_QUAD      // not yet modified for event detections and delays
  #define NCH 4
  
  #include "input_i2s_quad.h"
  AudioInputI2SQuad     acq;
  
  #define MQ (MAX_Q/NCH)
  #include "m_queue.h"
  mRecordQueue<MQ> *queue = new mRecordQueue<MQ> [NCH];

  AudioConnection     patchCord1(acq,0, queue[0],0);
  AudioConnection     patchCord2(acq,1, queue[1],0);
  AudioConnection     patchCord3(acq,2, queue[2],0);
  AudioConnection     patchCord4(acq,3, queue[3],0);

  
/*-------------------------- (multi channel TDM) -----------------------------*/
#elif ACQ == _I2S_TDM       // not yet modified for event detections and delays

  #define NCH 5 // if changing number of channels adapt Audio connections  // NCH must be less or equal than 8
  
  #include "i2s_tdm.h"
  I2S_TDM         acq;
  
  #define MQ (MAX_Q/NCH)
  #include "m_queue.h"
  mRecordQueue<MQ> queue[NCH];

  #if MDEL >=0
    #undef MDEL
    #define MDEL -1
  #endif
  
  AudioConnection     patchCord0(acq,0,queue[0],0);
  AudioConnection     patchCord1(acq,1,queue[1],0);
  AudioConnection     patchCord2(acq,2,queue[2],0);
  AudioConnection     patchCord3(acq,3,queue[3],0);
  AudioConnection     patchCord4(acq,4,queue[4],0);
  //
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
//extern unsigned long rtc_get(void);
//extern void *__rtc_localtime; // Arduino build process sets this
//extern void rtc_set(unsigned long t);

time_t getTeensy3Time(){  return Teensy3Clock.get();}
//__________________________General Arduino Routines_____________________________________
int started=0;
extern "C" void setup() {
  // put your setup code here, to run once:
  int32_t nsec;
  pinMode(3,INPUT_PULLUP); // needed to enter menu if grounded

  // set the Time library to use Teensy 3.0's RTC to keep time
  setSyncProvider(getTeensy3Time);

#if DO_DEBUG>0

   while(!Serial && !digitalRead(3));
//  while(!Serial && (millis()<3000)); // use this for testing without menu
   Serial.println("microSoundRecorder");
#endif
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

#define MAUDIO (MAX_Q+MDEL+50)
	AudioMemory (MAUDIO); // 600 blocks use about 200 kB (requires Teensy 3.6)

  // stop I2S early (to be sure)
  #if ((ACQ == _I2S) || (ACQ == _I2S_QUAD) || (ACQ == _I2S_32) || (ACQ == _I2S_32_MONO) || (ACQ == _I2S_TYMPAN) || (ACQ == _I2S_TDM))
    I2S_stop();
  #endif

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
   enviro_setup();
  // write temperature, pressure and humidity to SD card
   uSD.writeTemperature(temperature, pressure, humidity, lux);
#endif

  // if pin3 is connected to GND enter menu mode
  int ret;
  if(!digitalReadFast(3))
  { ret=doMenu();
    if(ret<0) ;  // should shutdown now (not implemented) // keep compiler happy
      
    // should here save parameters to disk if modified
    uSD.storeConfig((uint32_t *)&acqParameters, 8, (int32_t *)&snipParameters, 8);
  }
  //
  // check if it is our time to record
  nsec=checkDutyCycle(&acqParameters, -1);
  if(nsec>0) 
  { 
    #if ((ACQ == _I2S) || (ACQ == _I2S_QUAD) || (ACQ == _I2S_32) || (ACQ == _I2S_32_MONO) || (ACQ == _I2S_TYMPAN) || (ACQ == _I2S_TDM))
      I2S_stopClock();
    #endif
    setWakeupCallandSleep(nsec); // will not return if we should not continue with acquisition 
  }
  
  // Now modify objects from audio library
  #if (ACQ == _ADC_0) || (ACQ == _ADC_D) || (ACQ == _ADC_S)
    ADC_modification(F_SAMP,DIFF);
  
  #elif ((ACQ == _I2S))
    I2S_modification(F_SAMP,32,2);
  
  #elif (ACQ == _I2S_QUAD)
    I2S_modification(F_SAMP,16,4); // I2S_Quad not modified for 32 bit (therefore 16 bit)
  
  #elif((ACQ == _I2S_32) || (ACQ == _I2S_32_MONO))
    I2S_modification(F_SAMP,32,2);
    // shift I2S data right by 8 bits to move 24 bit ADC data to LSB 
    // the lower 16 bit are always maintained for further processing
    // typical shift value is between 8 and 12 as lower ADC bits are only noise
    int16_t nbits=NSHIFT; 
    acq.digitalShift(nbits); 

  #elif(ACQ == _I2S_TYMPAN)
    // initalize tympan's tlv320aic3206
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

  #elif(ACQ == _I2S_TDM)
    I2S_modification(F_SAMP,32,8);
    int16_t nbits=NSHIFT; 
    acq.digitalShift(nbits); 
  #endif

  //are we using the eventTrigger?
  if(snipParameters.thresh>=0) mustClose=0; else mustClose=-1;
  #if MDEL>=0
    if(mustClose<0) delay1.setDelay(0); else delay1.setDelay(MDEL);
  #endif
  
  // set filename prefix
  uSD.setPrefix(acqParameters.name);
  // lets start
  #if MDEL>=0
    process1.begin(&snipParameters); 
  #endif

  for(int ii=0; ii<NCH; ii++) queue[ii].begin();
  //
  Serial.println("End of Setup");
  started=0;
}

volatile uint32_t maxValue=0, maxNoise=0; // possibly be updated outside
int16_t tempBuffer[AUDIO_BLOCK_SAMPLES*NCH];

extern "C" void loop() {
  // put your main code here, to run repeatedly:
  int32_t nsec;
  uint32_t to=0,t1,t2;
  static uint32_t t3,t4;
  static int16_t state=0; // 0: open new file, -1: last file

  int have_data=1;
  for(int ii=0;ii<NCH;ii++) if(queue[ii].available()==0) have_data=0;

  if(have_data)
  { // have data on queue
    started=1; // flag that we have now data
    nsec=checkDutyCycle(&acqParameters, state);
    if(nsec<0) { uSD.setClosing();} // this will be last record in file
    if(nsec>0) 
    { 
      #if ((ACQ == _I2S) || (ACQ == _I2S_QUAD) || (ACQ == _I2S_32) || (ACQ == _I2S_32_MONO) || (ACQ == _I2S_TYMPAN) || (ACQ == _I2S_TDM))
        I2S_stopClock();
      #endif
      setWakeupCallandSleep(nsec); // file closed sleep now
    }
    //
    if(state==0)
    { // generate header before file is opened
      #ifdef GEN_WAV_FILE // is declared in audio_logger_if.h
         uint32_t *header=(uint32_t *) wavHeader(0); // call initially with zero filesize
         //
         int ndat=outptr-diskBuffer;
         if(ndat>0)
         { // shift exisiting data after header, which is always at beginnig of file
          for(int ii=0; ii<ndat; ii++) diskBuffer[22+ii]=diskBuffer[ii]; 
         }
         // copy header to disk buffer
         uint32_t *ptr=(uint32_t *) diskBuffer;
         for(int ii=0;ii<11;ii++) ptr[ii] = header[ii];
         outptr+=22; //(44 bytes)
      #else
         uint32_t *header=(uint32_t *) headerUpdate(); 
         //
         int ndat=outptr-diskBuffer;
         if(ndat>0)
         { // shift exisiting data after header, which is always at beginnig of file
          for(int ii=0; ii<ndat; ii++) diskBuffer[256+ii]=diskBuffer[ii]; 
         }
         // copy header to disk buffer
         uint32_t *ptr=(uint32_t *) diskBuffer;
         // copy to disk buffer
         for(int ii=0;ii<128;ii++) ptr[ii] = header[ii];
         outptr+=256; //(512 bytes)
      #endif
      state=1;
    }
    
    // fetch data from queues
    int16_t * data[NCH];
    for(int ii=0; ii<NCH; ii++) data[ii] = (int16_t *)queue[ii].readBuffer();
    // multiplex data
    int16_t *tmp = tempBuffer;
    for(int ii=0;ii<AUDIO_BLOCK_SAMPLES;ii++) for(int jj=0; jj<NCH; jj++) *tmp++ = *data[jj]++;
    // release queues
    for(int ii=0; ii<NCH; ii++) queue[ii].freeBuffer();

    // copy data to disk buffer
    int16_t *ptr=(int16_t *) outptr;
    
    // number of data in tempBuffer
    int32_t ndat = AUDIO_BLOCK_SAMPLES*NCH;
    
    // number of free samples on diskbuffer
    int32_t nout = diskBuffer+BUFFERSIZE - outptr;

    tmp = tempBuffer;
    if (nout>ndat)
    { // sufficient space for all data
      for(int ii=0;ii<ndat;ii++) *ptr++ = *tmp++;
      nout-=ndat;
      ndat=0;
    }
    else
    { // fill up disk buffer
      int nbuf=nout;
      if(uSD.isClosing()) nbuf=(nbuf/NCH)*NCH; // is last record of file 
      for(int ii=0;ii<nbuf;ii++) *ptr++ = *tmp++;
      ndat-=nbuf;
      nout=0;
    }
    
    if(nout==0) //buffer is filled, so write to disk
    { int32_t nbuf=ptr-diskBuffer;
    
      to=micros();
      state=uSD.write(diskBuffer,nbuf); // this is blocking
      t1=micros();
      t2=t1-to;
      if(t2<t3) t3=t2; // accumulate some time statistics
      if(t2>t4) t4=t2;

      ptr=(int16_t *)diskBuffer;
    }

    if(ndat>0) // save residual data
    {
      for(int ii=0;ii<ndat;ii++) *ptr++ = *tmp++;
    }
    
    // all data are copied
    outptr=(int16_t *)ptr; // save actual write position
/*
    //
    for(int ii=0;ii<AUDIO_BLOCK_SAMPLES;ii++) 
    { // the following is inefficient but needed for arbitrary NCH (to be improved)
      {
        for(int jj=0; jj<NCH; jj++)
        {  *ptr++ = *data[jj]++;
           uint32_t nbuf=0;
           if((jj==0) && (state==0) && (ptr+NCH > (int16_t *)(diskBuffer+BUFFERSIZE))) nbuf = (uint32_t)(ptr-diskBuffer);
           if(ptr == (int16_t *)(diskBuffer+BUFFERSIZE)) nbuf = BUFFERSIZE;
           if(nbuf>0)
           {
              // flush diskBuffer
              if((state>=0) 
                           && ((snipParameters.thresh<0) 
                                                       #if MDEL >=0
                                                         || (process1.getSigCount()>0)
                                                       #endif
                             ))
              {
                to=micros();
                state=uSD.write(diskBuffer,nbuf); // this is blocking
                t1=micros();
                t2=t1-to;
                if(t2<t3) t3=t2; // accumulate some time statistics
                if(t2>t4) t4=t2;
                ptr=(int16_t *)diskBuffer;
              }
           }
        }
      }
    } // copied now all data
    outptr=(int16_t *)ptr; // save actual write position
*/
    if(!state)
    { 
#if DO_DEBUG>0
      Serial.println("closed");
#endif
      // store config again if you wanted time of latest file stored
      uSD.storeConfig((uint32_t *)&acqParameters, 8, (int32_t *)&snipParameters, 8);
    }
  }
  else if(started)
  { // queue is empty that is we have no data
    // are we told to close or running out of time?
    // if delay is enabled must wait for delay to pass by
    if(
        #if MDEL >=0
          ((mustClose>0) && (process1.getSigCount()< -MDEL)) ||
        #endif
          ((mustClose==0) && ((checkDutyCycle(&acqParameters, state)<0))))
    { 
      Serial.println("QUEUE Empty");
      // write remaining data to disk and close file
      if(state>=0)
      { uint32_t nbuf = (uint32_t)(outptr-diskBuffer);
        state=uSD.write(diskBuffer,nbuf); // this is blocking
        state=uSD.close();
        uSD.storeConfig((uint32_t *)&acqParameters, 8, (int32_t *)&snipParameters, 8);
      }
      outptr = diskBuffer;

      // reset mustClose flag
      if(snipParameters.thresh>=0) mustClose=0; else mustClose=-1;
#if DO_DEBUG>0
      Serial.println("file closed");
#endif
    }
  }

#if DO_DEBUG>0
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
            queue[0].dropCount, 
            maxValue, maxNoise, maxValue/maxNoise,
            process1.getSigCount(),process1.getDetCount(),process1.getNoiseCount());
            
      queue[0].dropCount=0;
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
#endif

  asm("wfi"); // to save some power switch off idle cpu
}
