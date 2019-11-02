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

 /*********************** Begin possible User Modifications ********************************/
// possible modifications are marked with //<<<======>>>
//
//----------------------------------------------------------------------------------------
#define DO_DEBUG 1 // print debug info over usb-serial line  //<<<======>>>
#define DO_SERIAL1 1 // print debug info over serial1 line  //<<<======>>>

#define F_SAMP 48000 // desired sampling frequency  //<<<======>>>
/*
 * NOTE: changing frequency impacts the macros 
 *      AudioProcessorUsage and AudioProcessorUsageMax
 * defined in stock AudioStream.h
 */

////////////////////////////////////////////////////////////
// ------------------------- Acquisition interface control ----------------------------
// possible ACQ interfaces
#define _ADC_0          0 // single ended ADC0
#define _ADC_D          1 // differential ADC0
#define _ADC_S          2 // stereo ADC0 and ADC1
#define _I2S            3 // I2S (16 bit stereo audio)
#define _I2S_32         4 // I2S (32 bit stereo audio), eg. two ICS43434 mics
#define _I2S_QUAD       5 // I2S (16 bit quad audio)
#define _I2S_32_MONO    6 // I2S (32 bit mono audio), eg. one ICS43434 mic
#define _I2S_TYMPAN     7 // I2S (16/32 bit tympan stereo audio audio) for use the tympan board
#define _I2S_TDM        8 // I2S (8 channel TDM) // only first 5 channels are used (modify myAPP.cpp for less or more channels)
#define _I2S_CS42448    9 // I2S CS42448 audio board - 6chan input 8-chan output (input only)

//------------------------- Additional sensors ---------------------------------------------------------------
#define USE_ENVIRONMENTAL_SENSORS 0 // to use environmental sensors set to 1 otherwise set to 0  //<<<======>>>


#define ACQ   _I2S_CS42448  // selected acquisition interface  //<<<======>>>

// For ADC SE pins can be changed
#if ACQ == _ADC_0
  #define ADC_PIN A2 // can be changed  //<<<======>>>
  #define DIFF 0
#elif ACQ == _ADC_D
  #define ADC_PIN A10 //fixed analog pin
  #define DIFF 1
#elif ACQ == _ADC_S
  #define ADC_PIN1 A2 // can be changed //<<<======>>>
  #define ADC_PIN2 A3 // can be changed //<<<======>>>
  #define DIFF 0
#elif (ACQ == _I2S_32) || (ACQ == _I2S_32_MONO) || (ACQ == _I2S_TDM)
  #define NSHIFT 12 // number of bits to shift data to the right before extracting 16 bits //<<<======>>>
#endif

//------------------------- special Hardware configuration ----------------------------------------------------
#if ACQ == _I2S_TYMPAN
  #define TYMPAN_REVISION         TYMPAN_REV_C         //TYMPAN_REV_C or TYMPAN_REV_D   //<<<======>>>
  #define TYMPAN_INPUT_DEVICE     TYMPAN_INPUT_ON_BOARD_MIC // use the on-board microphones   //<<<======>>>
                                  //TYMPAN_INPUT_JACK_AS_MIC // use the microphone jack - defaults to mic bias 2.5V
                                  //TYMPAN_INPUT_JACK_AS_LINEIN // use the microphone jack - defaults to mic bias OFF
  #define input_gain_dB   10.5f   //<<<======>>>
  
  #define AIC_FS F_SAMP
  #define AIC_BITS        32  // could also be 16
  #if AIC_BITS==32
    #define NSHIFT 12 // number of bits to shift data to the right before extracting 16 bits //<<<======>>>
  #endif
#endif

#if  ACQ == _I2S_CS42448
  #define NUM_BITS        32  // could also be 16
  #if NUM_BITS==32
    #define NSHIFT 12 // number of bits to shift data to the right before extracting 16 bits //<<<======>>>
  #endif
#endif


#define MDEL -1     // maximal delay in buffer counts (128/fs each; for fs= 48 kHz: 128/48 = 2.5 ms each) //<<<======>>>
                    // MDEL == -1 connects ACQ interface directly to mux and queue

#define GEN_WAV_FILE  // generate wave files, if undefined generate raw data (with 512 byte header) //<<<======>>>

/****************************************************************************************/
// some structures to be used for controlling acquisition

#define MENU_PIN 3  // ground menu_pin to enter menu during startup

// -----------------------scheduled acquisition------------------------------------------
typedef struct
{ uint32_t on;  // acquisition on time in seconds
  uint32_t ad;  // acquisition file size in seconds
  uint32_t ar;  // acquisition rate, i.e. every ar seconds (if < on then continuous acquisition)
  uint32_t T1,T2; // first acquisition window (from T1 to T2) in Hours of day
  uint32_t T3,T4; // second acquisition window (from T1 to T2) in Hours of day
  uint32_t rec;  // time when recording started
  char name[8];   // prefix for recorder file names
} ACQ_Parameters_s;

// T1 to T3 are increasing hours, T4 can be before or after midnight
// choose for continuous recording {0,12,12,24}
// if "ar" > "on" the do dutycycle, i.e.sleep between on and ar seconds
//
// Example
// ACQ_Parameters_s acqParameters = {120, 60, 180, 0, 12, 12, 24, 0, "WMXZ"};
//  acquire 2 files each 60 s long (totaling 120 s)
//  sleep for 60 s (to reach 180 s acquisition interval)
//  acquire whole day (from midnight to noon and noon to midnight)
//

ACQ_Parameters_s acqParameters = { 30, 10, 60, 0, 12, 12, 24, 0, "WMXZ"}; //<<<======>>>

// the following global variable may be set from anywhere
// if one wanted to close file immediately
// mustClose = -1: disable this feature, close on time limit but finish to fill diskBuffer
// mustcClose = 0: flush data and close exactly on time limit
// mustClose = 1: flush data and close immediately (not for user, this will be done by program)

int16_t mustClose = -1;// initial value (can be -1: ignore event trigger or 0: implement event trigger) //<<<======>>>

//---------------------------------- snippet extraction module ---------------------------------------------
typedef struct
{  int32_t iproc;      // type of detection processor (0: high-pass-threshold; 1: Taeger-Kaiser-Operator)
   int32_t thresh;     // power SNR for snippet detection (-1: disable snippet extraction)
   int32_t win0;       // noise estimation window (in units of audio blocks)
   int32_t win1;       // detection watchdog window (in units of audio blocks typically 10x win0)
   int32_t extr;       // min extraction window
   int32_t inhib;      // guard window (inhibit follow-on secondary detections)
   int32_t nrep;       // noise only interval (nrep =0  indicates no noise archiving)
   int32_t ndel;        // pre trigger delay (in units of audio blocks)
} SNIP_Parameters_s; 

SNIP_Parameters_s snipParameters = { 0, -1, 1000, 10000, 3750, 375, 0, MDEL}; //<<<======>>>

//-------------------------- hibernate control---------------------------------------------------------------
// The following two lines control the maximal hibernate (sleep) duration
// this may be useful when using a powerbank, or other cases where frequent booting is desired
// is used in audio_hibernate.h
//#define SLEEP_SHORT             // comment when sleep duration is not limited   //<<<======>>>
#define ShortSleepDuration 60     // value in seconds (max sleep duration)    //<<<======>>>

//------------------------- configuration of trigger module ----------------------------------------------------
#define STEREO_TRIGGER 0
#define LEFT_TRIGGER 1
#define RIGHT_TRIGGER 2
#define ADC_TRIGGER 3
#define CENTROID_TRIGGER 4

#define PROCESS_TRIGGER CENTROID_TRIGGER  // can be changed //<<<======>>>

#if PROCESS_TRIGGER == ADC_TRIGGER
  #define extAnalogPin 1                  // can be changed //<<<======>>>
#endif

#if PROCESS_TRIGGER==CENTROID_TRIGGER // the follwing limits could be put to own menu
  // constants should be adapted to individual application
  #define FC_MIN 1      // min centroid frequency bin   // can be changed //<<<======>>>
  #define FC_MAX 255    // max centroid frequency bin   // can be changed //<<<======>>>
  #define FP_MIN 1      // min peak frequency bin       // can be changed //<<<======>>>
  #define FP_MAX 255    // max frequency bin            // can be changed //<<<======>>>
  #define PC_THR 10000  // centroid power threshold     // can be changed //<<<======>>>
  #define PP_THR 10000  // peak power threshold         // can be changed //<<<======>>>
  #define PW_THR 100    // average power threshold      // can be changed //<<<======>>>
#endif
//
/*********************** End possible User Modifications ********************************/
