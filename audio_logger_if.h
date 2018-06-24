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
#ifndef _AUDIO_LOGGER_IF_H
#define _AUDIO_LOGGER_IF_H

#include "core_pins.h"

//==================== local uSD interface ========================================
// this implementation used SdFs from Bill Greiman
// which needs to be installed as local library 
//
#include "SdFs.h"

#ifdef GEN_WAV_FILE
  char postfix[5]=".wav";
#else
  char postfix[5]=".raw";
#endif

// Preallocate 40MB file.
const uint64_t PRE_ALLOCATE_SIZE = 40ULL << 20;

// Use FIFO SDIO or DMA_SDIO
#define SD_CONFIG SdioConfig(FIFO_SDIO)
//#define SD_CONFIG SdioConfig(DMA_SDIO)

//#define MAXFILE 100
//#define MAXBUF 1000
#define BUFFERSIZE (8*1024)
int16_t diskBuffer[BUFFERSIZE];
int16_t *outptr = diskBuffer;

char header[512];

class c_uSD
{
  private:
    SdFs sd;
    FsFile file;
    
  public:
    c_uSD(): state(-1), closing(0) {;}
    void init();
    int16_t write(int16_t * data, int32_t ndat);
    uint16_t getNbuf(void) {return nbuf;}
    void setClosing(void) {closing=1;}

    int16_t close(void);
    void setPrefix(char *prefix);
  private:
    int16_t state; // 0 initialized; 1 file open; 2 data written; 3 to be closed
    int16_t nbuf;
    int16_t closing;

    char name[8];
//    char buffer[512];
    
  public:
  void loadConfig(uint32_t * param1, int n1, int32_t *param2, int n2);
  void storeConfig(uint32_t * param1, int n1, int32_t *param2, int n2);
  void writeTemperature(float temperature, float pressure, float humidity, uint16_t lux);
};
c_uSD uSD;

/*
 *  Logging interface support / implementation functions 
 */
//_______________________________ For File Time settings _______________________
#include <time.h>
#define EPOCH_YEAR 1970 //T3 RTC
#define LEAP_YEAR(Y) (((EPOCH_YEAR+Y)>0) && !((EPOCH_YEAR+Y)%4) && ( ((EPOCH_YEAR+Y)%100) || !((EPOCH_YEAR+Y)%400) ) )
static  const uint8_t monthDays[]={31,28,31,30,31,30,31,31,30,31,30,31}; 

/*  int  tm_sec;
  int tm_min;
  int tm_hour;
  int tm_mday;
  int tm_mon;
  int tm_year;
  int tm_wday;
  int tm_yday;
  int tm_isdst;
*/

struct tm seconds2tm(uint32_t tt)
{ struct tm tx;
  tx.tm_sec   = tt % 60;    tt /= 60; // now it is minutes
  tx.tm_min   = tt % 60;    tt /= 60; // now it is hours
  tx.tm_hour  = tt % 24;    tt /= 24; // now it is days
  tx.tm_wday  = ((tt + 4) % 7) + 1;   // Sunday is day 1 (tbv)

  // tt is now days since EPOCH_Year (1970)
  uint32_t year = 0;  
  uint32_t days = 0;
  while((unsigned)(days += (LEAP_YEAR(year) ? 366 : 365)) <= tt) year++;

  tx.tm_year = 1970+year; 

  // correct for last (actual) year
  days -= (LEAP_YEAR(year) ? 366 : 365);
  tt  -= days; // now tt is days in this year, starting at 0
  
  uint32_t mm=0;
  uint32_t monthLength=0;
  for (mm=0; mm<12; mm++) 
  { monthLength = monthDays[mm];
    if ((mm==1) & LEAP_YEAR(year)) monthLength++; 
    if (tt<monthLength) break;
    tt -= monthLength;
  }
  tx.tm_mon = mm + 1;   // jan is month 1  
  tx.tm_mday = tt + 1;     // day of month
  return tx;
}

uint32_t tm2seconds (struct tm *tx) 
{
  uint32_t tt;
  tt=tx->tm_sec+tx->tm_min*60+tx->tm_hour*3600;  

  // count days size epoch until previous midnight
  uint32_t days=tx->tm_mday-1;

  uint32_t mm=0;
  uint32_t monthLength=0;
  for (mm=0; mm<(tx->tm_mon-1); mm++) days+=monthDays[mm]; 
  if(tx->tm_mon>2 && LEAP_YEAR(tx->tm_year-1970)) days++;

  uint32_t years=0;
  while(years++ < (tx->tm_year-1970)) days += (LEAP_YEAR(years) ? 366 : 365);
  //  
  tt+=(days*24*3600);
  return tt;
}

// Call back for file timestamps (used by FS).  Only called for file create and sync().
void dateTime(uint16_t* date, uint16_t* time) 
{
  struct tm tx=seconds2tm(RTC_TSR);
    
  // Return date using FS_DATE macro to format fields.
  *date = FS_DATE(tx.tm_year, tx.tm_mon, tx.tm_mday);

  // Return time using FS_TIME macro to format fields.
  *time = FS_TIME(tx.tm_hour, tx.tm_min, tx.tm_sec);
}

char *makeFilename(char * prefix)
{ static char filename[40];

  struct tm tx = seconds2tm(RTC_TSR);
  sprintf(filename, "%s_%04d_%02d_%02d_%02d_%02d_%02d%s", prefix, 
                    tx.tm_year, tx.tm_mon, tx.tm_mday, tx.tm_hour, tx.tm_min, tx.tm_sec,postfix);
#if DO_DEBUG>0
  Serial.println(filename);
#endif
  return filename;  
}

char * headerUpdate(void)
{
	header[0] = 'W'; header[1] = 'M'; header[2] = 'X'; header[3] = 'Z';
	
	struct tm tx = seconds2tm(RTC_TSR);
	sprintf(&header[5], "%04d_%02d_%02d_%02d_%02d_%02d", tx.tm_year, tx.tm_mon, tx.tm_mday, tx.tm_hour, tx.tm_min, tx.tm_sec);
	//
	// add more info to header
	//

	return header;
}

char * wavHeader(uint32_t fileSize)
{
//  int fsamp=48000;
  int fsamp = F_SAMP;
#if ACQ == _I2S_32_MONO
  int nchan=1;
#else
  int nchan=2;
#endif

  int nbits=16;
  int nbytes=nbits/8;

  int nsamp=(fileSize-44)/(nbytes*nchan);
  //
  static char header[48]; // 44 for wav
  //
  strcpy(header,"RIFF");
  strcpy(header+8,"WAVE");
  strcpy(header+12,"fmt ");
  strcpy(header+36,"data");
  *(int32_t*)(header+16)= 16;// chunk_size
  *(int16_t*)(header+20)= 1; // PCM 
  *(int16_t*)(header+22)=nchan;// numChannels 
  *(int32_t*)(header+24)= fsamp; // sample rate 
  *(int32_t*)(header+28)= fsamp*nbytes; // byte rate
  *(int16_t*)(header+32)=nchan*nbytes; // block align
  *(int16_t*)(header+34)=nbits; // bits per sample 
  *(int32_t*)(header+40)=nsamp*nchan*nbytes; 
  *(int32_t*)(header+4)=36+nsamp*nchan*nbytes; 

   return header;
}
//____________________________ FS Interface implementation______________________
void c_uSD::init()
{
  if (!sd.begin(SD_CONFIG)) sd.errorHalt("sd.begin failed");
  // Set Time callback
  FsDateTime::callback = dateTime;
  //
  nbuf=0;
  state=0;
}

void c_uSD::setPrefix(char *prefix)
{
  strcpy(name,prefix);
}

int16_t c_uSD::write(int16_t *data, int32_t ndat)
{
  if(state == 0)
  { // open file
    char *filename = makeFilename(name);
    if(!filename) {state=-1; return state;} // flag to do not anything
    //
    if (!file.open(filename, O_CREAT | O_TRUNC |O_RDWR)) 
    {  sd.errorHalt("file.open failed");
    }
    if (!file.preAllocate(PRE_ALLOCATE_SIZE)) 
    { sd.errorHalt("file.preAllocate failed");    
    }
    #ifdef  GEN_WAV_FILE // keep first record
          memcpy(header,(const char *)data,512);
    #endif
    state=1; // flag that file is open
    nbuf=0;
  }
  
  if(state == 1 || state == 2)
  {  // write to disk
    state=2;
    if (2*ndat != file.write((char *) data, 2*ndat)) sd.errorHalt("file.write data failed");
    nbuf++;
    if(closing) {closing=0; state=3;}
  }
  
  if(state == 3)
  {
    state=close();
  }
  return state;
}
int16_t c_uSD::close(void)
{   // close file
    file.truncate();
    #ifdef GEN_WAV_FILE
       uint32_t fileSize = file.size();
       memcpy(header,wavHeader(fileSize),44);
       file.seek(0);
       file.write(header,512);
       file.seek(fileSize);
    #endif
    file.close();
#if DO_DEBUG>0
    Serial.println("file Closed");    
#endif
    state=0;  // flag to open new file
    return state;
}

void c_uSD::storeConfig(uint32_t * param1, int n1, int32_t *param2, int n2)
{ char text[32];
  file.open("Config.txt", O_CREAT|O_WRITE|O_TRUNC);
  for(int ii=0; ii<n1; ii++)
  { sprintf(text,"%10d\r\n",param1[ii]); file.write((uint8_t*)text,strlen(text));
  }
//
  for(int ii=0; ii<n2; ii++)
  { sprintf(text,"%10d\r\n",param2[ii]); file.write((uint8_t*)text,strlen(text));
  }
  sprintf(text,"%s\r\n",(char*) &param1[n1]);
  file.write((uint8_t *)text,6);

  file.close();
  
}

void c_uSD::loadConfig(uint32_t * param1, int n1, int32_t *param2, int n2)
{
  char text[32];
  if(!file.open("Config.txt",O_RDONLY)) return;
  //
  for(int ii=0; ii<n1; ii++)
  { if(file.read((uint8_t*)text,12)); sscanf(text,"%d",&param1[ii]);
  }
  for(int ii=0; ii<n2; ii++)
  { if(file.read((uint8_t*)text,12)); sscanf(text,"%d",&param2[ii]);
  }
  if(file.read((uint8_t *)text,6))
  { text[5]=0;
    sscanf(text,"%s",(char *) &param1[n1]);
  }  
  file.close();
}

// DD4WH, 24.5.2018
// write temperature data from a sensor to the SD card
// it is in Excel-readable txt-Format

// FILE STRUCTURE
// table with following columns
// date | time | temperature | pressure | humidity

// O_EXCL : Ensure that this call creates the file: if this flag is specified in conjunction with O_CREAT, 
//          and pathname already exists, then open() will fail. The behavior of O_EXCL is undefined if O_CREAT is not specified. 
// O_TRUNC : If the file already exists and is a regular file and the open mode allows writing (i.e., is O_RDWR or O_WRONLY) 
//           it will be truncated to length 0. If the file is a FIFO or terminal device file, the O_TRUNC flag is ignored. Otherwise the effect of O_TRUNC is unspecified.
// O_CREAT : If the file does not exist it will be created.   
// O_APPEND: The file is opened in append mode. Before each write(2), the file offset is positioned at the end of the file, as if with lseek(2).
// O_APPEND may lead to corrupted files on NFS file systems if more than one process appends data to a file at once. This is because NFS does not support appending to a file, so the client kernel has to simulate it, which can't be done without a race condition. 

void c_uSD::writeTemperature(float temperature, float pressure, float humidity, uint16_t lux)
{ 
  char text[32];
  char envfilename[24];
  sprintf(envfilename, "Envi_%s.txt", acqParameters.name);
  file.open(envfilename, O_CREAT|O_WRITE|O_APPEND);

    struct tm tx = seconds2tm(RTC_TSR);
    sprintf(text, "%04d_%02d_%02d,", tx.tm_year, tx.tm_mon, tx.tm_mday);  file.write((char*)text, strlen(text));
    sprintf(text, "%02d_%02d_%02d,", tx.tm_hour, tx.tm_min, tx.tm_sec);   file.write((char*)text, strlen(text));
    sprintf(text, "%10.1f,", temperature);   file.write((char*)text, strlen(text));
    sprintf(text, "%10.1f,", pressure);      file.write((char*)text, strlen(text));
    sprintf(text, "%10.1f,", humidity);      file.write((char*)text, strlen(text));
    sprintf(text, "%10d\r\n", lux);          file.write((char*)text, strlen(text));
  file.close(); 
}

#endif
