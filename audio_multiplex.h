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
 
#ifndef _AUDIO_MULTIPLEX_H
#define _AUDIO_MULTIPLEX_H
#include "kinetis.h"
#include "core_pins.h"
#include "AudioStream.h"

typedef void(*Fxn_t)(void);

class AudioStereoMultiplex: public AudioStream
{
public:
	AudioStereoMultiplex(Fxn_t Fx) : AudioStream(2, inputQueueArray) { fx = Fx; }
  virtual void update(void);
private:
	Fxn_t fx;
  audio_block_t *inputQueueArray[2];
};

void AudioStereoMultiplex::update(void)
{
  audio_block_t *inp1, *inp2, *out1, *out2;
  inp1=receiveReadOnly(0);
  inp2=receiveReadOnly(1);
  if(!inp1 && !inp2) return;
  //
  out1=allocate();
  out2=allocate();

  int jj=0;
  for(int ii=0; ii< AUDIO_BLOCK_SAMPLES/2; ii++)
  {
    if(inp1) out1->data[jj]=inp1->data[ii]; jj++;
    if(inp2) out1->data[jj]=inp2->data[ii]; jj++;
  }
  
  jj=0;
  for(int ii=AUDIO_BLOCK_SAMPLES/2; ii< AUDIO_BLOCK_SAMPLES; ii++)
  {
    if(inp1) out2->data[jj]=inp1->data[ii]; jj++;
    if(inp2) out2->data[jj]=inp2->data[ii]; jj++;
  }

  if(inp1) release(inp1);
  if(inp2) release(inp2);
  
  transmit(out1);
  release(out1); fx();
  transmit(out2);
  release(out2);
}

class AudioQuadMultiplex: public AudioStream
{
public:
	AudioQuadMultiplex(Fxn_t Fx) : AudioStream(4, inputQueueArray) { fx = Fx; }
	virtual void update(void);
private:
	Fxn_t fx;
  audio_block_t *inputQueueArray[4];
};

void AudioQuadMultiplex::update(void)
{
  audio_block_t *inp1, *inp2, *inp3, *inp4;
  audio_block_t *out1, *out2, *out3, *out4;
  inp1=receiveReadOnly(0);
  inp2=receiveReadOnly(1);
  inp3=receiveReadOnly(2);
  inp4=receiveReadOnly(3);
  if(!inp1 && !inp2 && !inp3 && !inp4 ) return;

  out1=allocate();
  out2=allocate();
  out3=allocate();
  out4=allocate();

  int jj=0;
  for(int ii=0; ii< AUDIO_BLOCK_SAMPLES/4; ii++)
  {
    if(inp1) out1->data[jj]=inp1->data[ii]; jj++;
    if(inp2) out1->data[jj]=inp2->data[ii]; jj++;
    if(inp3) out1->data[jj]=inp3->data[ii]; jj++;
    if(inp4) out1->data[jj]=inp4->data[ii]; jj++;
  }
  jj=0;
  for(int ii=AUDIO_BLOCK_SAMPLES/4; ii< AUDIO_BLOCK_SAMPLES/2; ii++)
  {
    if(inp1) out2->data[jj]=inp1->data[ii]; jj++;
    if(inp2) out2->data[jj]=inp2->data[ii]; jj++;
    if(inp3) out2->data[jj]=inp3->data[ii]; jj++;
    if(inp4) out2->data[jj]=inp4->data[ii]; jj++;
  }
  jj=0;
  for(int ii=AUDIO_BLOCK_SAMPLES/2; ii< 3*AUDIO_BLOCK_SAMPLES/4; ii++)
  {
    if(inp1) out3->data[jj]=inp1->data[ii]; jj++;
    if(inp2) out3->data[jj]=inp2->data[ii]; jj++;
    if(inp3) out3->data[jj]=inp3->data[ii]; jj++;
    if(inp4) out3->data[jj]=inp4->data[ii]; jj++;
  }
  jj=0;
  for(int ii=3*AUDIO_BLOCK_SAMPLES/4; ii< AUDIO_BLOCK_SAMPLES; ii++)
  {
    if(inp1) out4->data[jj]=inp1->data[ii]; jj++;
    if(inp2) out4->data[jj]=inp2->data[ii]; jj++;
    if(inp3) out4->data[jj]=inp3->data[ii]; jj++;
    if(inp4) out4->data[jj]=inp4->data[ii]; jj++;
  }

  if(inp1) release(inp1);
  if(inp2) release(inp2);
  if(inp3) release(inp3);
  if(inp4) release(inp4);
  
  transmit(out1);
  release(out1); fx();
  transmit(out2);
  release(out2); fx();
  transmit(out3);
  release(out3); fx();
  transmit(out4);
  release(out4);
}
//--------------------------------------------------------------------------------------
template <int NC>
class Audio_Multiplex: public AudioStream
{
public:
  Audio_Multiplex(Fxn_t Fx) : AudioStream(NC, inputQueueArray) { fx = Fx; clearBlocks();}
  virtual void update(void);
private:
  Fxn_t fx;
  audio_block_t *inputQueueArray[NC];
  audio_block_t *inp[NC];
  audio_block_t *out[NC];
  void clearBlocks(void) { for(int ii=0;ii<NC;ii++) { inp[ii]=0;}}
};

template <int NC>
void Audio_Multiplex<NC>::update(void)
{ audio_block_t *tmp;
  for(int ii=0;ii<NC;ii++) if(!inp[ii]) inp[ii]= receiveReadOnly(ii);
  for(int ii=0;ii<NC;ii++) if(!inp[ii]) return; // continue only is all channels have arrived
  for(int ii=0;ii<NC;ii++) out[ii]=allocate();
  
  // multiplex data
  for(int ii=0; ii<AUDIO_BLOCK_SAMPLES*NC;ii++)
  { int i1 = ii % NC;
    int i2 = ii / NC;
    int j1 = ii / AUDIO_BLOCK_SAMPLES;
    int j2 = ii % AUDIO_BLOCK_SAMPLES;
    out[j1]->data[j2] = inp[i1]->data[i2];    
  }
  
  for(int ii=0;ii<NC;ii++)
  { release(inp[ii]);
    transmit(out[ii]);
    release(out[ii]); if(ii<NC-1) fx();
  }
  clearBlocks();
}

#endif
