 /* Audio Library for Teensy 3.X
 * Copyright (c) 2014, Paul Stoffregen, paul@pjrc.com
 *
 * Development of this audio library was funded by PJRC.COM, LLC by sales of
 * Teensy and Audio Adaptor boards.  Please support PJRC's efforts to develop
 * open source software by purchasing Teensy or other PJRC products.
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
 // this SW is modified from PJRC Audio library by WMXZ 
 // for use with the BasicAudioLogger
 //
#ifndef I2S_32_H
#define I2S_32_H

#include "kinetis.h"
#include "core_pins.h"

#include "AudioStream.h"
#include "DMAChannel.h"
#include "output_i2s.h"

class I2S_32 : public AudioStream
{
public:

	I2S_32(void) : AudioStream(0, NULL) {begin();}
  void begin(void);
  virtual void update(void);
  void digitalShift(int16_t val){I2S_32::shift=val;}
  
protected:  
  static bool update_responsibility;
  static DMAChannel dma;
  static void isr32(void);
  
private:
  static int16_t shift;
  static audio_block_t *block_left;
  static audio_block_t *block_right;
  static uint16_t block_offset;

  void config_i2s(void);
};

// for 32 bit I2S we need doubled buffer
DMAMEM static uint32_t i2s_rx_buffer_32[2*AUDIO_BLOCK_SAMPLES];
int16_t I2S_32::shift=8; //8 shifts 24 bit data to LSB

audio_block_t * I2S_32:: block_left = NULL;
audio_block_t * I2S_32:: block_right = NULL;
uint16_t I2S_32:: block_offset = 0;
bool I2S_32::update_responsibility = false;
DMAChannel I2S_32::dma(false);

void I2S_32::begin(void)
{ 

  dma.begin(true); // Allocate the DMA channel first

  config_i2s();

  CORE_PIN13_CONFIG = PORT_PCR_MUX(4); // pin 13, PTC5, I2S0_RXD0
  
  dma.TCD->SADDR = (void *)((uint32_t)&I2S0_RDR0);
  dma.TCD->SOFF = 0;
  dma.TCD->ATTR = DMA_TCD_ATTR_SSIZE(2) | DMA_TCD_ATTR_DSIZE(2);
  dma.TCD->NBYTES_MLNO = 4;
  dma.TCD->SLAST = 0;
  dma.TCD->DADDR = i2s_rx_buffer_32;
  dma.TCD->DOFF = 4;
  dma.TCD->CITER_ELINKNO = sizeof(i2s_rx_buffer_32) / 4;
  dma.TCD->DLASTSGA = -sizeof(i2s_rx_buffer_32);
  dma.TCD->BITER_ELINKNO = sizeof(i2s_rx_buffer_32) / 4;
  dma.TCD->CSR = DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;

  dma.triggerAtHardwareEvent(DMAMUX_SOURCE_I2S0_RX);
  update_responsibility = update_setup();
  dma.enable();

  I2S0_RCSR |= I2S_RCSR_RE | I2S_RCSR_BCE | I2S_RCSR_FRDE | I2S_RCSR_FR;
  I2S0_TCSR |= I2S_TCSR_TE | I2S_TCSR_BCE; // TX clock enable, because sync'd to TX

  dma.attachInterrupt(isr32); 
}

void I2S_32::isr32(void)
{
  uint32_t daddr, offset;
  const int32_t *src, *end;
  int16_t *dest_left, *dest_right;
  audio_block_t *left, *right;

  daddr = (uint32_t)(dma.TCD->DADDR);

  dma.clearInterrupt();
  
  if (daddr < (uint32_t)i2s_rx_buffer_32 + sizeof(i2s_rx_buffer_32) / 2) {
    // DMA is receiving to the first half of the buffer
    // need to remove data from the second half
    src = (int32_t *)&i2s_rx_buffer_32[AUDIO_BLOCK_SAMPLES];
    end = (int32_t *)&i2s_rx_buffer_32[AUDIO_BLOCK_SAMPLES*2];
    if (I2S_32::update_responsibility) AudioStream::update_all();
  } else {
    // DMA is receiving to the second half of the buffer
    // need to remove data from the first half
    src = (int32_t *)&i2s_rx_buffer_32[0];
    end = (int32_t *)&i2s_rx_buffer_32[AUDIO_BLOCK_SAMPLES];
  }
  
   // extract 16 but from 32 bit I2S buffer but shift to right first
   // there will be two buffers with each having "AUDIO_BLOCK_SAMPLES" samples
  left = I2S_32::block_left;
  right = I2S_32::block_right;
  if (left != NULL && right != NULL) {
    offset = I2S_32::block_offset;
    if (offset <= AUDIO_BLOCK_SAMPLES/2) {
      dest_left = &(left->data[offset]);
      dest_right = &(right->data[offset]);
      I2S_32::block_offset = offset + AUDIO_BLOCK_SAMPLES/2;
      do {
        *dest_left++ = (*src++)>>I2S_32::shift;
        *dest_right++ = (*src++)>>I2S_32::shift;
      } while (src < end);
    }
  }
}

void I2S_32::update(void)
{
  audio_block_t *new_left=NULL, *new_right=NULL, *out_left=NULL, *out_right=NULL;

  // allocate 2 new blocks, but if one fails, allocate neither
  new_left = allocate();
  if (new_left != NULL) {
    new_right = allocate();
    if (new_right == NULL) {
      release(new_left);
      new_left = NULL;
    }
  }
  __disable_irq();
  if (block_offset >= AUDIO_BLOCK_SAMPLES) {
    // the DMA filled 2 blocks, so grab them and get the
    // 2 new blocks to the DMA, as quickly as possible

//#define DO_SIMULATION
#ifdef DO_SIMULATION
    //simulate a signal every 1000 buffers
      static uint32_t count=0;
      count++;
      if(count==1000)
      { block_left->data[64]=1<<10;
        block_right->data[32]=1<<9;
        count=0;
      }
#endif
    out_left = block_left;
    block_left = new_left;
    out_right = block_right;
    block_right = new_right;
    block_offset = 0;
    __enable_irq();
    // then transmit the DMA's former blocks
    transmit(out_left, 0);
    release(out_left);
    transmit(out_right, 1);
    release(out_right);
  } else if (new_left != NULL) {
    // the DMA didn't fill blocks, but we allocated blocks
    if (block_left == NULL) {
      // the DMA doesn't have any blocks to fill, so
      // give it the ones we just allocated
      block_left = new_left;
      block_right = new_right;
      block_offset = 0;
      __enable_irq();
    } else {
      // the DMA already has blocks, doesn't need these
      __enable_irq();
      release(new_left);
      release(new_right);
    }
  } else {
    // The DMA didn't fill blocks, and we could not allocate
    // memory... the system is likely starving for memory!
    // Sadly, there's nothing we can do.
    __enable_irq();
  }
}


// MCLK needs to be 48e6 / 1088 * 256 = 11.29411765 MHz -> 44.117647 kHz sample rate
//
#if F_CPU == 96000000 || F_CPU == 48000000 || F_CPU == 24000000
  // PLL is at 96 MHz in these modes
  #define MCLK_MULT 2
  #define MCLK_DIV  17
#elif F_CPU == 72000000
  #define MCLK_MULT 8
  #define MCLK_DIV  51
#elif F_CPU == 120000000
  #define MCLK_MULT 8
  #define MCLK_DIV  85
#elif F_CPU == 144000000
  #define MCLK_MULT 4
  #define MCLK_DIV  51
#elif F_CPU == 168000000
  #define MCLK_MULT 8
  #define MCLK_DIV  119
#elif F_CPU == 180000000
  #define MCLK_MULT 16
  #define MCLK_DIV  255
  #define MCLK_SRC  0
#elif F_CPU == 192000000
  #define MCLK_MULT 1
  #define MCLK_DIV  17
#elif F_CPU == 216000000
  #define MCLK_MULT 8
  #define MCLK_DIV  153
  #define MCLK_SRC  0
#elif F_CPU == 240000000
  #define MCLK_MULT 4
  #define MCLK_DIV  85
#elif F_CPU == 16000000
  #define MCLK_MULT 12
  #define MCLK_DIV  17
#else
  #error "This CPU Clock Speed is not supported by the Audio library";
#endif

#ifndef MCLK_SRC
#if F_CPU >= 20000000
  #define MCLK_SRC  3  // the PLL
#else
  #define MCLK_SRC  0  // system clock
#endif
#endif

void I2S_32::config_i2s(void)
{
  SIM_SCGC6 |= SIM_SCGC6_I2S;
  SIM_SCGC7 |= SIM_SCGC7_DMA;
  SIM_SCGC6 |= SIM_SCGC6_DMAMUX;

  // if either transmitter or receiver is enabled, do nothing
  if (I2S0_TCSR & I2S_TCSR_TE) return;
  if (I2S0_RCSR & I2S_RCSR_RE) return;

  // enable MCLK output
  I2S0_MCR = I2S_MCR_MICS(MCLK_SRC) | I2S_MCR_MOE;
  while (I2S0_MCR & I2S_MCR_DUF) ;
  I2S0_MDR = I2S_MDR_FRACT((MCLK_MULT-1)) | I2S_MDR_DIVIDE((MCLK_DIV-1));

  // configure transmitter
  I2S0_TMR = 0;
  I2S0_TCR1 = I2S_TCR1_TFW(1);  // watermark at half fifo size
  I2S0_TCR2 = I2S_TCR2_SYNC(0) | I2S_TCR2_BCP | I2S_TCR2_MSEL(1)
    | I2S_TCR2_BCD | I2S_TCR2_DIV(1);
  I2S0_TCR3 = I2S_TCR3_TCE;
  I2S0_TCR4 = I2S_TCR4_FRSZ(1) | I2S_TCR4_SYWD(31) | I2S_TCR4_MF
    | I2S_TCR4_FSE | I2S_TCR4_FSP | I2S_TCR4_FSD;
  I2S0_TCR5 = I2S_TCR5_WNW(31) | I2S_TCR5_W0W(31) | I2S_TCR5_FBT(31);

  // configure receiver (sync'd to transmitter clocks)
  I2S0_RMR = 0;
  I2S0_RCR1 = I2S_RCR1_RFW(1);
  I2S0_RCR2 = I2S_RCR2_SYNC(1) | I2S_TCR2_BCP | I2S_RCR2_MSEL(1)
    | I2S_RCR2_BCD | I2S_RCR2_DIV(1);
  I2S0_RCR3 = I2S_RCR3_RCE;
  I2S0_RCR4 = I2S_RCR4_FRSZ(1) | I2S_RCR4_SYWD(31) | I2S_RCR4_MF
    | I2S_RCR4_FSE | I2S_RCR4_FSP | I2S_RCR4_FSD;
  I2S0_RCR5 = I2S_RCR5_WNW(31) | I2S_RCR5_W0W(31) | I2S_RCR5_FBT(31);

  // configure pin mux for 3 clock signals
  CORE_PIN23_CONFIG = PORT_PCR_MUX(6); // pin 23, PTC2, I2S0_TX_FS (LRCLK)
  CORE_PIN9_CONFIG  = PORT_PCR_MUX(6); // pin  9, PTC3, I2S0_TX_BCLK
  CORE_PIN11_CONFIG = PORT_PCR_MUX(6); // pin 11, PTC6, I2S0_MCLK
}

#endif
