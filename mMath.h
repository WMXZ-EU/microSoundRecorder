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
#ifndef MMATH_H
#define MMATH_H

#include "kinetis.h"
#include "core_pins.h"

// differentiate data (equivalent to single pole high-pass filter)
inline void mDiff(int32_t *aux, int16_t *inp, int16_t ndat, int16_t old)
{
    aux[0] = (inp[0]-old);
    for(int ii=1; ii< ndat; ii++) aux[ii] = (inp[ii] - inp[ii-1]);  
}

// maximal signal value
inline int32_t mSig(int32_t *aux, int16_t ndat)
{   int32_t maxVal=0;
    for(int ii=0; ii< ndat; ii++)
    { aux[ii] *= aux[ii];
        if(aux[ii]>maxVal) maxVal=aux[ii];
    }
    return maxVal;
}

// averaging
inline int32_t avg(int32_t *aux, int16_t ndat)
{   int64_t avg=0;
    for(int ii=0; ii< ndat; ii++){ avg+=aux[ii]; }
    return avg/ndat;
}

#if PROCESS_TRIGGER == CENTROID_TRIGGER
    // fft input data
    void doFFT(int32_t *aux, int16_t *inp, int32_t *buff, int16_t ndat)
    {
        // prepare auxilary buffer
        for(int ii=0; ii<ndat; ii++) aux[ii]=buff[ii];
        for(int ii=0; ii<ndat; ii++) aux[ndat+ii]=inp[ii];
        for(int ii=0; ii<ndat; ii++) buff[ii]=aux[ndat+ii];

        #if CMSIS>0
            // do 32-bit FFT
            arm_rfft_q31(&S32, aux, aux);
            // get power spectrum
            for(int ii=1; ii<ndat;ii++) aux[ii]=aux[2*ii]*aux[2*ii]+aux[2*ii+1]*aux[2*ii+1];
        #else
            for(int ii=0; ii<ndat; ii++) aux[ii]=inp[ii];
        #endif
    }
    // estimate centrod frequency
    inline int32_t doCentroidFreq(int32_t * aux, int16_t ndat)
    {
        int64_t centVal1=0;
        int64_t centVal2=0;
        for(int ii=0; ii< ndat; ii++)
        {   centVal1 += aux[ii];
            centVal2 += ii*aux[ii];
        }
        return centVal2/centVal1;
    }

    // estimate centriod spectral power
    inline int32_t doCentroidPow(int32_t *aux, int16_t ndat, int32_t ii)
    {
        return aux[ii];
    }
    // estimate peak frequency
    inline int32_t doPeakFreq(int32_t *aux, int16_t ndat)
    {   int32_t peakFreq=0;
        int32_t peakVal=0;
        for(int ii=0; ii<ndat; ii++) if(aux[ii]>peakVal){ peakVal=aux[ii]; peakFreq=ii;}
        return peakFreq;
    }
    // estimate peak spectral power
    inline int32_t doPeakPow(int32_t *aux, int16_t ndat, int32_t ii)
    {
        return aux[ii];
    }
    // estimate detection variable
    int32_t detVar(int32_t maxVal,int32_t avgVal,int32_t centFreq,int32_t centPow,int32_t peakFreq, int32_t peakPow)
    {
        if((centFreq < FC_MIN) || (centFreq > FC_MAX)) maxVal=0;
        if((peakFreq < FP_MIN) || (peakFreq > FP_MAX)) maxVal=0;
        if((centPow  < PC_THR) || (peakPow  < PP_THR)) maxVal=0;
        
        if(avgVal < PW_THR) maxVal=0;
        return maxVal;
    }
#endif

#endif