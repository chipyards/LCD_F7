#include "options.h"
#ifdef USE_AUDIO
/**
  ******************************************************************************
  * @file    wm8994.c
  * @author  MCD Application Team
  * @version V2.0.0
  * @date    24-June-2015
  * @brief   This file provides the WM8994 Audio Codec driver.   
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "chip_wm8994.h"

#define AUDIO_I2C_ADDRESS                ((uint16_t)0x34)

/* Uncomment this line to enable verifying data sent to codec after each write 
   operation (for debug purpose) */
#if !defined (VERIFY_WRITTENDATA)  
// #define VERIFY_WRITTENDATA */
#endif /* VERIFY_WRITTENDATA */


/** @defgroup WM8994_Private_Functions
  * @{
  */ 
/**
  * @brief  Writes/Read a single data.
  * @param  Reg: Reg address 
  * @param  Value: Data to be written
  * @retval None
  */
static uint8_t CODEC_IO_Write( uint16_t Reg, uint16_t Value)
{
  uint32_t result = 0;
  
 AUDIO_IO_Write(AUDIO_I2C_ADDRESS, Reg, Value);
  
#ifdef VERIFY_WRITTENDATA
  /* Verify that the data has been correctly written */
if	( AUDIO_IO_Read(AUDIO_I2C_ADDRESS, Reg) == Value )	
	result = 0;
else	result = 1;
#endif /* VERIFY_WRITTENDATA */
  
  return result;
}


/**
  * @brief Initializes the audio codec and the control interface.
  * @param DeviceAddr: Device address on communication Bus.   
  * @param OutputInputDevice: can be OUTPUT_DEVICE_SPEAKER, OUTPUT_DEVICE_HEADPHONE,
  *  OUTPUT_DEVICE_BOTH, OUTPUT_DEVICE_AUTO, INPUT_DEVICE_DIGITAL_MICROPHONE_1,
  *  INPUT_DEVICE_DIGITAL_MICROPHONE_2, INPUT_DEVICE_INPUT_LINE_1 or INPUT_DEVICE_INPUT_LINE_2.
  * @param Volume: Initial volume level (from 0 (Mute) to 100 (Max))
  * @param AudioFreq: Audio Frequency 
  * @retval 0 if correct communication, else wrong communication
  */
uint32_t wm8994_Init(uint16_t OutputInputDevice, uint8_t Volume, uint32_t AudioFreq)
{
  uint32_t counter = 0;
  uint16_t output_device = OutputInputDevice & 0xFF;
  uint16_t input_device = OutputInputDevice & 0xFF00;
  uint16_t power_mgnt_reg_1 = 0;
  
  /* Initialize the Control interface of the Audio Codec */
  AUDIO_IO_Init();
  /* wm8994 Errata Work-Arounds */
  counter += CODEC_IO_Write(0x102, 0x0003);	// R258 inconnu
  counter += CODEC_IO_Write(0x817, 0x0000); // inconnu
  counter += CODEC_IO_Write(0x102, 0x0000); // inconnu
  
  /* Enable VMID soft start (fast), Start-up Bias Current Enabled */
  counter += CODEC_IO_Write(0x39, 0x006C); // R57 AntiPOP
  
  /* Enable bias generator, Enable VMID */
  counter += CODEC_IO_Write(0x01, 0x0003); // R1 Power
  
  /* Add Delay */
  AUDIO_IO_Delay(50);

  /* Path Configurations for output */
  if (output_device > 0)
  {
    switch (output_device)
    {
    case OUTPUT_DEVICE_SPEAKER:
      /* Enable DAC1 (Left), Enable DAC1 (Right),
      Disable DAC2 (Left), Disable DAC2 (Right)*/
      counter += CODEC_IO_Write(0x05, 0x0C0C);

      /* Enable the AIF1 Timeslot 0 (Left) to DAC 1 (Left) mixer path */
      counter += CODEC_IO_Write(0x601, 0x0000);

      /* Enable the AIF1 Timeslot 0 (Right) to DAC 1 (Right) mixer path */
      counter += CODEC_IO_Write(0x602, 0x0000);

      /* Disable the AIF1 Timeslot 1 (Left) to DAC 2 (Left) mixer path */
      counter += CODEC_IO_Write(0x604, 0x0002);

      /* Disable the AIF1 Timeslot 1 (Right) to DAC 2 (Right) mixer path */
      counter += CODEC_IO_Write(0x605, 0x0002);
      break;

    case OUTPUT_DEVICE_HEADPHONE:
      /* Disable DAC1 (Left), Disable DAC1 (Right),
      Enable DAC2 (Left), Enable DAC2 (Right)*/
      counter += CODEC_IO_Write(0x05, 0x0303);	// R5 Power

      /* Enable the AIF1 Timeslot 0 (Left) to DAC 1 (Left) mixer path */
      counter += CODEC_IO_Write(0x601, 0x0001);	// R1537

      /* Enable the AIF1 Timeslot 0 (Right) to DAC 1 (Right) mixer path */
      counter += CODEC_IO_Write(0x602, 0x0001);	// R1538

      /* Disable the AIF1 Timeslot 1 (Left) to DAC 2 (Left) mixer path */
      counter += CODEC_IO_Write(0x604, 0x0000);	// R1540

      /* Disable the AIF1 Timeslot 1 (Right) to DAC 2 (Right) mixer path */
      counter += CODEC_IO_Write(0x605, 0x0000);	// R1541
      break;

    case OUTPUT_DEVICE_BOTH:
      /* Enable DAC1 (Left), Enable DAC1 (Right),
      also Enable DAC2 (Left), Enable DAC2 (Right)*/
      counter += CODEC_IO_Write(0x05, 0x0303 | 0x0C0C);	// R5 Power

      /* Enable the AIF1 Timeslot 0 (Left) to DAC 1 (Left) mixer path */
      counter += CODEC_IO_Write(0x601, 0x0001);

      /* Enable the AIF1 Timeslot 0 (Right) to DAC 1 (Right) mixer path */
      counter += CODEC_IO_Write(0x602, 0x0001);

      /* Enable the AIF1 Timeslot 1 (Left) to DAC 2 (Left) mixer path */
      counter += CODEC_IO_Write(0x604, 0x0002);	// R1540

      /* Enable the AIF1 Timeslot 1 (Right) to DAC 2 (Right) mixer path */
      counter += CODEC_IO_Write(0x605, 0x0002);	// R1541
      break;
    }
  }

  /* Path Configurations for input */
  if (input_device > 0)
  {
    switch (input_device)
    {
    case INPUT_DEVICE_DIGITAL_MICROPHONE_2 :
      /* Enable AIF1ADC2 (Left), Enable AIF1ADC2 (Right)
       * Enable DMICDAT2 (Left), Enable DMICDAT2 (Right)
       * Enable Left ADC, Enable Right ADC */
      counter += CODEC_IO_Write(0x04, 0x0C30);	// R4 Power

      /* Enable AIF1 DRC2 Signal Detect & DRC in AIF1ADC2 Left/Right Timeslot 1 */
      counter += CODEC_IO_Write(0x450, 0x00DB);	// R1104 DRC2
      // counter += CODEC_IO_Write(0x450, 0);		// R1104 DRC2

      /* Disable IN1L, IN1R, IN2L, IN2R, Enable Thermal sensor & shutdown */
      counter += CODEC_IO_Write(0x02, 0x6000);	// R2 Power

      /* Enable the DMIC2(Left) to AIF1 Timeslot 1 (Left) mixer path */
      counter += CODEC_IO_Write(0x608, 0x0002);	// R1544

      /* Enable the DMIC2(Right) to AIF1 Timeslot 1 (Right) mixer path */
      counter += CODEC_IO_Write(0x609, 0x0002);	// R1545

      /* GPIO1 pin configuration GP1_DIR = output, GP1_FN = AIF1 DRC2 signal detect */
      counter += CODEC_IO_Write(0x700, 0x000E);	// R1792 GPIO 1
      break;

    case INPUT_DEVICE_INPUT_LINE_1 :
      /* Enable AIF1ADC1 (Left), Enable AIF1ADC1 (Right)
       * Enable Left ADC, Enable Right ADC */
      counter += CODEC_IO_Write(0x04, 0x0303);	// R4 Power

      /* Enable AIF1 DRC1 Signal Detect & DRC in AIF1ADC1 Left/Right Timeslot 0 */
      counter += CODEC_IO_Write(0x440, 0x00DB);	// R1088 DRC1
      // counter += CODEC_IO_Write(0x440, 0);		// R1088 DRC1

      /* Enable IN1L and IN1R, Disable IN2L and IN2R, Enable Thermal sensor & shutdown */
      counter += CODEC_IO_Write(0x02, 0x6350);	// R2 Power

      /* Enable the ADCL(Left) to AIF1 Timeslot 0 (Left) mixer path */
      counter += CODEC_IO_Write(0x606, 0x0002);	// R1542

      /* Enable the ADCR(Right) to AIF1 Timeslot 0 (Right) mixer path */
      counter += CODEC_IO_Write(0x607, 0x0002);	// R1543

      /* GPIO1 pin configuration GP1_DIR = output, GP1_FN = AIF1 DRC1 signal detect */
      counter += CODEC_IO_Write(0x700, 0x000D);	// R1792 GPIO 1
      break;

    case INPUT_DEVICE_DIGITAL_MICROPHONE_1 :
    case INPUT_DEVICE_INPUT_LINE_2 :
    default:
      /* Actually, no other input devices supported */
      counter++;
      break;
    }
  }

  
  /*  Clock Configurations */
  switch (AudioFreq)
  {
  case  AUDIO_FREQUENCY_8K:
    /* AIF1 Sample Rate = 8 (KHz), ratio=256 */ 
    counter += CODEC_IO_Write(0x210, 0x0003);	// R528 AIF1 rate
    break;
    
  case  AUDIO_FREQUENCY_16K:
    /* AIF1 Sample Rate = 16 (KHz), ratio=256 */ 
    counter += CODEC_IO_Write(0x210, 0x0033);
    break;
    
  case  AUDIO_FREQUENCY_48K:
    /* AIF1 Sample Rate = 48 (KHz), ratio=256 */ 
    counter += CODEC_IO_Write(0x210, 0x0083);
    break;
    
  case  AUDIO_FREQUENCY_96K:
    /* AIF1 Sample Rate = 96 (KHz), ratio=256 */ 
    counter += CODEC_IO_Write(0x210, 0x00A3);
    break;
    
  case  AUDIO_FREQUENCY_11K:
    /* AIF1 Sample Rate = 11.025 (KHz), ratio=256 */ 
    counter += CODEC_IO_Write(0x210, 0x0013);
    break;
    
  case  AUDIO_FREQUENCY_22K:
    /* AIF1 Sample Rate = 22.050 (KHz), ratio=256 */ 
    counter += CODEC_IO_Write(0x210, 0x0043);
    break;
    
  case  AUDIO_FREQUENCY_44K:
    /* AIF1 Sample Rate = 44.1 (KHz), ratio=256 */ 
    counter += CODEC_IO_Write(0x210, 0x0073);
    break; 
    
  default:
    /* AIF1 Sample Rate = 48 (KHz), ratio=256 */ 
    counter += CODEC_IO_Write(0x210, 0x0083);
    break; 
  }
  /* AIF1 Word Length = 16-bits, AIF1 Format = I2S (Default Register Value) */
  counter += CODEC_IO_Write(0x300, 0x4010);		// R768 AIF1 Ctrl
  
  /* slave mode */
  counter += CODEC_IO_Write(0x302, 0x0000);		// R770 AIF1 Master/Slave
  
  /* Enable the DSP processing clock for AIF1, Enable the core clock */
  counter += CODEC_IO_Write(0x208, 0x000A);		// R520 Clocking
  
  /* Enable AIF1 Clock, AIF1 Clock Source = MCLK1 pin */
  counter += CODEC_IO_Write(0x200, 0x0001);		// R512 AIF1 clocking

  if (output_device > 0)  /* Audio output selected */
  {
    /* Analog Output Configuration */

    /* Enable SPKRVOL PGA, Enable SPKMIXR, Enable SPKLVOL PGA, Enable SPKMIXL */
    counter += CODEC_IO_Write(0x03, 0x0300);	// R3 Power

    /* Left Speaker Mixer Volume = 0dB */
    counter += CODEC_IO_Write(0x22, 0x0000);	// R34

    /* Speaker output mode = Class D, Right Speaker Mixer Volume = 0dB ((0x23, 0x0100) = class AB)*/
    counter += CODEC_IO_Write(0x23, 0x0000);	// R35

    /* Unmute DAC2 (Left) to Left Speaker Mixer (SPKMIXL) path,
    Unmute DAC2 (Right) to Right Speaker Mixer (SPKMIXR) path */
    counter += CODEC_IO_Write(0x36, 0x0300);	// R54 SPKR Mixer

    /* Enable bias generator, Enable VMID, Enable SPKOUTL, Enable SPKOUTR */
    counter += CODEC_IO_Write(0x01, 0x3003);	// R1 Power

    /* Headphone/Speaker Enable */

    /* Enable Class W, Class W Envelope Tracking = AIF1 Timeslot 0 */
    counter += CODEC_IO_Write(0x51, 0x0005);	// R81 Class W

    /* Enable bias generator, Enable VMID, Enable HPOUT1 (Left) and Enable HPOUT1 (Right) input stages */
    /* idem for Speaker */
    power_mgnt_reg_1 |= 0x0303 | 0x3003;
    counter += CODEC_IO_Write(0x01, power_mgnt_reg_1);	// R1 Power

    /* Enable HPOUT1 (Left) and HPOUT1 (Right) intermediate stages */
    counter += CODEC_IO_Write(0x60, 0x0022);		// R96 Analog HP
									// premiere etape
    /* Enable Charge Pump */
    counter += CODEC_IO_Write(0x4C, 0x9F25);		// R76 Charge Pump
									// pourquoi 9F25, lol
    /* Add Delay */
    AUDIO_IO_Delay(15);

    /* Select DAC1 (Left) to Left Headphone Output PGA (HPOUT1LVOL) path */
    counter += CODEC_IO_Write(0x2D, 0x0001);		// R45 Ouput Mixer
									// 0x0100 pour Direct
    /* Select DAC1 (Right) to Right Headphone Output PGA (HPOUT1RVOL) path */
    counter += CODEC_IO_Write(0x2E, 0x0001);		// R46
									// 0x0100 pour Direct
    /* Enable Left Output Mixer (MIXOUTL), Enable Right Output Mixer (MIXOUTR) */
    /* idem for SPKOUTL and SPKOUTR */
    counter += CODEC_IO_Write(0x03, 0x0030 | 0x0300);	// R3 Power

    /* Enable DC Servo and trigger start-up mode on left and right channels */
    counter += CODEC_IO_Write(0x54, 0x0033);	// R84 DC Servo

    /* Add Delay */
    AUDIO_IO_Delay(250);

    /* Enable HPOUT1 (Left) and HPOUT1 (Right) intermediate and output stages. Remove clamps */
    counter += CODEC_IO_Write(0x60, 0x00EE);	// R96 Analog HP
								// seconde etape
    /* Unmutes et volumes digitaux a 0dB */

    /* Unmute DAC 1 (Left) */
    counter += CODEC_IO_Write(0x610, 0x00C0);	// R1552 DAC1L volume
								// manque le bit VU
    /* Unmute DAC 1 (Right) */
    counter += CODEC_IO_Write(0x611, 0x00C0);	// R1553 DAC1R volume
								// manque le bit VU
    /* Unmute the AIF1 Timeslot 0 DAC path */
    counter += CODEC_IO_Write(0x420, 0x0000);	// R1056 AIF1 DAC1 filters
								// unmute
    /* Unmute DAC 2 (Left) */
    counter += CODEC_IO_Write(0x612, 0x00C0);	// R1554 DAC2L volume
								// manque le bit VU
    /* Unmute DAC 2 (Right) */
    counter += CODEC_IO_Write(0x613, 0x00C0);	// R1555 DAC2R volume
								// manque le bit VU
    /* Unmute the AIF1 Timeslot 1 DAC2 path */
    counter += CODEC_IO_Write(0x422, 0x0000);	// R1058 AIF1 DAC2 filters
								// unmute
    /* Volume Control analog */
    wm8994_Set_out_Volume( Volume );
  }

  if (input_device > 0) /* Audio input selected */
  {
    if ((input_device == INPUT_DEVICE_DIGITAL_MICROPHONE_1) || (input_device == INPUT_DEVICE_DIGITAL_MICROPHONE_2))
    {
      /* Enable Microphone bias 1 generator, Enable VMID */
      power_mgnt_reg_1 |= 0x0013;
      counter += CODEC_IO_Write(0x01, power_mgnt_reg_1);	// R1 Power

      /* ADC oversample enable */
      counter += CODEC_IO_Write(0x620, 0x0002);		// R1568 OSR128 "HiFi" ADC=2, DAC=1

      /* AIF ADC2 HPF enable, HPF cut = voice mode 1 fc=127Hz at fs=8kHz */
      // counter += CODEC_IO_Write(0x411, 0x3800);	// R1041 AIF1 DMIC2 filters
      /* AIF ADC2 HPF enable, HPF cut = hifi mode fc=4Hz at fs=48kHz JLN modif */
      counter += CODEC_IO_Write(0x411, 0x1800);		// R1041 AIF1 DMIC2 filters
    }
    else if ((input_device == INPUT_DEVICE_INPUT_LINE_1) || (input_device == INPUT_DEVICE_INPUT_LINE_2))
    {
      /* Enable normal bias generator, Enable VMID */
      power_mgnt_reg_1 |= 0x0003;
      counter += CODEC_IO_Write(0x01, power_mgnt_reg_1);	// R1 Power

      wm8994_Set_line_in_Volume( 11, 11 );		// 0dB

      /* IN1LN_TO_IN1L, IN1LP_TO_VMID, IN1RN_TO_IN1R, IN1RP_TO_VMID */
      counter += CODEC_IO_Write(0x28, 0x0011);	// R40 analog mix select
						// IN1 Single end line in = 0011

      /* AIF ADC1 HPF enable, HPF cut = hifi mode fc=4Hz at fs=48kHz */
      // counter += CODEC_IO_Write(0x410, 0x1800);	// R1040 AIF1 ADC1 filters
      /* AIF ADC1 HPF disable JLN modif */
      counter += CODEC_IO_Write(0x410, 0x0);		// R1040 AIF1 ADC1 filters
    }
  }
  /* Return communication control value */
  return counter;  
}

/**
  * @brief  Deinitializes the audio codec.
  * @param  None
  * @retval  None
  */
void wm8994_DeInit(void)
{
  /* Deinitialize Audio Codec interface */
  AUDIO_IO_DeInit();
}

/**
  * @brief  Get the WM8994 ID.
  * @param DeviceAddr: Device address on communication Bus.
  * @retval The WM8994 ID 
  */
uint32_t wm8994_ReadID(void)
{
  /* Initialize the Control interface of the Audio Codec */
  AUDIO_IO_Init();

  return ((uint32_t)AUDIO_IO_Read(AUDIO_I2C_ADDRESS, WM8994_CHIPID_ADDR));
}

/**
  * @brief Start the audio Codec play feature.
  * @note For this codec no Play options are required.
  * @param DeviceAddr: Device address on communication Bus.   
  * @retval 0 if correct communication, else wrong communication
  */
uint32_t wm8994_Play(uint16_t* pBuffer, uint16_t Size)
{
  uint32_t counter = 0;
 
  /* Resumes the audio file playing */  
  /* Unmute the output first */
  counter += wm8994_SetMute(AUDIO_MUTE_OFF);
  
  return counter;
}

/**
  * @brief Pauses playing on the audio codec.
  * @param DeviceAddr: Device address on communication Bus. 
  * @retval 0 if correct communication, else wrong communication
  */
uint32_t wm8994_Pause()
{  
  uint32_t counter = 0;
 
  /* Pause the audio file playing */
  /* Mute the output first */
  counter += wm8994_SetMute(AUDIO_MUTE_ON);
  
  /* Put the Codec in Power save mode */
  counter += CODEC_IO_Write(0x02, 0x01);
 
  return counter;
}

/**
  * @brief Resumes playing on the audio codec.
  * @param DeviceAddr: Device address on communication Bus. 
  * @retval 0 if correct communication, else wrong communication
  */
uint32_t wm8994_Resume()
{
  uint32_t counter = 0;
 
  /* Resumes the audio file playing */  
  /* Unmute the output first */
  counter += wm8994_SetMute(AUDIO_MUTE_OFF);
  
  return counter;
}

/**
  * @brief Stops audio Codec playing. It powers down the codec.
  * @param DeviceAddr: Device address on communication Bus. 
  * @param CodecPdwnMode: selects the  power down mode.
  *          - CODEC_PDWN_SW: only mutes the audio codec. When resuming from this 
  *                           mode the codec keeps the previous initialization
  *                           (no need to re-Initialize the codec registers).
  *          - CODEC_PDWN_HW: Physically power down the codec. When resuming from this
  *                           mode, the codec is set to default configuration 
  *                           (user should re-Initialize the codec in order to 
  *                            play again the audio stream).
  * @retval 0 if correct communication, else wrong communication
  */
uint32_t wm8994_Stop(uint32_t CodecPdwnMode)
{
  uint32_t counter = 0;

    /* Mute the output first */
    counter += wm8994_SetMute(AUDIO_MUTE_ON);

    if (CodecPdwnMode == CODEC_PDWN_SW)
    {
       /* Only output mute required*/
    }
    else /* CODEC_PDWN_HW */
    {
      /* Mute the AIF1 Timeslot 0 DAC1 path */
      counter += CODEC_IO_Write(0x420, 0x0200);

      /* Mute the AIF1 Timeslot 1 DAC2 path */
      counter += CODEC_IO_Write(0x422, 0x0200);

      /* Disable DAC1L_TO_HPOUT1L */
      counter += CODEC_IO_Write(0x2D, 0x0000);

      /* Disable DAC1R_TO_HPOUT1R */
      counter += CODEC_IO_Write(0x2E, 0x0000);

      /* Disable DAC1 and DAC2 */
      counter += CODEC_IO_Write(0x05, 0x0000);

      /* Reset Codec by writing in 0x0000 address register */
      counter += CODEC_IO_Write(0x0000, 0x0000);
    }

  return counter;
}

// digital volume for the DMICs - because no analog control exists !
// 0.375 dB/step, 0 = mute, FS = 239
uint32_t wm8994_Set_DMIC_Volume( unsigned int vol )
{
uint32_t counter = 0;
if	( vol > 239 )
	vol = 239;
/* Left AIF1 ADC2 volume */
counter += CODEC_IO_Write( 0x404, vol | 0x100);		// R1028 DMIC2 vol.
							// VU bit = 100, FS = EF
/* Right AIF1 ADC2 volume */
counter += CODEC_IO_Write( 0x405, vol | 0x100);		// R1029 DMIC2 vol.
return counter;
}

// analog line in volume control	// JLN modif : separer L et R
// 1.5 dB/step, 0dB = 11, full scale 51
uint32_t wm8994_Set_line_in_Volume( unsigned int Lvol, unsigned int Rvol )
{
uint32_t counter = 0;
int Lboost30, Rboost30;
// on met le vol digital à 0dB
/* Left AIF1 ADC1 volume */
counter += CODEC_IO_Write( 0x400, 0xC0 | 0x100 );		// R1024 ADC1 to AIF1 vol.
								// VU bit = 100, 0dB = C0
/* Right AIF1 ADC1 volume */
counter += CODEC_IO_Write( 0x401, 0xC0 | 0x100 );		// R1025 ADC1 to AIF1 vol.

// on ajuste le vol analog
//	vol	0	31	32	51
//	R24	0	31	12	31
//	boost	0	 0	1	1
//	dB	-16.5	+30	+31.5	+60
if	( Lvol > 51 )	Lvol = 51;
if	( Lvol > 31 )	{ Lboost30 = 0x10; Lvol -= 20; } else Lboost30 = 0;
if	( Rvol > 51 )	Rvol = 51;
if	( Rvol > 31 )	{ Rboost30 = 0x10; Rvol -= 20; } else Rboost30 = 0;

counter += CODEC_IO_Write( 0x18, Lvol | 0x100 );		// R24 IN1 L analog vol
							// VU bit = 100, 0dB = 0B, FS = 1F
counter += CODEC_IO_Write( 0x1A, Rvol | 0x100 );		// R26 IN1 R analog vol

counter += CODEC_IO_Write( 0x29, 0x20 | Lboost30 );	// R41 MIXINL analog mix
						// IN1L = 0020, 30dB boost = 0010
counter += CODEC_IO_Write( 0x2A, 0x20 | Rboost30 );	// R42 MIXINL analog mix
						// IN1R = 0020, 30dB boost = 0010

return counter;
}

// analog output volume, full-scale 63
uint32_t wm8994_Set_out_Volume( unsigned int vol )
{
uint32_t counter = 0;
if	( vol > 63 )
	vol = 63;
/* Unmute audio codec */
counter += wm8994_SetMute(AUDIO_MUTE_OFF);

/* Left Headphone Volume */
counter += CODEC_IO_Write(0x1C, vol | 0x140);	// R28 HP analog volume
						// VU bit = 100, unmute = 40, FS = 3F 
/* Right Headphone Volume */
counter += CODEC_IO_Write(0x1D, vol | 0x140);	// R29

/* Left Speaker Volume */
counter += CODEC_IO_Write(0x26, vol | 0x140);	// R38 SPKR analog volume

/* Right Speaker Volume */
counter += CODEC_IO_Write(0x27, vol | 0x140);	// R39

return counter;
}




/**
  * @brief Enables or disables the mute feature on the audio codec.
  * @param DeviceAddr: Device address on communication Bus.   
  * @param Cmd: AUDIO_MUTE_ON to enable the mute or AUDIO_MUTE_OFF to disable the
  *             mute mode.
  * @retval 0 if correct communication, else wrong communication
  */
uint32_t wm8994_SetMute(uint32_t Cmd)
{
  uint32_t counter = 0;
  
    /* Set the Mute mode */
    if(Cmd == AUDIO_MUTE_ON)
    {
      /* Soft Mute the AIF1 Timeslot 0 DAC1 path L&R */
      counter += CODEC_IO_Write(0x420, 0x0200);		// R1056: Mute DAC1

      /* Soft Mute the AIF1 Timeslot 1 DAC2 path L&R */
      counter += CODEC_IO_Write(0x422, 0x0200);		// R1058: Mute DAC2
    }
    else /* AUDIO_MUTE_OFF Disable the Mute */
    {
      /* Unmute the AIF1 Timeslot 0 DAC1 path L&R */
      counter += CODEC_IO_Write(0x420, 0x0000);

      /* Unmute the AIF1 Timeslot 1 DAC2 path L&R */
      counter += CODEC_IO_Write(0x422, 0x0000);
    }
  return counter;
}

/**
  * @brief Switch dynamically (while audio file is played) the output target 
  *         (speaker or headphone).
  * @param DeviceAddr: Device address on communication Bus.
  * @param Output: specifies the audio output target: OUTPUT_DEVICE_SPEAKER,
  *         OUTPUT_DEVICE_HEADPHONE, OUTPUT_DEVICE_BOTH or OUTPUT_DEVICE_AUTO 
  * @retval 0 if correct communication, else wrong communication
  */
uint32_t wm8994_SetOutputMode(uint8_t Output)
{
  uint32_t counter = 0; 
  
  switch (Output) 
  {
  case OUTPUT_DEVICE_SPEAKER:
    /* Enable DAC1 (Left), Enable DAC1 (Right), 
    Disable DAC2 (Left), Disable DAC2 (Right)*/
    counter += CODEC_IO_Write(0x05, 0x0C0C);
    
    /* Enable the AIF1 Timeslot 0 (Left) to DAC 1 (Left) mixer path */
    counter += CODEC_IO_Write(0x601, 0x0000);
    
    /* Enable the AIF1 Timeslot 0 (Right) to DAC 1 (Right) mixer path */
    counter += CODEC_IO_Write(0x602, 0x0000);
    
    /* Disable the AIF1 Timeslot 1 (Left) to DAC 2 (Left) mixer path */
    counter += CODEC_IO_Write(0x604, 0x0002);
    
    /* Disable the AIF1 Timeslot 1 (Right) to DAC 2 (Right) mixer path */
    counter += CODEC_IO_Write(0x605, 0x0002);
    break;
    
  case OUTPUT_DEVICE_HEADPHONE:
    /* Disable DAC1 (Left), Disable DAC1 (Right), 
    Enable DAC2 (Left), Enable DAC2 (Right)*/
    counter += CODEC_IO_Write(0x05, 0x0303);
    
    /* Enable the AIF1 Timeslot 0 (Left) to DAC 1 (Left) mixer path */
    counter += CODEC_IO_Write(0x601, 0x0001);
    
    /* Enable the AIF1 Timeslot 0 (Right) to DAC 1 (Right) mixer path */
    counter += CODEC_IO_Write(0x602, 0x0001);
    
    /* Disable the AIF1 Timeslot 1 (Left) to DAC 2 (Left) mixer path */
    counter += CODEC_IO_Write(0x604, 0x0000);
    
    /* Disable the AIF1 Timeslot 1 (Right) to DAC 2 (Right) mixer path */
    counter += CODEC_IO_Write(0x605, 0x0000);
    break;
    
  case OUTPUT_DEVICE_BOTH:
    /* Enable DAC1 (Left), Enable DAC1 (Right), 
    also Enable DAC2 (Left), Enable DAC2 (Right)*/
    counter += CODEC_IO_Write(0x05, 0x0303 | 0x0C0C);
    
    /* Enable the AIF1 Timeslot 0 (Left) to DAC 1 (Left) mixer path */
    counter += CODEC_IO_Write(0x601, 0x0001);
    
    /* Enable the AIF1 Timeslot 0 (Right) to DAC 1 (Right) mixer path */
    counter += CODEC_IO_Write(0x602, 0x0001);
    
    /* Enable the AIF1 Timeslot 1 (Left) to DAC 2 (Left) mixer path */
    counter += CODEC_IO_Write(0x604, 0x0002);
    
    /* Enable the AIF1 Timeslot 1 (Right) to DAC 2 (Right) mixer path */
    counter += CODEC_IO_Write(0x605, 0x0002);
    break;
    
  default:
    /* Disable DAC1 (Left), Disable DAC1 (Right), 
    Enable DAC2 (Left), Enable DAC2 (Right)*/
    counter += CODEC_IO_Write(0x05, 0x0303);
    
    /* Enable the AIF1 Timeslot 0 (Left) to DAC 1 (Left) mixer path */
    counter += CODEC_IO_Write(0x601, 0x0001);
    
    /* Enable the AIF1 Timeslot 0 (Right) to DAC 1 (Right) mixer path */
    counter += CODEC_IO_Write(0x602, 0x0001);
    
    /* Disable the AIF1 Timeslot 1 (Left) to DAC 2 (Left) mixer path */
    counter += CODEC_IO_Write(0x604, 0x0000);
    
    /* Disable the AIF1 Timeslot 1 (Right) to DAC 2 (Right) mixer path */
    counter += CODEC_IO_Write(0x605, 0x0000);
    break;    
  }  
  return counter;
}

/**
  * @brief Sets new frequency.
  * @param DeviceAddr: Device address on communication Bus.
  * @param AudioFreq: Audio frequency used to play the audio stream.
  * @retval 0 if correct communication, else wrong communication
  */
uint32_t wm8994_SetFrequency(uint32_t AudioFreq)
{
  uint32_t counter = 0;
 
  /*  Clock Configurations */
  switch (AudioFreq)
  {
  case  AUDIO_FREQUENCY_8K:
    /* AIF1 Sample Rate = 8 (KHz), ratio=256 */ 
    counter += CODEC_IO_Write(0x210, 0x0003);
    break;
    
  case  AUDIO_FREQUENCY_16K:
    /* AIF1 Sample Rate = 16 (KHz), ratio=256 */ 
    counter += CODEC_IO_Write(0x210, 0x0033);
    break;
    
  case  AUDIO_FREQUENCY_48K:
    /* AIF1 Sample Rate = 48 (KHz), ratio=256 */ 
    counter += CODEC_IO_Write(0x210, 0x0083);
    break;
    
  case  AUDIO_FREQUENCY_96K:
    /* AIF1 Sample Rate = 96 (KHz), ratio=256 */ 
    counter += CODEC_IO_Write(0x210, 0x00A3);
    break;
    
  case  AUDIO_FREQUENCY_11K:
    /* AIF1 Sample Rate = 11.025 (KHz), ratio=256 */ 
    counter += CODEC_IO_Write(0x210, 0x0013);
    break;
    
  case  AUDIO_FREQUENCY_22K:
    /* AIF1 Sample Rate = 22.050 (KHz), ratio=256 */ 
    counter += CODEC_IO_Write(0x210, 0x0043);
    break;
    
  case  AUDIO_FREQUENCY_44K:
    /* AIF1 Sample Rate = 44.1 (KHz), ratio=256 */ 
    counter += CODEC_IO_Write(0x210, 0x0073);
    break; 
    
  default:
    /* AIF1 Sample Rate = 48 (KHz), ratio=256 */ 
    counter += CODEC_IO_Write(0x210, 0x0083);
    break; 
  }
  return counter;
}

/**
  * @brief Resets wm8994 registers.
  * @param DeviceAddr: Device address on communication Bus. 
  * @retval 0 if correct communication, else wrong communication
  */
uint32_t wm8994_Reset()
{
  uint32_t counter = 0;
  
  /* Reset Codec by writing in 0x0000 address register */
  counter = CODEC_IO_Write(0x0000, 0x0000);

  return counter;
}
#endif
