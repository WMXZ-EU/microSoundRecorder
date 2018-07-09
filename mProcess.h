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
  int32_t getNoiseCount(void) {return noiseCount;}
  void resetDetCount(void) {detCount=0;}
  void resetNoiseCount(void) {noiseCount=0;}
  
protected:  
  audio_block_t *inputQueueArray[2];

private:
  int32_t sigCount;
  int32_t detCount;
  int32_t noiseCount;
  int32_t max1Val, max2Val, avg1Val, avg2Val;
  uint32_t blockCount;
  uint32_t watchdog;
  //
   int32_t thresh;  // power SNR for snippet detection
   int32_t win0;       // noise estimation window (in units of audio blocks)
   int32_t win1;       // detection watchdog window (in units of audio blocks typicaly 10x win0)
   int32_t extr;       // min extraction window 
   int32_t inhib;      // guard window (inhibit followon secondary detections)
   int32_t nrep;       // noise only interval (
  //
  int32_t nest1, nest2;// background noise estimate
     
  audio_block_t *out1, *out2;
};
 
void mProcess::begin(SNIP_Parameters_s *param)
{
  sigCount=-1;
  detCount=0;
  noiseCount=0;
  
  blockCount=0;
  
  thresh=param->thresh;
  win0=param->win0;
  win1=param->win1;
  extr=param->extr;
  inhib=param->inhib;
  nrep=param->nrep;

  nest1=1<<10;
  nest2=1<<10;
  
  out1=NULL;
  out2=NULL;

  watchdog=0;
}

inline void mDiff(int32_t *aux, int16_t *inp, int16_t ndat, int16_t old)
{
  aux[0]=(inp[0]-old);
  for(int ii=1; ii< ndat; ii++) aux[ii]=(inp[ii] - inp[ii-1]);  
}

inline int32_t mSig(int32_t *aux, int16_t ndat)
{ int32_t maxVal=0;
  for(int ii=0; ii< ndat; ii++)
  { aux[ii] *= aux[ii];
    if(aux[ii]>maxVal) maxVal=aux[ii];
  }
  return maxVal;
}

inline int32_t avg(int32_t *aux, int16_t ndat)
{ int64_t avg=0;
  for(int ii=0; ii< ndat; ii++){ avg+=aux[ii]; }
  return avg/ndat;
}

void mProcess::update(void)
{
  audio_block_t *inp1, *inp2, *tmp1, *tmp2;
  inp1=receiveReadOnly(0);
  inp2=receiveReadOnly(1);
  
  if(!inp1 && !inp2) return; // have no input data
  if(thresh<0) // don't run detector
  {
    if(inp1) release(inp1);
    if(inp2) release(inp2);
    return;
  }
  
  blockCount++;
  
  if(inp1) tmp1=allocate(); else tmp1=0;
  if(inp2) tmp2=allocate(); else tmp2=0;

  // store data and release input buffers
  for(int ii=0; ii< AUDIO_BLOCK_SAMPLES; ii++)
  {
    if(inp1) tmp1->data[ii]=inp1->data[ii];
    if(inp2) tmp2->data[ii]=inp2->data[ii];
  }
  if(inp1) release(inp1);
  if(inp2) release(inp2);

  // do here something useful with data 
  // example is a simple thresold detector on both channels
  // simple high-pass filter (6 db/octave)
  // followed by threshold detector

  int16_t ndat = AUDIO_BLOCK_SAMPLES;
  //
  // first channel
  if(tmp1)
  {
    mDiff(aux, tmp1->data, ndat, 0);//out1? out1->data[ndat-1]: tmp1->data[0]);
    max1Val = mSig(aux, ndat);
    avg1Val = avg(aux, ndat);
  }
  else
  {
    max1Val = 0;
    avg1Val = 0;
  }
  
  // second channel
  if(tmp2)
  {
    mDiff(aux, tmp2->data, ndat, 0);//out2? out2->data[ndat-1]: tmp2->data[0]);
    max2Val = mSig(aux, ndat);
    avg2Val = avg(aux, ndat);
  }
  else
  {
    max2Val = 0;
    avg2Val = 0;
  }
  
  //
  // if threshold detector fires, the open transmisssion of input data for "extr"
  // due to use of temprary storage, the block before detection will be transmitted first
  // that means 
  // "extr" ==2 means both, pre-trigger and trigger blocks are transmitted
  // "extr" ==3 means pre-trigger, trigger and a post-trigger blocks are transmitted
  // "extr" ==4 means pre-trigger, trigger and 2 post-trigger blocks are transmitted
  //
  // Re-triggering is possible
  // any detection during transmissions of blocks extends transmission
  //
  // 

//  static uint32_t watchdog=0;
  static int16_t isFirst = 1;
  int32_t det1 = (max1Val > thresh*nest1);
  int32_t det2 = (max2Val > thresh*nest2);

  if(((sigCount>0) || (sigCount<=-inhib)) && ( det1 || det2)) {sigCount=extr;}
  
  if(sigCount>0) // we have detection or still data to be transmitted
  { detCount++;
    // mark first block of new set of transmission // could remove or use millis()
    if(isFirst) // flag new data block (corrupting the first two words with blockCount)
    { //if(out1) {out1->data[0]=-1; out1->data[1]=-1;}
      //if(out2) {*(uint32_t*)(out2->data) = blockCount;}
      isFirst=0;
    }
    //
    if(out1) transmit(out1,0);
    if(out2) transmit(out2,1);
  }
  //
  // is we wanted single event file signal main program to close immediately if queue is empty
  if((sigCount==0) && (mustClose==0)) { Serial.println("mustClose"); mustClose=1;}

  if(mustClose<=0)
  { // transmit anyway a single buffer every now and then 
    if((nrep>0) && (sigCount<0) && ((watchdog % nrep)==0))
    { noiseCount++;
      //if(out1) {out1->data[0]=-1; out1->data[1]=-1;}
      //if(out2) {*(uint32_t*)(out2->data) = blockCount;}
      //
      if(out1) transmit(out1,0);
      if(out2) transmit(out2,1);
    }
  }  
  
  // reduce sigCount to a minimal value providing the possibility of a guard window
  // between two detections
  // increment a watchdog counter that allows regular transmisson of noise
  // flag that next triggered transmission is first
  sigCount--;
  if(sigCount< -inhib) { sigCount = -inhib; watchdog++; isFirst=1;}

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

  // clean up audio_blocks
  if(out1) release(out1);
  if(out2) release(out2);
  out1=tmp1;
  out2=tmp2;
}

#endif
