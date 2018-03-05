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

  if(numDelay >= MQ) return; // error don't do anything
  //
  
  int16_t h = (head + 1) % MQ;
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
  
  int16_t index = (head - numDelay + MQ) % MQ;
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
