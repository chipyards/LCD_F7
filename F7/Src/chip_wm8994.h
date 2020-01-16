/**
  * @file    wm8994.h
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __WM8994_H
#define __WM8994_H

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

// ex audio.h
/* Codec audio Standards */
#define CODEC_STANDARD                0x04
#define I2S_STANDARD                  I2S_STANDARD_PHILIPS


/******************************************************************************/
/***************************  Codec User defines ******************************/
/******************************************************************************/
/* Codec output DEVICE */
#define OUTPUT_DEVICE_SPEAKER                 ((uint16_t)0x0001)
#define OUTPUT_DEVICE_HEADPHONE               ((uint16_t)0x0002)
#define OUTPUT_DEVICE_BOTH                    ((uint16_t)0x0003)
#define OUTPUT_DEVICE_AUTO                    ((uint16_t)0x0004)
#define INPUT_DEVICE_DIGITAL_MICROPHONE_1     ((uint16_t)0x0100)
#define INPUT_DEVICE_DIGITAL_MICROPHONE_2     ((uint16_t)0x0200)
#define INPUT_DEVICE_INPUT_LINE_1             ((uint16_t)0x0300)
#define INPUT_DEVICE_INPUT_LINE_2             ((uint16_t)0x0400)

/* Volume Levels values */
#define DEFAULT_VOLMIN                0x00
#define DEFAULT_VOLMAX                0xFF
#define DEFAULT_VOLSTEP               0x04

#define AUDIO_PAUSE                   0
#define AUDIO_RESUME                  1

/* Codec POWER DOWN modes */
#define CODEC_PDWN_HW                 1
#define CODEC_PDWN_SW                 2

/* MUTE commands */
#define AUDIO_MUTE_ON                 1
#define AUDIO_MUTE_OFF                0

/* AUDIO FREQUENCY */
#define AUDIO_FREQUENCY_192K          ((uint32_t)192000)
#define AUDIO_FREQUENCY_96K           ((uint32_t)96000)
#define AUDIO_FREQUENCY_48K           ((uint32_t)48000)
#define AUDIO_FREQUENCY_44K           ((uint32_t)44100)
#define AUDIO_FREQUENCY_32K           ((uint32_t)32000)
#define AUDIO_FREQUENCY_22K           ((uint32_t)22050)
#define AUDIO_FREQUENCY_16K           ((uint32_t)16000)
#define AUDIO_FREQUENCY_11K           ((uint32_t)11025)
#define AUDIO_FREQUENCY_8K            ((uint32_t)8000)  

#define VOLUME_CONVERT(Volume)        (((Volume) > 100)? 100:((uint8_t)(((Volume) * 63) / 100)))
#define VOLUME_IN_CONVERT(Volume)     (((Volume) >= 100)? 239:((uint8_t)(((Volume) * 240) / 100)))

/******************************************************************************/
/****************************** REGISTER MAPPING ******************************/
/******************************************************************************/
/** 
  * @brief  WM8994 ID  
  */  
#define  WM8994_ID    0x8994

/**
  * @brief Device ID Register: Reading from this register will indicate device 
  *                            family ID 8994h
  */
#define WM8994_CHIPID_ADDR                  0x00

/**
  * @}
  */ 

/** @defgroup WM8994_Exported_Macros
  * @{
  */ 
/**
  * @}
  */ 

/** @defgroup WM8994_Exported_Functions
  * @{
  */
    
/*------------------------------------------------------------------------------
                           Audio Codec functions 
------------------------------------------------------------------------------*/
/* High Layer codec functions */
uint32_t wm8994_Init(uint16_t OutputInputDevice, uint8_t Volume, uint32_t AudioFreq);
void     wm8994_DeInit(void);
uint32_t wm8994_ReadID(void);
uint32_t wm8994_Play(uint16_t* pBuffer, uint16_t Size);
uint32_t wm8994_Pause(void);
uint32_t wm8994_Resume(void);
uint32_t wm8994_Stop(uint32_t Cmd);
// uint32_t wm8994_SetVolume(uint8_t Volume);
uint32_t wm8994_Set_DMIC_Volume( unsigned int vol );
uint32_t wm8994_Set_out_Volume( unsigned int vol );
uint32_t wm8994_Set_line_in_Volume( unsigned int Lvol, unsigned int Rvol );
uint32_t wm8994_SetMute(uint32_t Cmd);
uint32_t wm8994_SetOutputMode(uint8_t Output);
uint32_t wm8994_SetFrequency(uint32_t AudioFreq);
uint32_t wm8994_Reset(void);

/* AUDIO IO functions */
void    AUDIO_IO_Init(void);
void    AUDIO_IO_DeInit(void);
void    AUDIO_IO_Write(uint8_t Addr, uint16_t Reg, uint16_t Value);
uint8_t AUDIO_IO_Read(uint8_t Addr, uint16_t Reg);
void    AUDIO_IO_Delay(uint32_t Delay);

#endif /* __WM8994_H */

/**
  * @}
  */ 

/**
  * @}
  */ 

/**
  * @}
  */

/**
  * @}
  */ 

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
