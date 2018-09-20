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
 
#ifndef _AUDIO_HIBERNATE_H
#define _AUDIO_HIBERNATE_H

 // derived from duff's lowpower modes
 // 06-jun-17: changed some compiler directives
 //            added high speed mode of K66
 // using hybernate disconnects from USB so serial monitor will break (pre TD 1.42)
 
#include "kinetis.h"
#include "core_pins.h"

/******************* Seting Alarm **************************/
#define RTC_IER_TAIE_MASK       0x4u
#define RTC_SR_TAF_MASK         0x4u

void rtcSetup(void)
{
   SIM_SCGC6 |= SIM_SCGC6_RTC;// enable RTC clock
   RTC_CR |= RTC_CR_OSCE;// enable RTC
}

void rtcSetAlarm(uint32_t nsec)
{ // set alarm nsec seconds in the future
   RTC_TAR = RTC_TSR + nsec;
   RTC_IER |= RTC_IER_TAIE_MASK;
}

/********************LLWU**********************************/
#define LLWU_ME_WUME5_MASK       0x20u
#define LLWU_F3_MWUF5_MASK       0x20u
#define LLWU_MF5_MWUF5_MASK      0x20u

static void llwuISR(void)
{
    //
#if defined(HAS_KINETIS_LLWU_32CH)
    LLWU_MF5 |= LLWU_MF5_MWUF5_MASK; // clear source in LLWU Flag register
#else
    LLWU_F3 |= LLWU_F3_MWUF5_MASK; // clear source in LLWU Flag register
#endif
    //
    RTC_IER = 0;// clear RTC interrupts
}

static void llwuSetup(void)
{
  attachInterruptVector( IRQ_LLWU, llwuISR );
  NVIC_SET_PRIORITY( IRQ_LLWU, 2*16 );
//
  NVIC_CLEAR_PENDING( IRQ_LLWU );
  NVIC_ENABLE_IRQ( IRQ_LLWU );
//
  LLWU_PE1 = 0;
  LLWU_PE2 = 0;
  LLWU_PE3 = 0;
  LLWU_PE4 = 0;
#if defined(HAS_KINETIS_LLWU_32CH)
  LLWU_PE5 = 0;
  LLWU_PE6 = 0;
  LLWU_PE7 = 0;
  LLWU_PE8 = 0;
#endif
  LLWU_ME  = LLWU_ME_WUME5_MASK; //rtc alarm
//   
    SIM_SOPT1CFG |= SIM_SOPT1CFG_USSWE;
    SIM_SOPT1 |= SIM_SOPT1_USBSSTBY;
//
    PORTA_PCR0 = PORT_PCR_MUX(0);
    PORTA_PCR1 = PORT_PCR_MUX(0);
    PORTA_PCR2 = PORT_PCR_MUX(0);
    PORTA_PCR3 = PORT_PCR_MUX(0);

    PORTB_PCR2 = PORT_PCR_MUX(0);
    PORTB_PCR3 = PORT_PCR_MUX(0);
}

/********************* go to deep sleep *********************/
#define SMC_PMPROT_AVLLS_MASK   0x2u
#define SMC_PMCTRL_STOPM_MASK   0x7u
#define SCB_SCR_SLEEPDEEP_MASK  0x4u

// see SMC section (e.g. p 339 of K66) 
#define VLLS3 0x3 // RAM retained I/O states held
#define VLLS2 0x2 // RAM partially retained
#define VLLS1 0x1 // I/O states held
#define VLLS0 0x0 // all stop

#define VLLS_MODE VLLS0
static void gotoSleep(void)
{  
//  /* Make sure clock monitor is off so we don't get spurious reset */
   MCG_C6 &= ~MCG_C6_CME0;
   //

// if K66 is running in highspeed mode (>120 MHz) reduce speed
// is defined in kinetis.h and mk20dx128c
#if defined(HAS_KINETIS_HSRUN) && (F_CPU > 120000000)
    kinetis_hsrun_disable( );
#endif   
   /* Write to PMPROT to allow all possible power modes */
   SMC_PMPROT = SMC_PMPROT_AVLLS_MASK;
   /* Set the STOPM field to 0b100 for VLLSx mode */
   SMC_PMCTRL &= ~SMC_PMCTRL_STOPM_MASK;
   SMC_PMCTRL |= SMC_PMCTRL_STOPM(0x4); // VLLSx

   SMC_VLLSCTRL =  SMC_VLLSCTRL_VLLSM(VLLS_MODE);
   /*wait for write to complete to SMC before stopping core */
   (void) SMC_PMCTRL;

   SYST_CSR &= ~SYST_CSR_TICKINT;      // disable systick timer interrupt
   SCB_SCR |= SCB_SCR_SLEEPDEEP_MASK;  // Set the SLEEPDEEP bit to enable deep sleep mode (STOP)
   
   asm volatile( "wfi" );  // WFI instruction will start entry into STOP mode
   // will never return with "VLLS_MODE==VLLS0", but wake-up results in call to ResetHandler() in mk20dx128.c
}

void setWakeupCallandSleep(uint32_t nsec)
{  // set alarm to nsec seconds in the future and go to hibernate
   rtcSetup();
   llwuSetup();  
   rtcSetAlarm(nsec);
   yield();
#if DO_DEBUG>0
   Serial.println(nsec);
   pinMode(13,OUTPUT); digitalWriteFast(13,HIGH); delay(1000); digitalWriteFast(13,LOW);
#endif
   gotoSleep();
}

/***********************************************************************************************/
// control if sleep duration is limited
#ifndef SLEEP_SHORT
//  #define SLEEP_SHORT   // uncomment if default behaviour should be short sleep
#endif
#ifndef ShortSleepDuration
  #define ShortSleepDuration 60   // i.e. wake up every 'ShortSleepDuration' seconds
#endif
//
// flag can be 0 file to be open // time to shutdown if required
int16_t checkDutyCycle(ACQ_Parameters_s *acqParameters,int16_t flag)
{	static uint32_t t_start = 0;  // start of actual file
  static uint16_t recording = 0;  // acquisition has started

  uint32_t tt=RTC_TSR;
  struct tm tx=seconds2tm(tt);
  uint16_t to = tx.tm_hour;
  
  // check if we should sleep longer
  // sleep time between T2 and T3 and T4 and T1 (values are in hours)
  
  uint16_t T1 = acqParameters->T1;
  uint16_t T2 = acqParameters->T2;
  uint16_t T3 = acqParameters->T3;
  uint16_t T4 = acqParameters->T4;
  
  uint16_t doRecording=1;
  if (T4<T1) // e.g. 3-4, 23-1 (work over midnight)
    doRecording =   ((to>=T3) || (to<T4) || ((to>=T1) && (to<T2)));
  else		// e.g. 3-4, 23-24
    doRecording = (((to>=T1) && (to<T2)) || ((to>=T3) && (to<T4)));

  uint32_t nsec=0;
  if (doRecording) // we can record
  {
      uint16_t t_on = acqParameters->on;
      uint16_t t_dur = acqParameters->ad;
      uint16_t t_rep = acqParameters->ar;
      uint32_t t_rec = acqParameters->rec;
      
    if(flag>=0)
    { 
      if((flag>0) && (tt >= t_start + t_dur)) //we are indeed still recording
      { // need to close file
#if DO_DEBUG>0
        Serial.println("close acquisition");
#endif
        t_start = tt; // update start time for next file
        return -1; // flag to close acquisition
      }
      
      if( flag==0 )  // file is closed: new file
      { 
        if(!recording) // we are at the beginning of an acquisition cycle
        {
          { t_rec=tt; 
            acqParameters->rec=t_rec;
            recording=1; 
          } 
          // the following is for each new file
          t_start = tt; // beginning of each file
        }
        else
        // check is we end acquisition cycle
        if ((t_rep>t_on) && (tt >= t_rec + t_on))
        { // need to stop
          nsec = (t_rec + t_rep - tt);
          #ifdef SLEEP_SHORT
            if(nsec>ShortSleepDuration) nsec=ShortSleepDuration;
          #endif
#if DO_DEBUG>0
          Serial.println(nsec); 
          Serial.println("Hibernate now 1");
#endif
          return nsec; 
        }
      }
    }
    else // initial check during setup
    {
          // check if this is simply wakeup
          if((tt>t_rec+t_on) && (tt < t_rec+t_rep))
          {
            nsec = (t_rec+t_rep-tt);
            #ifdef SLEEP_SHORT
              if(nsec>ShortSleepDuration) nsec=ShortSleepDuration;
            #endif
#if DO_DEBUG>0
            Serial.println(nsec); 
            Serial.println("Hibernate now 2");
#endif
            return nsec; 
          }
    }
  }
  else
  {
    uint32_t tto= tt%(24*3600); // seconds since midnight
    nsec=0;
    // estimate next start time
    if ((to >= T2) && (to<T3))  // sleep during the day  //eg: to=10: T1=4; T2=9; T3=16; T4=20
    { if(tto < T3 * 3600) 
      nsec = T3 * 3600 - tto;
    }
    //
    if((to>=T4) && (T4>T1)) // sleep over midnight to T1 //eg: to=21: T1=4; T2=9; T3=16; T4=20
    {  nsec = (T1+24) * 3600 - tto;
    }
    //
    if(to<T1)                                            //eg: to=2:  T1=4; T2=9; T3=16; T4=20
    { nsec = T1 * 3600 - tto;
    }
  
    #ifdef SLEEP_SHORT
            if(nsec>ShortSleepDuration) nsec=ShortSleepDuration;
    #endif
    
#if DO_DEBUG>0
    Serial.println(nsec); 
    Serial.println("Hibernate now 3");
#endif
    return nsec;
  }
  return 0;
}

#endif
