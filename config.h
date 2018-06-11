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

 /*********************** Begin possible User Modifications ********************************/
// possible modifications are marked with //<<<======>>>
//----------------------------------------------------------------------------------------
#define F_SAMP 48000 // desired sampling frequency  //<<<======>>>
/*
 * NOTE: changing frequency impacts the macros 
 *      AudioProcessorUsage and AudioProcessorUsageMax
 * defined in stock AudioStream.h
 */

////////////////////////////////////////////////////////////

// possible ACQ interfaces
#define _ADC_0          0 // single ended ADC0
#define _ADC_D          1 // differential ADC0
#define _ADC_S          2 // stereo ADC0 and ADC1
#define _I2S            3 // I2S (16 bit stereo audio)
#define _I2S_32         4 // I2S (32 bit stereo audio), eg. two ICS43434 mics
#define _I2S_QUAD       5 // I2S (16 bit quad audio)
#define _I2S_32_MONO    6 // I2S (32 bit mono audio), eg. one ICS43434 mic
#define _I2S_TYMPAN     7 // I2S (16 bit tympan stereo audio audio) for use the tympan board

#define ACQ   _I2S  // selected acquisition interface  //<<<======>>>

// For ADC SE pins can be changed
#if ACQ == _ADC_0
  #define ADC_PIN A2 // can be changed  //<<<======>>>
  #define DIFF 0
#elif ACQ == _ADC_D
  #define ADC_PIN A10 //fixed
  #define DIFF 1
#elif ACQ == _ADC_S
  #define ADC_PIN1 A2 // can be changed //<<<======>>>
  #define ADC_PIN2 A3 // can be changed //<<<======>>>
  #define DIFF 0
#endif

#define MQUEU 550 // number of buffers in aquisition queue
#define MDEL 100    // maximal delay in buffer counts (128/fs each; for fs= 48 kHz: 128/48 = 2.5 ms each) //<<<======>>>
                  // MDEL == -1 conects ACQ interface directly to mux and queue
#define GEN_WAV_FILE  // generate wave files, if undefined generate raw data (with 512 byte header) //<<<======>>>

/****************************************************************************************/
// some structures to be used for controllong acquisition
// scheduled acquisition
typedef struct
{ uint32_t on;  // acquisition on time in seconds
  uint32_t ad;  // acquisition file size in seconds
  uint32_t ar;  // acquisition rate, i.e. every ar seconds (if < on then continuous acqisition)
  uint32_t T1,T2; // first aquisition window (from T1 to T2) in Hours of day
  uint32_t T3,T4; // second aquisition window (from T1 to T2) in Hours of day
  uint32_t rec;  // time when secording started
  char name[8];   // prefix for recorder file names
} ACQ_Parameters_s;

// T1 to T3 are increasing hours, T4 can be before or after midnight
// choose for continuous recording {0,12,12,24}
// if "ar" > "on" the do dutycycle, i.e.sleep between on and ar seconds
//
// Example
// ACQ_Parameters_s acqParameters = {120, 60, 180, 0, 12, 12, 24, 0, "WMXZ"};
//  acquire 2 files each 60 s long (totalling 120 s)
//  sleep for 60 s (to reach 180 s acquisition interval)
//  acquire whole day (from midnight to noon and noot to midnight)
//

ACQ_Parameters_s acqParameters = { 30, 10, 60, 3, 10, 18, 24, 0, "Mono"}; //<<<======>>>

// the following global variable may be set from anywhere
// if one wanted to close file immedately
// mustClose = -1: disable this feature, close on time limit but finish to fill diskBuffer
// mustcClose = 0: flush data and close exactly on time limit
// mustClose = 1: flush data and close immediately (not for user, this will be done by program)

int16_t mustClose = -1;// initial value (can be -1: ignore event trigger or 0: implement event trigger) //<<<======>>>

// snippet extraction modul
typedef struct
{  int32_t iproc;      // type of detection rocessor (0: hihg-pass-rheshold; 1: Taeger-Kaiser-Operator)
   int32_t thresh;     // power SNR for snippet detection (-1: disable snippet extraction)
   int32_t win0;       // noise estimation window (in units of audio blocks)
   int32_t win1;       // detection watchdog window (in units of audio blocks typicaly 10x win0)
   int32_t extr;       // min extraction window
   int32_t inhib;      // guard window (inhibit follow-on secondary detections)
   int32_t nrep;       // noise only interval (nrep =0  indicates no noise archiving)
   int32_t ndel;        // pre trigger delay (in units of audio blocks)
} SNIP_Parameters_s; 

SNIP_Parameters_s snipParameters = { 0, -1, 1000, 10000, 3750, 375, 0, MDEL}; //<<<======>>>

// The following two lines control the maximal hibernate (sleep) duration
// this may be useful when using a powerbank, or other cases where frequent booting is desired
// is used in audio_hibernate.h
#define SLEEP_SHORT             // comment when sleep duration is not limited   //<<<======>>>
#define ShortSleepDuration 60   // value in seconds

#define USE_ENVIRONMENTAL_SENSORS 1 // to use environmental sensors set tom 1 otherwise set to 0  //<<<======>>>

#if ACQ == _I2S_TYMPAN
  #undef USE_ENVIRONMENTAL_SENSORS
  #define USE_ENVIRONMENTAL_SENSORS 0 // for tympan switch off environmental sensors
  #undef MDEL
  #define MDEL -1
  #define TYMPAN_REVISION         TYMPAN_REV_C         //TYMPAN_REV_C or TYMPAN_REV_D   //<<<======>>>
  #define TYMPAN_INPUT_DEVICE     TYMPAN_INPUT_ON_BOARD_MIC // use the on-board microphones   //<<<======>>>
                                  //TYMPAN_INPUT_JACK_AS_MIC // use the microphone jack - defaults to mic bias 2.5V
                                  //TYMPAN_INPUT_JACK_AS_LINEIN // use the microphone jack - defaults to mic bias OFF
  #define input_gain_dB   10.5f   //<<<======>>>
  
#elif !((ACQ == _I2S_32) || (ACQ == _I2S_32_MONO))
  #undef USE_ENVIRONMENTAL_SENSORS
  #define USE_ENVIRONMENTAL_SENSORS 0 // for tympan switch off environmental sensors
  #undef MDEL
  #define MDEL -1
  
#endif
//
/*********************** End possible User Modifications ********************************/

