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
#ifndef MPROCESS_H
#define MPROCESS_H

#include "kinetis.h"
#include "core_pins.h"

#include "AudioStream.h"

extern int16_t mustClose;
/*
// snippet extraction modul
typedef struct
{  int32_t threshold;  // power SNR for snippet detection
   int32_t win0;       // noise estimation window (in units of audio blocks)
   int32_t win1;       // detection watchdog window (in units of audio blocks typicaly 10x win0)
   int32_t extr;       // min extraction window
   int32_t inhib;      // guard window (inhibit followon secondary detections)
   int32_t nrep;       // noise only interval
   int32_t ndel;       // pre trigger delay (in units of audio blocks)
} SNIP_Parameters_s; 
*/

//
int32_t aux[AUDIO_BLOCK_SAMPLES];

extern volatile uint32_t maxValue, maxNoise;

class mProcess: public AudioStream
{
public:

  mProcess(SNIP_Parameters_s *param) : AudioStream(2, inputQueueArray) {}
  void begin(SNIP_Parameters_s *param);
  virtual void update(void);
  void setThreshold(int32_t val) {thresh=val;}
  int32_t getSigCount(void) {return sigCount;}
  int32_t getDetCount(void) {return detCount;}
  void resetDetCount(void) {detCount=0;}
  
protected:  
  audio_block_t *inputQueueArray[2];

private:
  int32_t sigCount;
  int32_t detCount;
  int32_t max1Val, max2Val, avg1Val, avg2Val;
  //
   int32_t thresh;  // power SNR for snippet detection
   int32_t win0;       // noise estimation window (in units of audio blocks)
   int32_t win1;       // detection watchdog window (in units of audio blocks typicaly 10x win0)
   int32_t extr;       // min extraction window 
   int32_t inhib;      // guard window (inhibit followon secondary detections)
   int32_t ndel;       // pre detection delays in block 
  //
  int32_t nest1, nest2;// background noise estimate
     
};
 
void mProcess::begin(SNIP_Parameters_s *param)
{  
//  blockCount=0;
  
  thresh=param->thresh;
  win0=param->win0;
  win1=param->win1;
  extr=param->extr;
  inhib=param->inhib;
  ndel=param->ndel;

  sigCount= -1; // start with no detection
  detCount=0;

  nest1=1<<10;
  nest2=1<<10;
}

// 6dB/octave high-pass filter
inline void mDiff(int32_t *aux, int16_t *inp, int16_t ndat, int16_t old)
{ aux[0]=(inp[0]-old);
  for(int ii=1; ii< ndat; ii++) aux[ii]=(inp[ii] - inp[ii-1]);  
}

inline int32_t mSig(int32_t *aux, int16_t ndat)
{ int32_t maxVal=0;
  for(int ii=0; ii< ndat; ii++)
  { aux[ii] = aux[ii]*aux[ii];  // assume data are 16 bit
    if(aux[ii]>maxVal) maxVal=aux[ii];
  }
  return maxVal;
}

inline int32_t avg(int32_t *aux, int16_t ndat)
{ int64_t avg=0;
  for(int ii=0; ii< ndat; ii++) {avg+=aux[ii]; }
  return avg/ndat;
}

void mProcess::update(void)
{
  audio_block_t *inp1, *inp2;//, *tmp1, *tmp2;
  inp1=receiveReadOnly(0);
  inp2=receiveReadOnly(1);
  
  if(!inp1 && !inp2) return; // have no input data
  if(thresh<0) // don't run detector
  {
    if(inp1) release(inp1);
    if(inp2) release(inp2);
    return;
  }
  
  // do here something useful with data 
  // example is a simple threshold detector on both channels
  // simple high-pass filter (6 db/octave)
  // followed by threshold detector

  int16_t ndat = AUDIO_BLOCK_SAMPLES;
  //
  // first channel
  if(inp1)
  {
    mDiff(aux, inp1->data, ndat, 0);
    max1Val = mSig(aux, ndat);
    avg1Val = avg(aux, ndat);
  }
  else
  {
    max1Val = 0;
    avg1Val = 0;
  }
  
  // second channel
  if(inp2)
  {
    mDiff(aux, inp2->data, ndat, 0);//out2? out2->data[ndat-1]: tmp2->data[0]);
    max2Val = mSig(aux, ndat);
    avg2Val = avg(aux, ndat);
  }
  else
  {
    max2Val = 0;
    avg2Val = 0;
  }
  
  //  done with processing of input data: release input buffers
  if(inp1) release(inp1);
  if(inp2) release(inp2);

  int32_t det1 = (max1Val > thresh*nest1);
  int32_t det2 = (max2Val > thresh*nest2);

  // 
  // on detection sigCount will be set to (extr+ndel) and counting down to -inhib
  // if sigCount>0, new detections will reset sigCount to max value (extr+ndel)
  // if sigCount reaches 0, storage will be closed.
  // new detections are only accepted if sigCount gets less than -inhib
  //

  if(((sigCount>0) || (sigCount<=-inhib)) && ( det1 || det2)) sigCount=extr+ndel; // retrigger extraction
  if(sigCount>0) detCount++;

  // reduce sigCount to a minimal value providing the possibility of a guard window
  // between two detections
  sigCount--; if(sigCount< -inhib) sigCount = -inhib; 

  // update background noise estimate
  uint32_t winx;
  // change averaging window according to detection status
  if(sigCount<0) winx=win0; else winx=win1;
    
  nest1=(((int64_t)nest1)*winx+(int64_t)(avg1Val-nest1))/winx;
  nest2=(((int64_t)nest2)*winx+(int64_t)(avg2Val-nest2))/winx;   

  // for debugging
  uint32_t tmp=(nest1>nest2)? nest1:nest2;
  maxNoise=(tmp>maxNoise)? tmp:maxNoise;
  
  tmp=(max1Val>max2Val)? max1Val:max2Val;
  maxValue=(tmp>maxValue)? tmp:maxValue;
}

#endif
