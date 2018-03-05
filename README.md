

# BasicAudioLogger
Basic framework for logging audio to uSD on T3.6 (T3.5 possible but not tested)

## supports 
### buid-in 
- ADC, ADC_Stereo
- I2S, I2S_32, I2S_QUAD 

The I2S_32 mode allows an improved acquisiton of 24 bit MEMS microphones

## implements
- variable sampling frequency
- scheduled acquisition
- audio-triggered archiving
- single file / event archiving
- variable pre-trigger
