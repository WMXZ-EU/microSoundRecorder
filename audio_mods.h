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
#ifndef _AUDIO_MODS_H
#define _AUDIO_MODS_H
/*
 * NOTE: changing frequency impacts the macros 
 *      AudioProcessorUsage and AudioProcessorUsageMax
 * defined in stock AudioStream.h
 */
 
/*
 * -------------------mods for changing ADC sampling frequency-------------------------- 
 * 
 * following Mark Butcher: https://community.nxp.com/thread/434148
 * and https://community.nxp.com/thread/310745
 * 
 *  the waitfor cal was adapted from cores/teensy3/analog.c 
 *  as it is static declared in analog.c and not visible outside
 */
#include "kinetis.h"
#include "core_pins.h"

static void analogWaitForCal(void)
{ uint16_t sum;

#if defined(HAS_KINETIS_ADC0) && defined(HAS_KINETIS_ADC1)
  while ((ADC0_SC3 & ADC_SC3_CAL) || (ADC1_SC3 & ADC_SC3_CAL)) { }
#elif defined(HAS_KINETIS_ADC0)
  while (ADC0_SC3 & ADC_SC3_CAL) { }
#endif
  __disable_irq();
    sum = ADC0_CLPS + ADC0_CLP4 + ADC0_CLP3 + ADC0_CLP2 + ADC0_CLP1 + ADC0_CLP0;
    sum = (sum / 2) | 0x8000;
    ADC0_PG = sum;
    sum = ADC0_CLMS + ADC0_CLM4 + ADC0_CLM3 + ADC0_CLM2 + ADC0_CLM1 + ADC0_CLM0;
    sum = (sum / 2) | 0x8000;
    ADC0_MG = sum;
#ifdef HAS_KINETIS_ADC1
    sum = ADC1_CLPS + ADC1_CLP4 + ADC1_CLP3 + ADC1_CLP2 + ADC1_CLP1 + ADC1_CLP0;
    sum = (sum / 2) | 0x8000;
    ADC1_PG = sum;
    sum = ADC1_CLMS + ADC1_CLM4 + ADC1_CLM3 + ADC1_CLM2 + ADC1_CLM1 + ADC1_CLM0;
    sum = (sum / 2) | 0x8000;
    ADC1_MG = sum;
#endif
  __enable_irq();
}
//
 void modifyADCS(int16_t res, uint16_t avg, uint16_t diff, uint16_t hspd)
 { // Mono only
// stop PDB
  uint32_t ch0c1 = PDB0_CH0C1; // keep old value
  uint32_t ch1c1 = PDB0_CH1C1; // keep old value
  PDB0_CH0C1 = 0;   // disable ADC triggering
  PDB0_CH1C1 = 0;   // disable ADC triggering
  PDB0_SC &= ~PDB_SC_PDBEN;

  analogReadRes(res);
  analogReference(INTERNAL); // range 0 to 1.2 volts
  analogReadAveraging(avg);
  analogWaitForCal();
  //
//  while(ADC0_SC2&ADC_SC2_ADACT);
//    (void)ADC0_RA;
//    (void)ADC0_RB;

  if(diff)
    ADC0_SC1A |= ADC_SC1_DIFF;
  if(hspd)
    ADC0_CFG2 |= ADC_CFG2_ADHSC;

  ADC0_SC2 |= ADC_SC2_ADTRG | ADC_SC2_DMAEN;  // reassert HW trigger
  ADC1_SC2 |= ADC_SC2_ADTRG | ADC_SC2_DMAEN;  // reassert HW trigger

// restart PDB
  (void)ADC0_RA;
  (void)ADC1_RA;
  PDB0_CH0C1 = ch0c1;
  PDB0_CH1C1 = ch1c1;
  PDB0_SC |= PDB_SC_PDBEN ;
  PDB0_SC |= PDB_SC_SWTRIG ;  // kick off the PDB  - just once  
 }

//modify Sampling rate
#define PDB_CONFIG (PDB_SC_TRGSEL(15) | PDB_SC_PDBEN | PDB_SC_CONT | PDB_SC_PDBIE | PDB_SC_DMAEN)

void ADC_modification(uint32_t fsamp, uint16_t diff)
{ 
  uint16_t n_bits=16, n_avg=4, hspd=0;
  uint32_t fmax = 58500;
  if(diff) fmax = 46000;
  if(fsamp>fmax)         // assume that limit scales with n_avg
  { n_bits=12;
    n_avg=1;    
    hspd=1;
  }
  modifyADCS(n_bits,n_avg,diff,hspd);

  // sampling rate can be modified on the fly
  uint32_t PDB_period;
  PDB_period = F_BUS/fsamp -1;

  PDB0_MOD = PDB_period;
  PDB0_SC = PDB_CONFIG | PDB_SC_LDOK;
}

// ********************************************** following is to change I2S samling rates ********************
// attempt to generate dividers programmatically
// always better to check
void I2S_dividers(uint32_t *iscl, uint32_t fsamp, uint32_t nbits)
{ // nbits is number of bits / frame
    int64_t i1 = 1;
    int64_t i2 = 1;
    int64_t i3 = iscl[2]+1;
    float A=F_CPU/2.0f/i3/((float)nbits*fsamp);
    float mn=1.0; 
    for(int ii=1;ii<=32;ii++) 
    { float xx;
      xx=A*ii-(int32_t)(A*ii);
      if(xx<mn && A*ii<256.0) { mn=xx; i1=ii; i2=A*ii;} //select first candidate
    }
    iscl[0] = (int) (i1-1);
    iscl[1] = (int) (i2-1);
    iscl[2] = (int) (i3-1);
}

void I2S_stopClock(void)
{
      SIM_SCGC6 &= ~SIM_SCGC6_I2S;
}

void I2S_stop(void)
{
    I2S0_RCSR &= ~(I2S_RCSR_RE | I2S_RCSR_BCE);
}

void I2S_modification(uint32_t fsamp, uint16_t nbits, int nch)
{ uint32_t iscl[3];

  iscl[2]=1;
  I2S_dividers(iscl, fsamp ,nch*nbits);
  float fs = (F_CPU * (iscl[0]+1.0f)) / (iscl[1]+1l) / 2 / (iscl[2]+1l) / ((float)nch*nbits);
#if DO_DEBUG>0
  Serial.printf("%d %d %d %d %d %d %d\n\r",
                F_CPU, fsamp, (int)fs, nbits,iscl[0]+1,iscl[1]+1,iscl[2]+1);
#endif
  // stop I2S
  I2S0_RCSR &= ~(I2S_RCSR_RE | I2S_RCSR_BCE);

  // modify sampling frequency
  I2S0_MDR = I2S_MDR_FRACT(iscl[0]) | I2S_MDR_DIVIDE(iscl[1]);

  // configure transmitter
  I2S0_TCR2 = I2S_TCR2_SYNC(0) | I2S_TCR2_BCP | I2S_TCR2_MSEL(1)
    | I2S_TCR2_BCD | I2S_TCR2_DIV(iscl[2]);
  I2S0_TCR4 = I2S_TCR4_FRSZ(nch-1) | I2S_TCR4_SYWD(0) | I2S_TCR4_MF
    | I2S_TCR4_FSE | I2S_TCR4_FSD;

  // configure receiver (sync'd to transmitter clocks)
  I2S0_RCR2 = I2S_RCR2_SYNC(1) | I2S_TCR2_BCP | I2S_RCR2_MSEL(1)
    | I2S_RCR2_BCD | I2S_RCR2_DIV(iscl[2]);
  I2S0_RCR4 = I2S_RCR4_FRSZ(nch-1) | I2S_RCR4_SYWD(0) | I2S_RCR4_MF
    | I2S_RCR4_FSE | I2S_RCR4_FSD;

  //restart I2S
  I2S0_RCSR |= I2S_RCSR_RE | I2S_RCSR_BCE;
}
#endif
