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
#define DO_DEBUG 2 // print debug info over usb-serial line  (2 write also log file)//<<<======>>>

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
#define _I2S_TYMPAN     7 // I2S (16 bit tympan stereo audio audio) for use the tympan board
#define _I2S_TDM        8 // I2S (8 channel TDM) // only first 5 channels are used (modify myAcq.h if less or more channels)

#define ACQ  _I2S_32_MONO // selected acquisition interface  //<<<======>>>

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

#define MDEL -1     // maximal delay in buffer counts (128/fs each; for fs= 48 kHz: 128/48 = 2.5 ms each) //<<<======>>>
                    // MDEL == -1 connects ACQ interface directly to mux and queue
                    // MDEL >= 0 switches on event detector
                    // MDEL > 0 delays detector by MDEL buffers 
#define MDET (MDEL>=0)

#define GEN_WAV_FILE  // generate wave files, if undefined generate raw data (with 512 byte header) //<<<======>>>

/****************************************************************************************/
// some structures to be used for controlling acquisition
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
//  acquire whole day (from midnight to noon and noot to midnight)
//

//ACQ_Parameters_s acqParameters = { 60, 10, 120, 0, 12, 12, 24, 0, "WMXZ"}; //<<<======>>>
ACQ_Parameters_s acqParameters = { 300, 300, 3600, 0, 12, 12, 24, 0, "WMXZ"}; //<<<======>>>


//---------------------------------- snippet extraction module ---------------------------------------------
typedef struct
{  int32_t iproc;      // type of detection processor (0: high-pass-threshold; 1: Taeger-Kaiser-Operator)
   int32_t thresh;     // power SNR for snippet detection (-1: disable snippet extraction)
   int32_t win0;       // noise estimation window (in units of audio blocks)
   int32_t win1;       // detection watchdog window (in units of audio blocks typically 10x win0)
   int32_t extr;       // min extraction window
   int32_t inhib;      // guard window (inhibit follow-on secondary detections)
   int32_t nrep;       // noise only interval (nrep =0  indicates no noise archiving, TBD)
   int32_t ndel;       // pre trigger delay (in units of audio blocks)
} SNIP_Parameters_s; 
// Note: 375 blocks is 1s for 48 kHz sampling

#if MDEL<0
  // continuous acquisition; disable detector
  #define THR -1
#else
  #define THR 100 // detection threshold (on power: 100 == 20 dB) //<<<======>>>
#endif

SNIP_Parameters_s snipParameters = { 0, THR, 1000, 10000, 38, 375, 0, MDEL}; //<<<======>>>


//-------------------------- hibernate control---------------------------------------------------------------
// The following two lines control the maximal hibernate (sleep) duration
// this may be useful when using a powerbank, or other cases where frequent booting is desired
// is used in audio_hibernate.h
//#define SLEEP_SHORT             // comment when sleep duration is not limited   //<<<======>>>
#define ShortSleepDuration 60   // value in seconds     //<<<======>>>

//------------------------- Additional sensors ---------------------------------------------------------------
#define USE_ENVIRONMENTAL_SENSORS 0 // to use environmental sensors set to 1 otherwise set to 0  //<<<======>>>

//------------------------- special Hardware configuration ----------------------------------------------------
#if ACQ == _I2S_TYMPAN
  #undef USE_ENVIRONMENTAL_SENSORS
  #define USE_ENVIRONMENTAL_SENSORS 0 // for tympan switch off environmental sensors
  #define TYMPAN_REVISION         TYMPAN_REV_C         //TYMPAN_REV_C or TYMPAN_REV_D   //<<<======>>>
  #define TYMPAN_INPUT_DEVICE     TYMPAN_INPUT_ON_BOARD_MIC // use the on-board microphones   //<<<======>>>
                                  //TYMPAN_INPUT_JACK_AS_MIC // use the microphone jack - defaults to mic bias 2.5V
                                  //TYMPAN_INPUT_JACK_AS_LINEIN // use the microphone jack - defaults to mic bias OFF
  #define input_gain_dB   10.5f   //<<<======>>>
#endif

//
/*********************** End possible User Modifications ********************************/
