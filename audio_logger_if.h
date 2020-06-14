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
#ifndef _AUDIO_LOGGER_IF_H
#define _AUDIO_LOGGER_IF_H

#include "core_pins.h"

//==================== local uSD interface ========================================
// this implementation used SdFat-beta from Bill Greiman
// which needs to be installed as local library 
// rename in SdFat-beta/src the file SdFat.h to SdFat-beta.h
// to avoid confict with stock SD library needed for Audio library
//
#include "SdFat-beta.h" 

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
#define BUFFERSIZE (8*8*128)
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
    int16_t isClosing(void) {return closing;}

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

#include <TimeLib.h>

// Call back for file timestamps (used by FS).  Only called for file create and sync().
  void dateTime(uint16_t* date, uint16_t* time, uint8_t* ms10) 
{
  // Return date using FS_DATE macro to format fields.
  *date = FS_DATE(year(), month(), day());

  // Return time using FS_TIME macro to format fields.
  *time = FS_TIME(hour(), minute(), second());

  // Return low time bits in units of 10 ms.
  *ms10 = second() & 1 ? 100 : 0;
}

char *makeFilename(char * prefix)
{ static char filename[40];

  sprintf(filename, "%s_%04d_%02d_%02d_%02d_%02d_%02d%s", prefix, 
                    year(), month(), day(), hour(), minute(), second(), postfix);
#if DO_DEBUG>0
  Serial.println(filename);
#endif
  return filename;  
}

char * headerUpdate(void)
{
	header[0] = 'W'; header[1] = 'M'; header[2] = 'X'; header[3] = 'Z';
	
	sprintf(&header[5], "%04d_%02d_%02d_%02d_%02d_%02d", 
                        year(), month(), day(), hour(), minute(), second());
	//
	// add more info to header
	//

	return header;
}

char * wavHeader(uint32_t fileSize)
{
//  int fsamp=48000;
  int fsamp = F_SAMP;
  int nchan=NCH;

  int nbits=16;
  int nbytes=nbits/8;

  int nsamp=(fileSize-44)/(nbytes*nchan);
  //
  static char wheader[48]; // 44 for wav
  //
  strcpy(wheader,"RIFF");
  strcpy(wheader+8,"WAVE");
  strcpy(wheader+12,"fmt ");
  strcpy(wheader+36,"data");
  *(int32_t*)(wheader+16)= 16;// chunk_size
  *(int16_t*)(wheader+20)= 1; // PCM 
  *(int16_t*)(wheader+22)=nchan;// numChannels 
  *(int32_t*)(wheader+24)= fsamp; // sample rate 
  *(int32_t*)(wheader+28)= fsamp*nbytes; // byte rate
  *(int16_t*)(wheader+32)=nchan*nbytes; // block align
  *(int16_t*)(wheader+34)=nbits; // bits per sample 
  *(int32_t*)(wheader+40)=nsamp*nchan*nbytes; 
  *(int32_t*)(wheader+4)=36+nsamp*nchan*nbytes; 

   return wheader;
}
//____________________________ FS Interface implementation______________________
void c_uSD::init()
{
  if (!sd.begin(SD_CONFIG))
  {
//    sd.errorHalt("sd.begin failed");
    while(1)
    {
      // blink code suggests insertion of an SD card
            pinMode(13,OUTPUT);
            digitalWriteFast(13,HIGH);
            delay(200);
            digitalWriteFast(13,LOW);
            delay(200);
    }
  }

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
    if (!file.open(filename, O_CREAT | O_TRUNC |O_RDWR)) sd.errorHalt("file.open failed");
    if (!file.preAllocate(PRE_ALLOCATE_SIZE)) sd.errorHalt("file.preAllocate failed");
    #ifdef  GEN_WAV_FILE // keep first record
          memcpy(header,(const char *)data,512);
    #endif
    state=1; // flag that file is open
    nbuf=0;
  }
  
  if(state == 1 || state == 2)
  {  // write to disk
    state=2;
    if (2*ndat != (int32_t) file.write((char *) data, 2*ndat)) sd.errorHalt("file.write data failed");
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
  { sprintf(text,"%10d\r\n",(int) param1[ii]); file.write((uint8_t*)text,strlen(text));
  }
//
  for(int ii=0; ii<n2; ii++)
  { sprintf(text,"%10d\r\n",(int) param2[ii]); file.write((uint8_t*)text,strlen(text));
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
  { if(file.read((uint8_t*)text,12)>0) sscanf(text,"%d",(int *) &param1[ii]);
  }
  for(int ii=0; ii<n2; ii++)
  { if(file.read((uint8_t*)text,12)>0) sscanf(text,"%d", (int *)&param2[ii]);
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
// date | time | temperature | pressure | humidity | lux

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

    sprintf(text, "%04d_%02d_%02d,", year(), month(), day());  file.write((char*)text, strlen(text));
    sprintf(text, "%02d_%02d_%02d,", hour(), minute(), second());   file.write((char*)text, strlen(text));
    sprintf(text, "%10.1f,", temperature);   file.write((char*)text, strlen(text));
    sprintf(text, "%10.1f,", pressure);      file.write((char*)text, strlen(text));
    sprintf(text, "%10.1f,", humidity);      file.write((char*)text, strlen(text));
    sprintf(text, "%10d\r\n", lux);          file.write((char*)text, strlen(text));
  file.close(); 
}

#endif
