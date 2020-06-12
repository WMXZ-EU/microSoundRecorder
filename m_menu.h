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
 
#ifndef MENU_H
#define MENU_H

/**************** BasicAudioLogger Menu ***************************************/
/*
typedef struct
{	uint16_t on;	// acquisition on time in seconds
	uint16_t ad;	// acquisition file size in seconds
	uint16_t ar;	// acquisition rate, i.e. every ar seconds (if < on then continuous acqisition)
	uint16_t T1,T2; // first aquisition window (from T1 to T2) in Hours of day
	uint16_t T3,T4; // second aquisition window (from T1 to T2) in Hours of day
} ACQ_Parameters_s;
*/

/*
typedef struct
{  int32_t iproc;      // type of detection rocessor (0: hihg-pass-rheshold; 1: Taeger-Kaiser-Operator)
   int32_t thresh;     // power SNR for snippet detection
   int32_t win0;       // noise estimation window (in units of audio blocks)
   int32_t win1;       // detection watchdog window (in units of audio blocks typicaly 10x win0)
   int32_t extr;       // min extraction window
   int32_t inhib;      // guard window (inhibit follow-on secondary detections)
   int32_t nrep;       // noise only interval (nrep =0  indicates no noise archiving)
} SNIP_Parameters_s; 
*/


#include <TimeLib.h>

static char * getDate(char *text)
{
    sprintf(text,"%04d/%02d/%02d",year(), month(), day());
    return text;  
}

static char * getTime(char *text)
{
    sprintf(text,"%02d:%02d:%02d",hour(),minute(),second());
    return text;
}

void setRTCTime(int hr,int min,int sec,int dy, int mnth, int yr){
 // year can be given as full four digit year or two digts (2010 or 10 for 2010);  
 //it is converted to years since 1970
  if( yr > 99)
      yr = yr - 1970;
  else
      yr += 30;  
  
  tmElements_t tm;
  tm.Year = yr;
  tm.Month = mnth;
  tm.Day = dy;
  tm.Hour = hr;
  tm.Minute = min;
  tm.Second = sec;

  uint32_t tt = makeTime(tm);
  Teensy3Clock.set(tt);
  setTime(tt);
}

static void setDate(uint16_t year, uint16_t month, uint16_t day)
{
    setRTCTime(hour(),minute(),second(),day, month, year);
}

static void setTime(uint16_t hour, uint16_t minutes, uint16_t seconds)
{
    setRTCTime(hour,minutes,seconds,day(),month(),year());
}

/*
? o\n:  ESM_Logger reports "on_time" value
? a\n:  ESM_Logger reports "acq_time" value
? r\n:  ESM_Logger reports "rep_rate" value
? 1\n:  ESM_Logger reports "first_hour" value
? 2\n:  ESM_Logger reports "second_hour" value
? 3\n:  ESM_Logger reports "third_hour" value
? 4\n:  ESM_Logger reports "last_hour" value
? n\n:  ESM_Logger reports "name" value
? d\n:  ESM_Logger reports date
? t\n:  ESM_Logger reports time
*/
/*
? c\n:  iproc;      // Processot type
? h\n:  thresh;     // power SNR for snippet detection
? w\n:  win0;       // noise estimation window (in units of audio blocks)
? s\n:  win1;       // (slow) detection watchdog window (in units of audio blocks typicaly 10x win0)
? e\n:  extr;       // min extraction window
? i\n:  inhib;      // guard window (inhibit follow-on secondary detections)
? k\n:  nrep;       // noise only interval (nrep =0  indicates no noise archiving)
? p\n:  ndel;       // pre-trigger delay 
*/
char text[32]; // neded for text operations

extern ACQ_Parameters_s acqParameters;
extern SNIP_Parameters_s snipParameters;

static void printAll(void)
{
  Serial.printf("%c %5d on_time\n\r",     'o',acqParameters.on);
  Serial.printf("%c %5d acq_time\n\r",    'a',acqParameters.ad);
  Serial.printf("%c %5d rep_rate\n\r",    'r',acqParameters.ar);
  Serial.printf("%c %5d first_hour\n\r",  '1',acqParameters.T1);
  Serial.printf("%c %5d second_hour\n\r", '2',acqParameters.T2);
  Serial.printf("%c %5d third_hour\n\r",  '3',acqParameters.T3);
  Serial.printf("%c %5d last_hour\n\r",   '4',acqParameters.T4);
  Serial.println();
  Serial.printf("%c %s name\n\r",         'n',acqParameters.name);
  Serial.printf("%c %s date\n\r",         'd',getDate(text));
  Serial.printf("%c %s time\n\r",         't',getTime(text));
  Serial.println();
  Serial.printf("%c %5d processing type\r\n",       'c',snipParameters.iproc);
  Serial.printf("%c %5d threshold\r\n",             'h',snipParameters.thresh);
  Serial.printf("%c %5d noise window\r\n",          'w',snipParameters.win0);
  Serial.printf("%c %5d slow window\r\n",           's',snipParameters.win1);
  Serial.printf("%c %5d extraction window\r\n",     'e',snipParameters.extr);
  Serial.printf("%c %5d inhibit window\r\n",        'i',snipParameters.inhib);
  Serial.printf("%c %5d noise repetition rate\r\n", 'k',snipParameters.nrep);
  Serial.printf("%c %5d pre trigger delay\r\n",     'p',snipParameters.ndel);
  //
  Serial.println();
  Serial.println("exter 'a' to print this");
  Serial.println("exter '?c' to read value c=(o,a,r,1,2,3,4,n,d,t,c,h,w,s,m,i,k,p)");
  Serial.println("  e.g.: ?1 will print first hour");
  Serial.println("exter '!cval' to read value c=(0,a,r,1,2,3,4,n,d,t,c,h,w,s,m,i,k,p) and val is new value");
  Serial.println("  e.g.: !110 will set first hour to 10");
  Serial.println("exter 'xval' to exit menu (x is delay in minutes, -1 means immediate)");
  Serial.println("  e.g.: x10 will exit and hibernate for 10 minutes");
  Serial.println("        x-1 with exit and start immediately");
  Serial.println();
}

static void doMenu1(void)
{ // for enquiries
    while(!Serial.available());
    char c=Serial.read();
    
    if (strchr("oar1234ndtchwseikp", c))
    { switch (c)
      {
        case 'o': Serial.printf("%02d\r\n",acqParameters.on); break;
        case 'a': Serial.printf("%02d\r\n",acqParameters.ad); break;
        case 'r': Serial.printf("%02d\r\n",acqParameters.ar); break;
        case '1': Serial.printf("%02d\r\n",acqParameters.T1);break;
        case '2': Serial.printf("%02d\r\n",acqParameters.T2);break;
        case '3': Serial.printf("%02d\r\n",acqParameters.T3);break;
        case '4': Serial.printf("%02d\r\n",acqParameters.T4);break;
        case 'n': Serial.printf("%s\r\n",acqParameters.name);break; 
        
        case 'd': Serial.printf("%s\r\n",getDate(text));break;
        case 't': Serial.printf("%s\r\n",getTime(text));break;
        
        case 'c': Serial.printf("%04d\r\n",snipParameters.iproc);break;
        case 'h': Serial.printf("%04d\r\n",snipParameters.thresh);break;
        case 'w': Serial.printf("%04d\r\n",snipParameters.win0);break;
        case 's': Serial.printf("%04d\r\n",snipParameters.win1);break;
        case 'e': Serial.printf("%04d\r\n",snipParameters.extr);break;
        case 'i': Serial.printf("%04d\r\n",snipParameters.inhib);break;
        case 'k': Serial.printf("%04d\r\n",snipParameters.nrep);break;
        case 'p': Serial.printf("%04d\r\n",snipParameters.ndel);break;
      }
    }
}
#define MAX_VAL 1<<17 // maimal input value
int boundaryCheck(int val, int minVal, int maxVal)
{
  if(minVal < maxVal) // standard case
  {
    if(val<minVal) val=minVal;
    if(val>maxVal) val=maxVal;
  }
  else // wrap around when checking hours
  {
    if((val>maxVal) && (val<minVal)) val=maxVal;
    if((val>24)) val=24;
  }
  return val; 
}
int boundaryCheck2(int val, int minVal, int maxVal, int modVal)
{
  if(minVal < maxVal) // standard case
  {
    if(val<minVal) val=minVal;
    if(val>maxVal) val=maxVal;
  }
  else // wrap around when checking hours
  {
    if(val<0) val=0;
    if(val>modVal) val=modVal;
    // shift data to next good value
    if((val>maxVal) && (val<minVal))
    { if(val>(minVal+maxVal)/2) val = minVal; else val=maxVal;
    }
  }
  return val; 
}
/*
! o val\n:       ESM_Logger sets "on_time" value 
! a val\n:       ESM_Logger sets "acq_time" value
! r val\n:       ESM_Logger sets "rep_rate" value
! 1 val\n:       ESM_Logger sets "first_hour" value
! 2 val\n:       ESM_Logger sets "second_hour" value
! 3 val\n:       ESM_Logger sets "third_hour" value
! 4 val\n:       ESM_Logger sets "last_hour" value
! n val\n:       ESM_Logger sets "name" value 
! d datestring\n ESM_Logger sets date
! t timestring\n ESM_Logger sets time
! x delay\n      ESM_Logger exits menu and hibernates for the amount given in delay
*/
/*
! c val\n:  iproc;      // Processot type
! h val\n:  thresh;     // power SNR for snippet detection
! w val\n:  win0;       // noise estimation window (in units of audio blocks)
! s val\n:  win1;       // (slow) detection watchdog window (in units of audio blocks typicaly 10x win0)
! e val\n:  extr;       // min extraction window
! i val\n:  inhib;      // guard window (inhibit follow-on secondary detections)
! k val\n:  nrep;       // noise only interval (nrep =0  indicates no noise archiving)
! p val\n:  ndel;       // pre-trigger delay 
 */

static void doMenu2(void)
{ // for settings
    uint16_t year,month,day,hour,minutes,seconds;
    int T1=acqParameters.T1;
    int T2=acqParameters.T2;
    int T3=acqParameters.T3;
    //int T4=acqParameters.T4; mot used
    //
    while(!Serial.available());
    char c=Serial.read();
        
    if (strchr("oar1234ndtchwseikp", c))
    { switch (c)
      { case 'o': acqParameters.on   = boundaryCheck(Serial.parseInt(),0,MAX_VAL); break;
        case 'a': acqParameters.ad   = boundaryCheck(Serial.parseInt(),0,MAX_VAL); break;
        case 'r': acqParameters.ar   = boundaryCheck(Serial.parseInt(),0,MAX_VAL); break;
        case '1': acqParameters.T1   = boundaryCheck(Serial.parseInt(),0,24); break;
        case '2': acqParameters.T2   = boundaryCheck(Serial.parseInt(),T1,24); break;
        case '3': acqParameters.T3   = boundaryCheck(Serial.parseInt(),T2,24); break;
        case '4': acqParameters.T4   = boundaryCheck2(Serial.parseInt(),T3,T1,24); break;
        case 'n': for(int ii=0; ii<4;ii++) acqParameters.name[ii] = Serial.read();
                  acqParameters.name[4]=0; break;
        case 'd':     
                  year=   boundaryCheck(Serial.parseInt(),2000,3000);
                  month=  boundaryCheck(Serial.parseInt(),1,12);
                  day=    boundaryCheck(Serial.parseInt(),1,31);
                  setDate(year,month,day);
                  break;
        case 't': 
                  hour=     boundaryCheck(Serial.parseInt(),0,23);
                  minutes=  boundaryCheck(Serial.parseInt(),0,59);
                  seconds=  boundaryCheck(Serial.parseInt(),0,59);
                  setTime(hour,minutes,seconds);
                  break;
        //
        case 'c': snipParameters.iproc  = boundaryCheck(Serial.parseInt(),0,1); break;
        case 'h': snipParameters.thresh = boundaryCheck(Serial.parseInt(),-1,MAX_VAL); break;
        case 'w': snipParameters.win0   = boundaryCheck(Serial.parseInt(),0,MAX_VAL); break;
        case 's': snipParameters.win1   = boundaryCheck(Serial.parseInt(),0,MAX_VAL); break;
        case 'e': snipParameters.extr   = boundaryCheck(Serial.parseInt(),0,MAX_VAL); break;
        case 'i': snipParameters.inhib  = boundaryCheck(Serial.parseInt(),0,MAX_VAL); break;
        case 'k': snipParameters.nrep   = boundaryCheck(Serial.parseInt(),0,MAX_VAL); break;
        case 'p': snipParameters.ndel   = boundaryCheck(Serial.parseInt(),0,MDEL); break;
      }
    }  
}

int16_t doMenu(void)
{
  int16_t ret=0;
  do
  {
    while(!Serial.available());
    char c=Serial.read();
    
    if (strchr("?!xa", c))
    { switch (c)
      {
        case '?': doMenu1(); break;
        case '!': doMenu2(); break;
        case 'x': ret = Serial.parseInt(); break;
        case 'a': printAll(); break;
      }
    }
  } while(ret==0);
  return ret;
}

#endif
