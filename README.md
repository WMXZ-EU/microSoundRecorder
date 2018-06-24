


# microSoundRecorder
Environmental Sound Recorder for Teensy 3.6. It builds on the BasicAudioLogger, but will be developed into a tool suited for environmental sound monitoring and bio-acoustic research.

More information can be found in the build-in Wiki

## supports 
### build-in Interfaces
- ADC, ADC_Stereo
- I2S, I2S_32, I2S_32_MONO, I2S_QUAD 
- I2S_TYMPAN
- I2S_TDM

The I2S_32 mode allows an improved acquisition of two 24 bit MEMS microphones

The I2S_32_MONO mode allows an improved acquisition of one 24 bit MEMS microphone, reducing also used disk space

The I2S_TYMPAN mode allows the high quality TYMPAN audio board https://tympan.org/ to be used as audio recorder

The I2S_TDM mode allows multiple digital(e.g. ICS-52000 ) microphones be used in TDM mode

## implements
- variable sampling frequency
- scheduled acquisition
- audio-triggered archiving
- single file / event archiving
- variable pre-trigger
- startup menu on demand
- logging of environmental data (temperature, pressure, humidity, lux)

## requires following non-standard library (in private Arduino/libraries folder)
- SdFs   (Bill Greiman's uSD filing system: https://github.com/greiman/SdFs)

## optional non-standard libraries(in private Arduino/libraries folder) for environmental logging
- BME280 (temperature,pressure, humidity sensor: https://github.com/bolderflight/BME280)
- BH1750 (light sensor: https://github.com/claws/BH1750)

## caveat
this is still in development

## Examples
this directory contains examples recorded during testing and in the field

## DataSheets
this directory contains useful data sheets

## planned
- data offload option

