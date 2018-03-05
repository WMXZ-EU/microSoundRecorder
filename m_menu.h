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
{  int32_t thresh;     // power SNR for snippet detection
   int32_t win0;       // noise estimation window (in units of audio blocks)
   int32_t win1;       // detection watchdog window (in units of audio blocks typicaly 10x win0)
   int32_t extr;       // min extraction window
   int32_t inhib;      // guard window (inhibit follow-on secondary detections)
   int32_t nrep;       // noise only interval (nrep =0  indicates no noise archiving)
} SNIP_Parameters_s; 
*/


#include <time.h>
static uint32_t getRTC(void) {return RTC_TSR;}
static void setRTC(uint32_t tt)
{
  RTC_SR = 0;
  RTC_TPR = 0;
  RTC_TSR = tt;
  RTC_SR = RTC_SR_TCE;
}

static char * getDate(char *text)
{
    uint32_t tt=getRTC();
    struct tm tx =seconds2tm(tt);
    sprintf(text,"%04d/%02d/%02d",tx.tm_year, tx.tm_mon, tx.tm_mday);
    return text;  
}

static char * getTime(char *text)
{
    uint32_t tt=getRTC();
    struct tm tx =seconds2tm(tt);
    sprintf(text,"%02d:%02d:%02d",tx.tm_hour, tx.tm_min, tx.tm_sec);
    return text;
}

static void setDate(uint16_t year, uint16_t month, uint16_t day)
{
    uint32_t tt=getRTC();
    struct tm tx=seconds2tm(tt);
    tx.tm_year=year;
    tx.tm_mon=month;
    tx.tm_mday=day;
    tt=tm2seconds(&tx);
    setRTC(tt);
}

static void setTime(uint16_t hour, uint16_t minutes, uint16_t seconds)
{
    uint32_t tt=getRTC();
    struct tm tx=seconds2tm(tt);
    tx.tm_hour=hour;
    tx.tm_min=minutes;
    tx.tm_sec=seconds;
    tt=tm2seconds(&tx);
    setRTC(tt);
}

/*
? g\n:  ESM_Logger reports "on_time" value
? p\n:  ESM_Logger reports "acq_time" value
? r\n:  ESM_Logger reports "re_rate" value
? i\n:  ESM_Logger reports "first_hour" value
? u\n:  ESM_Logger reports "second_hour" value
? v\n:  ESM_Logger reports "third_hour" value
? f\n:  ESM_Logger reports "last_hour" value
? n\n:  ESM_Logger reports "name" value
? d\n:  ESM_Logger reports date
? t\n:  ESM_Logger reports time
*/
char text[32]; // neded for text operations

extern ACQ_Parameters_s acqParameters;
extern SNIP_Parameters_s snipParameters;

static void printAll(void)
{
  Serial.printf("%c %2d on_time\n\r",     'g',acqParameters.on);
  Serial.printf("%c %2d acq_time\n\r",    'p',acqParameters.ad);
  Serial.printf("%c %2d rep_rate\n\r",    'p',acqParameters.ar);
  Serial.printf("%c %2d first_hour\n\r",  '1',acqParameters.T1);
  Serial.printf("%c %2d second_hour\n\r", '2',acqParameters.T2);
  Serial.printf("%c %2d third_hour\n\r",  '3',acqParameters.T3);
  Serial.printf("%c %2d last_hour\n\r",   '4',acqParameters.T4;
//  Serial.printf("%c %s name\n\r",         'n',acqParameters.name);
  Serial.printf("%c %s date\n\r",         'd',getDate(text));
  Serial.printf("%c %s time\n\r",         't',getTime(text));
  Serial.println();
  Serial.println("exter 'a' to print this");
  Serial.println("exter '?c' to read value c=(g,p,r,1,2,3,4,n,d,t)");
  Serial.println("  e.g.: ?i will print first hour");
  Serial.println("exter '!cval' to read value c=(g,p,r,1,2,3,4,n,d,t) and val is new value");
  Serial.println("  e.g.: !i10 will set first hour to 10");
  Serial.println("exter 'xval' to exit menu (x is delay in minutes, -1 means immediate)");
  Serial.println("  e.g.: x10 will exit and hibernate for 10 minutes");
  Serial.println("        x-1 with exit and start immediately");
  Serial.println();
}

static void doMenu1(void)
{ // for enquiries
    while(!Serial.available());
    char c=Serial.read();
    
    if (strchr("gpr1234ndt", c))
    { switch (c)
      {
        case 'g': Serial.printf("%02d\r\n",acqParameters.on); break;
        case 'p': Serial.printf("%02d\r\n",acqParameters.ad); break;
        case 'r': Serial.printf("%02d\r\n",acqParameters.ar); break;
        case '1': Serial.printf("%02d\r\n",acqParameters.T1);break;
        case '2': Serial.printf("%02d\r\n",acqParameters.T2);break;
        case '3': Serial.printf("%02d\r\n",acqParameters.T3);break;
        case '4': Serial.printf("%02d\r\n",acqParameters.T4);break;
//        case 'n': Serial.printf("%s\r\n",acqParameters.name);break;  // could be (unique) mac address
        case 'd': Serial.printf("%s\r\n",getDate(text));break;
        case 't': Serial.printf("%s\r\n",getTime(text));break;
      }
    }
}

/*
! g val\n:       ESM_Logger sets "on_time" value 
! p val\n:       ESM_Logger sets "acq_time" value
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
static void doMenu2(void)
{ // for settings
    while(!Serial.available());
    char c=Serial.read();
    uint16_t year,month,day,hour,minutes,seconds;
    
    if (strchr("gpr1234ndt", c))
    { switch (c)
      {
        case 'g': acqParameters.on  =Serial.parseInt(); break;
        case 'p': acqParameters.ad  =Serial.parseInt(); break;
        case 'r': acqParameters.ar  =Serial.parseInt(); break;
        case '1': acqParameters.T1  =Serial.parseInt();break;
        case '2': acqParameters.T2  =Serial.parseInt();break;
        case '3': acqParameters.T3  =Serial.parseInt();break;
        case '4': acqParameters.T4  =Serial.parseInt();break;
//        case 'n': for(int ii=0; ii<4;ii++) parameters.name[ii] = Serial.read();
//                  parameters.name[4]=0; break;
        case 'd':     
                  year= Serial.parseInt();
                  month= Serial.parseInt();
                  day= Serial.parseInt();
                  setDate(year,month,day);
                  break;
        case 't': 
                  hour= Serial.parseInt();
                  minutes= Serial.parseInt();
                  seconds= Serial.parseInt();
                  setTime(hour,minutes,seconds);
                  break;
      }
      parMods=1;
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