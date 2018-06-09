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
 
#ifndef M_DELAY_H
#define M_DELAY_H

#include "AudioStream.h"

template <int NCH, int MQ>
class mDelay : public AudioStream
{
public:

  mDelay(int del) : AudioStream(NCH, inputQueueArray), head(MQ), numDelay(del){ reset(); }
  void reset(void);
  void setDelay(int16_t ndel) {numDelay=ndel;}
  virtual void update(void);
  
protected:  
  audio_block_t *inputQueueArray[NCH];

private:
  audio_block_t * volatile queue[NCH][MQ];
  volatile uint16_t head, numDelay;

};

template <int NCH, int MQ>
void mDelay<NCH,MQ>::reset(void)
{
  for(int ii=0; ii<NCH; ii++) for (int jj=0; jj<MQ; jj++) queue[ii][jj]=NULL;
  head=MQ;
}

template <int NCH, int MQ>
void mDelay<NCH,MQ>::update(void)
{
  audio_block_t *block;

  if((numDelay<=0) || (numDelay >= MQ)) // bypass queue
  {
    for(int ii=0;ii<NCH;ii++)
    {
      block = receiveReadOnly(ii);
      if(block)
      {
        transmit(block,ii);
        release(block);        
      }
    }
    return;
  }
  //
  
  uint16_t h = (head + 1) % MQ;
  for(int ii=0;ii<NCH;ii++)
  {
    if(queue[ii][h]) release(queue[ii][h]);
    block = receiveReadOnly(ii);
    if(block)
    {
      queue[ii][h]=block;
    }
    else
      queue[ii][h]=NULL;
  }
  head = h;

  uint16_t index = ((head  + MQ) - numDelay) % MQ;
  for(int ii=0;ii<NCH;ii++)
  {
    if(queue[ii][index])
    {
      transmit(queue[ii][index],ii);
      release(queue[ii][index]);
      queue[ii][index]=NULL;
    }
  }
}

#endif
