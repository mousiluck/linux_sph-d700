/*
 * wm8994_aries.c  --  WM8994 ALSA Soc Audio driver related Aries
 *
 *  Copyright (C) 2010 Samsung Electronics.
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <asm/gpio.h> 
#include <plat/gpio-cfg.h> 
#include <plat/map-base.h>
#include <plat/regs-clock.h>
#include <mach/gpio-jupiter.h>
#include "wm8994.h"

//------------------------------------------------
//		Debug Feature
//------------------------------------------------
#define SUBJECT "wm8994_universal.c"

#define AUDIO_CODEC_WM8994_DEBUG  1

#if AUDIO_CODEC_WM8994_DEBUG
#define DEBUG_LOG(format,...)\
	printk ("[ "SUBJECT " (%s,%d) ] " format "\n", __func__, __LINE__, ## __VA_ARGS__);
#else
#define DEBUG_LOG(format,...)
#endif

//#define STEREO_SPEAKER_SUPPORT  //Define for Garnett Stereo Speaker.

//------------------------------------------------
// Definitions of tunning volumes for wm8994
//------------------------------------------------
// DAC
#define TUNING_DAC1L_VOL		0xC0		// 610h
#define TUNING_DAC1R_VOL		0xC0		// 611h

// Speaker
#define TUNING_SPKMIXL_ATTEN		0x0		// 22h
#define TUNING_SPKMIXR_ATTEN		0x0		// 23h
#define TUNING_MP3_SPKL_VOL		    0x3B		// 26h
#define TUNING_MP3_SPKR_VOL		    0x3B		// 27h
#define TUNING_MP3_CLASSD_VOL		0x7	// 25h

#define TUNING_FMRADIO_SPKL_VOL		0x3E		// 26h
#define TUNING_FMRADIO_CLASSD_VOL	0x5		// 25h

#define TUNING_CALL_SPKL_VOL		0x3E		// 26h
#define TUNING_CALL_CLASSD_VOL		0x7		// 25h

// Headset
#define TUNING_EAR_OUTMIX5_VOL		0x0		// 31h
#define TUNING_EAR_OUTMIX6_VOL		0x0		// 32h

#define TUNING_MP3_OUTPUTL_VOL		0x34		// 1Ch
#define TUNING_MP3_OUTPUTR_VOL		0x34		// 1Dh
#define TUNING_MP3_OPGAL_VOL		0x39		// 20h
#define TUNING_MP3_OPGAR_VOL		0x39		// 21h

#define TUNING_CALL_OUTPUTL_VOL		0x30		// 1Ch
#define TUNING_CALL_OUTPUTR_VOL		0x30		// 1Dh
#define TUNING_CALL_OPGAL_VOL		0x39		// 20h
#define TUNING_CALL_OPGAR_VOL		0x39		// 21h

#define TUNING_FMRADIO_OUTPUTL_VOL	0x3D		// 1Ch
#define TUNING_FMRADIO_OUTPUTR_VOL	0x3D		// 1Dh
#define TUNING_FMRADIO_OPGAL_VOL	0x39		// 20h
#define TUNING_FMRADIO_OPGAR_VOL	0x39		// 21h

#define TUNING_HPOUTMIX_VOL		0x0

// Dual
#define TUNING_DUAL_OUTPUTL_VOL		0x2E		// 1Ch
#define TUNING_DUAL_OUTPUTR_VOL		0x2E		// 1Dh

// Main MIC
#define TUNING_RECORD_MAIN_INPUTLINE_VOL	0x16		// 18h

// Ear MIC
#define TUNING_RECORD_SUB_INPUTMIX_VOL	0x17		// 1Ah

// Call Main MIC
#define TUNING_CALL_RCV_INPUTMIX_VOL	0x16		// 18h
#define TUNING_CALL_SPK_INPUTMIX_VOL	0x17		// 18h

// Call Ear MIC
#define TUNING_CALL_EAR_INPUTMIX_VOL	0x17		// 1Ah

// Receiver
#define TUNING_RCV_OUTMIX5_VOL		0x0		// 31h
#define TUNING_RCV_OUTMIX6_VOL 		0x0		// 32h
#define TUNING_RCV_OPGAL_VOL		0x3E		// 20h
#define TUNING_RCV_OPGAR_VOL		0x3E		// 21h
#define TUNING_HPOUT2_VOL		0x0		// 1Fh


extern int hw_version_check(void);
extern short int get_headset_status(void);		// For ear-jack control(MIC-Bias)

int audio_init(void)
{

	//CODEC LDO SETTING
	if (gpio_is_valid(GPIO_CODEC_LDO_EN))
	{
		if (gpio_request(GPIO_CODEC_LDO_EN, "CODEC_LDO_EN"))
			DEBUG_LOG_ERR("Failed to request CODEC_LDO_EN! \n");
		gpio_direction_output(GPIO_CODEC_LDO_EN, 0);
	}

	s3c_gpio_setpull(GPIO_CODEC_LDO_EN, S3C_GPIO_PULL_NONE);
	
	// For preserving output of codec related pins.
	s3c_gpio_slp_cfgpin(GPIO_CODEC_LDO_EN, S3C_GPIO_SLP_PREV);

	//CODEC XTAL CLK SETTING
	//b4 : AP Gpio emul, B5 : CODEC_XTAL_EN 
#if (defined CONFIG_JUPITER_VER_B5) || (defined CONFIG_ARIES_VER_B0)
	if (gpio_is_valid(GPIO_CODEC_XTAL_EN)) {
		if (gpio_request(GPIO_CODEC_XTAL_EN, "GPIO_CODEC_XTAL_EN")) 
			DEBUG_LOG_ERR("Failed to request GPIO_CODEC_XTAL_EN! \n");
		
		gpio_direction_output(GPIO_CODEC_XTAL_EN, 0);
	}
	s3c_gpio_setpull(GPIO_CODEC_XTAL_EN, S3C_GPIO_PULL_NONE);
#endif

	s3c_gpio_slp_setpull_updown(GPIO_CODEC_LDO_EN, S3C_GPIO_PULL_NONE);

	// PCM_SEL Setting
#if  (defined CONFIG_JUPITER_VER_B5) || (defined CONFIG_ARIES_VER_B0) || (defined CONFIG_ARIES_VER_B1)
	gpio_direction_output(GPIO_PCM_SEL, 0);
	s3c_gpio_setpull(GPIO_PCM_SEL, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_PCM_SEL, 0);
#endif

#if (defined CONFIG_JUPITER_VER_B5)  || (defined CONFIG_ARIES_VER_B0)
	s3c_gpio_slp_setpull_updown(GPIO_CODEC_XTAL_EN, S3C_GPIO_PULL_NONE);
#endif
		
	return 0;

}

int audio_power(int en)
{
#if (defined CONFIG_JUPITER_VER_B4)
	u32 val;

	DEBUG_LOG("LDO Enable = %d, XTAL CLK From AP EMUL", en);
	val = __raw_readl(S5P_CLK_OUT);
	val &= 0xFFFE0FFF; //clear bits 12~16
#endif
	if(en)
	{
		// Turn on LDO for codec.
		gpio_set_value(GPIO_CODEC_LDO_EN, 1);

		msleep(10);	// Wait for warming up.

		// Turn on master clock.
#if 1
	__raw_writel(__raw_readl(S5P_OTHERS) | (3<<8) , S5P_OTHERS); 
	__raw_writel(__raw_readl(S5P_CLK_OUT) | (0x1) , S5P_CLK_OUT);	
#else 
#if (defined CONFIG_JUPITER_VER_B5)  || (defined CONFIG_ARIES_VER_B0)
		DEBUG_LOG("LDO Enable = %d, XTAL CLK From OSI", en);
		gpio_set_value(GPIO_CODEC_XTAL_EN, 1);
#elif (defined CONFIG_ARIES_VER_B1) || (defined CONFIG_ARIES_VER_B2) || (defined CONFIG_ARIES_VER_B3)
#if (defined CONFIG_SND_UNIVERSAL_WM8994_MASTER)
		if(hw_version_check())
		{
			__raw_writel(__raw_readl(S5P_OTHERS) | (3<<8) , S5P_OTHERS); 
			__raw_writel(__raw_readl(S5P_CLK_OUT) | (0x1) , S5P_CLK_OUT);	
		}
#endif
#elif (defined CONFIG_JUPITER_VER_B4)
#ifdef CONFIG_SND_UNIVERSAL_WM8994_MASTER
		val |= (0x11 << 12); //crystall
#else
		val |= (0x02 << 12); // epll
		val |= (0x5 << 20);
#endif	// end of CONFIG_SND_UNIVERSAL_WM8994_MASTER
		 __raw_writel(val, S5P_CLK_OUT);
#endif	//end of CONFIG_JUPITER_VER_05
#endif 
	}
	else
	{
		// Turn off LDO for codec.
		gpio_set_value(GPIO_CODEC_LDO_EN, 0);

		msleep(125);	// Wait to turn off codec entirely.

		// Turn on master clock.
#if 1
		__raw_writel(__raw_readl(S5P_OTHERS) & (~(0x3<<8)) , S5P_OTHERS);
		__raw_writel(__raw_readl(S5P_CLK_OUT) & (0xFFFFFFFE) , S5P_CLK_OUT);
#else 
#if (defined CONFIG_JUPITER_VER_B5) ||(defined CONFIG_ARIES_VER_B0)
		DEBUG_LOG("LDO Disable = %d, XTAL CLK From OSI", en);
		gpio_set_value(GPIO_CODEC_XTAL_EN, 0);
#elif (defined CONFIG_JUPITER_VER_B4)
		DEBUG_LOG("LDO Disable = %d, XTAL CLK From AP EMUL", en);
#ifdef CONFIG_SND_UNIVERSAL_WM8994_MASTER
		val &= ~(0x11 << 12); //crystall
#else
		val &= ~(0x02 << 12); // epll
		val &= ~(0x5 << 20);
#endif	//end of CONFIG_SND_UNIVERSAL_WM8994_MASTER
#elif (defined CONFIG_ARIES_VER_B1) || (defined CONFIG_ARIES_VER_B2) || (defined CONFIG_ARIES_VER_B3)
#if (defined CONFIG_SND_UNIVERSAL_WM8994_MASTER)
		__raw_writel(__raw_readl(S5P_OTHERS) & (~(0x3<<8)) , S5P_OTHERS);
		__raw_writel(__raw_readl(S5P_CLK_OUT) & (0xFFFFFFFE) , S5P_CLK_OUT);
#endif
#else
		__raw_writel(val, S5P_CLK_OUT);	
#endif	//end of CONFIG_JUPITER_VER_05
#endif 
	}

	DEBUG_LOG("AUDIO POWER COMPLETED : %d", en);

	return 0;
}

void audio_ctrl_mic_bias_gpio(int enable)
{
	int headset_state;
	
	DEBUG_LOG("enable = [%d]", enable);
	
	if(enable)
		gpio_set_value(GPIO_MICBIAS_EN, 1);
	else
	{
		headset_state = get_headset_status();
		
		if( headset_state == 0 ||headset_state == 0x1 << 5 )	// TV out or no headset
			gpio_set_value(GPIO_MICBIAS_EN, 0);
	}
}

/*Audio Routing routines for the universal board..wm8994 codec*/
void wm8994_disable_playback_path(struct snd_soc_codec *codec, enum audio_path path)
{
	u16 val;

	DEBUG_LOG("Path = [%d]", path);

	val = wm8994_read(codec, WM8994_POWER_MANAGEMENT_1);	
	
	switch(path)
	{
		case RCV:		
			//Disbale the HPOUT2
			val &= ~(WM8994_HPOUT2_ENA_MASK);
			wm8994_write(codec, WM8994_POWER_MANAGEMENT_1, val);
			
			// Disable left MIXOUT
			val = wm8994_read(codec,WM8994_OUTPUT_MIXER_1);
			val &= ~(WM8994_DAC1L_TO_MIXOUTL_MASK);
			wm8994_write(codec,WM8994_OUTPUT_MIXER_1,val);

			// Disable right MIXOUT
			val = wm8994_read(codec,WM8994_OUTPUT_MIXER_2);
			val &= ~(WM8994_DAC1R_TO_MIXOUTR_MASK);
			wm8994_write(codec,WM8994_OUTPUT_MIXER_2,val);

			// Disable HPOUT Mixer
			val = wm8994_read(codec,WM8994_HPOUT2_MIXER);
			val &= ~(WM8994_MIXOUTLVOL_TO_HPOUT2_MASK | WM8994_MIXOUTRVOL_TO_HPOUT2_MASK);
			wm8994_write(codec,WM8994_HPOUT2_MIXER,val);

			// Disable mixout volume control
			val = wm8994_read(codec,WM8994_POWER_MANAGEMENT_3);
			val &= ~(WM8994_MIXOUTLVOL_ENA_MASK | WM8994_MIXOUTRVOL_ENA_MASK | WM8994_MIXOUTL_ENA_MASK | WM8994_MIXOUTR_ENA_MASK);
			wm8994_write(codec,WM8994_POWER_MANAGEMENT_3,val);			
			break;

		case SPK:
			//Disbale the SPKOUTL
			val &= ~(WM8994_SPKOUTL_ENA_MASK); 
			wm8994_write(codec, WM8994_POWER_MANAGEMENT_1, val);

			// Disable SPKLVOL
			val = wm8994_read(codec,WM8994_POWER_MANAGEMENT_3);
			val &= ~(WM8994_SPKLVOL_ENA_MASK);
			wm8994_write(codec, WM8994_POWER_MANAGEMENT_3, val);

			// Disable SPKOUT mixer
			val = wm8994_read(codec,WM8994_SPKOUT_MIXERS);
			val &= ~(WM8994_SPKMIXL_TO_SPKOUTL_MASK | 
				WM8994_SPKMIXR_TO_SPKOUTL_MASK | WM8994_SPKMIXR_TO_SPKOUTR_MASK);
			wm8994_write(codec, WM8994_SPKOUT_MIXERS, val);
			
			//Mute Speaker mixer
			val = wm8994_read(codec,WM8994_SPEAKER_MIXER);
			val &= ~(WM8994_DAC1L_TO_SPKMIXL_MASK);
			wm8994_write(codec, WM8994_SPEAKER_MIXER ,val);
			break;

		case HP:
			#if 0
			val &= ~(WM8994_HPOUT1L_ENA_MASK |WM8994_HPOUT1R_ENA_MASK);
			wm8994_write(codec, WM8994_POWER_MANAGEMENT_1, val);

			// Disable DAC1L to HPOUT1L path
			val = wm8994_read(codec, WM8994_OUTPUT_MIXER_1);
			val &= ~(WM8994_DAC1L_TO_HPOUT1L_MASK | WM8994_DAC1L_TO_MIXOUTL_MASK);
			wm8994_write(codec,WM8994_OUTPUT_MIXER_1, val);
			
			// Disable DAC1R to HPOUT1R path
			val = wm8994_read(codec, WM8994_OUTPUT_MIXER_2);
			val &= ~(WM8994_DAC1R_TO_HPOUT1R_MASK | WM8994_DAC1R_TO_MIXOUTR_MASK);
			wm8994_write(codec, WM8994_OUTPUT_MIXER_2, val);
			
			//Disable Charge Pump	
			val = wm8994_read(codec, WM8994_CHARGE_PUMP_1);
			val &= ~WM8994_CP_ENA_MASK ;
			val |=  WM8994_CP_ENA_DEFAULT ; // this is from wolfson	
			wm8994_write(codec, WM8994_CHARGE_PUMP_1, val);
			
			// Intermediate HP settings
			val = wm8994_read(codec, WM8994_ANALOGUE_HP_1); 	
			val &= ~(WM8994_HPOUT1R_DLY_MASK |WM8994_HPOUT1R_OUTP_MASK |WM8994_HPOUT1R_RMV_SHORT_MASK |
				WM8994_HPOUT1L_DLY_MASK |WM8994_HPOUT1L_OUTP_MASK | WM8994_HPOUT1L_RMV_SHORT_MASK);
			wm8994_write(codec, WM8994_ANALOGUE_HP_1, val);		
			DEBUG_LOG("WM8994_ANALOGUE_HP_1: 0x%x", val);	
			#else

			val = wm8994_read(codec, WM8994_DAC1_LEFT_VOLUME);
			val &= ~(0x02C0);
			val |= 0x02C0;
			wm8994_write(codec, WM8994_DAC1_LEFT_VOLUME, 0x02C0);

			val = wm8994_read(codec, WM8994_DAC1_RIGHT_VOLUME);
			val &= ~(0x02C0);
			val |= 0x02C0;
			wm8994_write(codec, WM8994_DAC1_RIGHT_VOLUME, 0x02C0);

			val = wm8994_read(codec, WM8994_ANALOGUE_HP_1);
			val &= ~(0x0022);
			val |= 0x0022;
			wm8994_write(codec, WM8994_ANALOGUE_HP_1, 0x0022);

			val = wm8994_read(codec, WM8994_OUTPUT_MIXER_1);
			val &= ~(0x0);
			val |= 0x0;
			wm8994_write(codec, WM8994_OUTPUT_MIXER_1, 0x0);

			val = wm8994_read(codec, WM8994_OUTPUT_MIXER_2);
			val &= ~(0x0);
			val |= 0x0;
			wm8994_write(codec, WM8994_OUTPUT_MIXER_2, 0x0);

			val = wm8994_read(codec, WM8994_POWER_MANAGEMENT_5);
			val &= ~(0x0300);
			val |= 0x0300;
			wm8994_write(codec, WM8994_POWER_MANAGEMENT_5, 0x0300);

			val = wm8994_read(codec, WM8994_CHARGE_PUMP_1);
			val &= ~(0x1F25);
			val |= 0x1F25;
			wm8994_write(codec, WM8994_CHARGE_PUMP_1, 0x1F25);
			#endif
			break;

		case SPK_HP :
			val &= ~(WM8994_HPOUT1L_ENA_MASK |WM8994_HPOUT1R_ENA_MASK |WM8994_SPKOUTL_ENA_MASK);
			wm8994_write(codec, WM8994_POWER_MANAGEMENT_1, val);

			// ------------------ Ear path setting ------------------
			// Disable DAC1L to HPOUT1L path
			val = wm8994_read(codec, WM8994_OUTPUT_MIXER_1);
			val &= ~(WM8994_DAC1L_TO_HPOUT1L_MASK | WM8994_DAC1L_TO_MIXOUTL_MASK);
			wm8994_write(codec,WM8994_OUTPUT_MIXER_1, val);
			
			// Disable DAC1R to HPOUT1R path
			val = wm8994_read(codec, WM8994_OUTPUT_MIXER_2);
			val &= ~(WM8994_DAC1R_TO_HPOUT1R_MASK | WM8994_DAC1R_TO_MIXOUTR_MASK);
			wm8994_write(codec, WM8994_OUTPUT_MIXER_2, val);
			
			//Disable Charge Pump	
			val = wm8994_read(codec, WM8994_CHARGE_PUMP_1);
			val &= ~WM8994_CP_ENA_MASK ;
			val |=  WM8994_CP_ENA_DEFAULT ; // this is from wolfson	
			wm8994_write(codec, WM8994_CHARGE_PUMP_1, val);
			
			// Intermediate HP settings
			val = wm8994_read(codec, WM8994_ANALOGUE_HP_1); 	
			val &= ~(WM8994_HPOUT1R_DLY_MASK |WM8994_HPOUT1R_OUTP_MASK |WM8994_HPOUT1R_RMV_SHORT_MASK |
				WM8994_HPOUT1L_DLY_MASK |WM8994_HPOUT1L_OUTP_MASK | WM8994_HPOUT1L_RMV_SHORT_MASK);
			wm8994_write(codec, WM8994_ANALOGUE_HP_1, val);

			// ------------------ Spk path setting ------------------		
			// Disable SPKLVOL
			val = wm8994_read(codec,WM8994_POWER_MANAGEMENT_3);
			val &= ~(WM8994_SPKLVOL_ENA_MASK);
			wm8994_write(codec, WM8994_POWER_MANAGEMENT_3, val);

			// Disable SPKOUT mixer
			val = wm8994_read(codec,WM8994_SPKOUT_MIXERS);
			val &= ~(WM8994_SPKMIXL_TO_SPKOUTL_MASK | 
				WM8994_SPKMIXR_TO_SPKOUTL_MASK | WM8994_SPKMIXR_TO_SPKOUTR_MASK);
			wm8994_write(codec, WM8994_SPKOUT_MIXERS, val);
			
			//Mute Speaker mixer
			val = wm8994_read(codec,WM8994_SPEAKER_MIXER);
			val &= ~(WM8994_DAC1L_TO_SPKMIXL_MASK);
			wm8994_write(codec, WM8994_SPEAKER_MIXER ,val);
			break;			

		default:
			DEBUG_LOG_ERR("Path[%d] is not invaild!\n", path);
			return;
			break;
	}
}

void wm8994_disable_rec_path(struct snd_soc_codec *codec, enum mic_path rec_path) 
{
	struct wm8994_priv *wm8994 = codec->private_data;

	u16 val;

	wm8994->rec_path = MIC_OFF;
	audio_ctrl_mic_bias_gpio(0);	// Disable MIC bias
	
        val = wm8994_read(codec, WM8994_POWER_MANAGEMENT_1);
	val &= ~(WM8994_MICB1_ENA_MASK);
	wm8994_write(codec,WM8994_POWER_MANAGEMENT_1, val);

        switch(rec_path)
        {
                case MAIN:
			DEBUG_LOG("Disabling MAIN Mic Path..\n");
			
			val = wm8994_read(codec,WM8994_POWER_MANAGEMENT_2  );
			val &= ~( WM8994_IN1L_ENA_MASK | WM8994_MIXINL_ENA_MASK );
			wm8994_write(codec,WM8994_POWER_MANAGEMENT_2,val);
			
			if(!wm8994->testmode_config_flag)
			{	
				// Mute IN1L PGA, update volume
				val = wm8994_read(codec, WM8994_LEFT_LINE_INPUT_1_2_VOLUME);	
				val &= ~(WM8994_IN1L_MUTE_MASK | WM8994_IN1L_VOL_MASK);
				val |= (WM8994_IN1L_VU |WM8994_IN1L_MUTE); //volume
				wm8994_write(codec, WM8994_LEFT_LINE_INPUT_1_2_VOLUME, val);
			
				//Mute the PGA
				val = wm8994_read(codec, WM8994_INPUT_MIXER_3);
				val&= ~(WM8994_IN1L_TO_MIXINL_MASK | WM8994_IN1L_MIXINL_VOL_MASK | WM8994_MIXOUTL_MIXINL_VOL_MASK);
				wm8994_write(codec, WM8994_INPUT_MIXER_3, val); 
			}
			
			//Disconnect IN1LN ans IN1LP to the inputs
			val = wm8994_read(codec,WM8994_INPUT_MIXER_2 ); 
			val &= (WM8994_IN1LN_TO_IN1L_MASK | WM8994_IN1LP_TO_IN1L_MASK );
			wm8994_write(codec, WM8994_INPUT_MIXER_2, val);
			
			//Digital Paths
			
			val = wm8994_read(codec,WM8994_POWER_MANAGEMENT_4 );
			val &= ~(WM8994_ADCL_ENA_MASK |WM8994_AIF1ADC1L_ENA_MASK  );
			wm8994_write(codec,WM8994_POWER_MANAGEMENT_4 ,val);

			//Disable timeslots
			val = wm8994_read(codec,WM8994_AIF1_ADC1_LEFT_MIXER_ROUTING );
			val |=WM8994_ADC1L_TO_AIF1ADC1L ;  
			wm8994_write(codec,WM8994_AIF1_ADC1_LEFT_MIXER_ROUTING ,val);			
			break;

                case SUB:
			DEBUG_LOG("Disbaling SUB Mic path..\n");
			val = wm8994_read(codec,WM8994_POWER_MANAGEMENT_2 );
			val &= ~(WM8994_IN1R_ENA_MASK |WM8994_MIXINR_ENA_MASK  );
			wm8994_write(codec,WM8994_POWER_MANAGEMENT_2,val);
			
			if(!wm8994->testmode_config_flag)
			{	
				// Disable volume,unmute Right Line	
				val = wm8994_read(codec,WM8994_RIGHT_LINE_INPUT_1_2_VOLUME);	
				val &= ~WM8994_IN1R_MUTE_MASK;	// Unmute IN1R
				val |= (WM8994_IN1R_VU | WM8994_IN1R_MUTE);
				wm8994_write(codec,WM8994_RIGHT_LINE_INPUT_1_2_VOLUME,val);
				
				// Mute right pga, set volume 
				val = wm8994_read(codec,WM8994_INPUT_MIXER_4 );
				val&= ~(WM8994_IN1R_TO_MIXINR_MASK | WM8994_IN1R_MIXINR_VOL_MASK | WM8994_MIXOUTR_MIXINR_VOL_MASK);
				wm8994_write(codec,WM8994_INPUT_MIXER_4 ,val);
			}
			
			//Disconnect in1rn to inr1 and in1rp to inrp
			val = wm8994_read(codec,WM8994_INPUT_MIXER_2);
			val &= ~( WM8994_IN1RN_TO_IN1R_MASK | WM8994_IN1RP_TO_IN1R_MASK);
			wm8994_write(codec,WM8994_INPUT_MIXER_2,val);
			
			//Digital Paths 
			//Disable right ADC and time slot
			val = wm8994_read(codec,WM8994_POWER_MANAGEMENT_4);
			val &= ~(WM8994_ADCR_ENA_MASK |WM8994_AIF1ADC1R_ENA_MASK );
			wm8994_write(codec,WM8994_POWER_MANAGEMENT_4 ,val);
			
			//ADC Right mixer routing
			val = wm8994_read(codec,WM8994_AIF1_ADC1_RIGHT_MIXER_ROUTING);
			val &= ~( WM8994_ADC1R_TO_AIF1ADC1R_MASK);
			wm8994_write(codec,WM8994_AIF1_ADC1_RIGHT_MIXER_ROUTING,val);
			break;
	
		default:
			DEBUG_LOG_ERR("Path[%d] is not invaild!\n", rec_path);
                	break;
        }
}

void wm8994_disable_fmradio_path(struct snd_soc_codec *codec, enum fmradio_audio_path path)
{
	struct wm8994_priv *wm8994 = codec->private_data;

	u16 val;

	DEBUG_LOG("Turn off fmradio_path = [%d]", path);

	switch(path)
	{
		case FMR_OFF :
			wm8994->fmradio_path = FMR_OFF;

			//---------------------------------------------------
			// Disable speaker setting for FM radio

			//disbale the SPKOUTL
			val =wm8994_read(codec,WM8994_POWER_MANAGEMENT_1);
			val &= ~(WM8994_SPKOUTL_ENA_MASK); 
			wm8994_write(codec,WM8994_POWER_MANAGEMENT_1 ,val);

			// Disable SPK Volume.
			val = wm8994_read(codec, WM8994_POWER_MANAGEMENT_3);
			val &= ~(WM8994_SPKRVOL_ENA_MASK | WM8994_SPKLVOL_ENA_MASK);
			wm8994_write(codec, WM8994_POWER_MANAGEMENT_3, val);

			if(!wm8994->testmode_config_flag)
			{	
				// Mute the SPKMIXVOLUME
				val = wm8994_read(codec, WM8994_SPKMIXL_ATTENUATION);
				val &= ~(WM8994_SPKMIXL_VOL_MASK);
				wm8994_write(codec, WM8994_SPKMIXL_ATTENUATION, val);
					
				val = wm8994_read(codec, WM8994_SPKMIXR_ATTENUATION);
				val &= ~(WM8994_SPKMIXR_VOL_MASK);
				wm8994_write(codec, WM8994_SPKMIXR_ATTENUATION, val);
			
				val = wm8994_read(codec, WM8994_SPEAKER_VOLUME_LEFT);
				val &= ~(WM8994_SPKOUTL_MUTE_N_MASK | WM8994_SPKOUTL_VOL_MASK);
				wm8994_write(codec, WM8994_SPEAKER_VOLUME_LEFT, val);
			
				val = wm8994_read(codec, WM8994_SPEAKER_VOLUME_RIGHT);
				val &= ~(WM8994_SPKOUTR_MUTE_N_MASK | WM8994_SPKOUTR_VOL_MASK);
				wm8994_write(codec, WM8994_SPEAKER_VOLUME_RIGHT, val);
			
				val = wm8994_read(codec, WM8994_CLASSD);
				val &= ~(WM8994_SPKOUTL_BOOST_MASK);
				wm8994_write(codec, WM8994_CLASSD, val);
			}
			
			/*Output MIxer-Output PGA*/
			val = wm8994_read(codec,WM8994_SPKOUT_MIXERS );
			val &= ~(WM8994_SPKMIXL_TO_SPKOUTL_MASK | 
				WM8994_SPKMIXR_TO_SPKOUTL_MASK | WM8994_SPKMIXR_TO_SPKOUTR_MASK);
			wm8994_write(codec, WM8994_SPKOUT_MIXERS, val);
							
			// Output mixer setting
			val = wm8994_read(codec, WM8994_SPEAKER_MIXER);
			val &= ~(WM8994_MIXINL_TO_SPKMIXL_MASK | WM8994_MIXINR_TO_SPKMIXR_MASK);
			wm8994_write(codec, WM8994_SPEAKER_MIXER, val);

			//---------------------------------------------------
			// Disable earpath setting for FM radio

			//Disable end point for preventing pop up noise.
			val =wm8994_read(codec,WM8994_POWER_MANAGEMENT_1);
			val &= ~(WM8994_HPOUT1L_ENA_MASK |WM8994_HPOUT1R_ENA_MASK);
			wm8994_write(codec, WM8994_POWER_MANAGEMENT_1, val);

			// Disable MIXOUT
			val = wm8994_read(codec, WM8994_POWER_MANAGEMENT_3);
			val &= ~(WM8994_MIXOUTLVOL_ENA_MASK | WM8994_MIXOUTRVOL_ENA_MASK | WM8994_MIXOUTL_ENA_MASK | WM8994_MIXOUTR_ENA_MASK);
			wm8994_write(codec, WM8994_POWER_MANAGEMENT_3, val);

			if(!wm8994->testmode_config_flag)
			{
				// Output setting
				val = wm8994_read(codec, WM8994_LEFT_OUTPUT_VOLUME);
				val &= ~(WM8994_HPOUT1L_MUTE_N_MASK | WM8994_HPOUT1L_VOL_MASK);
				val |= (WM8994_HPOUT1_VU | WM8994_HPOUT1L_MUTE_N | TUNING_MP3_OUTPUTL_VOL);
				wm8994_write(codec, WM8994_LEFT_OUTPUT_VOLUME, val);
			
				val = wm8994_read(codec, WM8994_RIGHT_OUTPUT_VOLUME);
				val &= ~(WM8994_HPOUT1R_MUTE_N_MASK | WM8994_HPOUT1R_VOL_MASK);
				val |= (WM8994_HPOUT1_VU | WM8994_HPOUT1R_MUTE_N | TUNING_MP3_OUTPUTR_VOL);
				wm8994_write(codec, WM8994_RIGHT_OUTPUT_VOLUME, val);
			}

			//Disable Charge Pump	
			val = wm8994_read(codec, WM8994_CHARGE_PUMP_1);
			val &= ~WM8994_CP_ENA_MASK ;
			val |= WM8994_CP_ENA_DEFAULT ; // this is from wolfson		
			wm8994_write(codec, WM8994_CHARGE_PUMP_1, val);
			
			// Intermediate HP settings
			val = wm8994_read(codec, WM8994_ANALOGUE_HP_1); 	
			val &= ~(WM8994_HPOUT1R_DLY_MASK |WM8994_HPOUT1R_OUTP_MASK |WM8994_HPOUT1R_RMV_SHORT_MASK |
				WM8994_HPOUT1L_DLY_MASK |WM8994_HPOUT1L_OUTP_MASK | WM8994_HPOUT1L_RMV_SHORT_MASK);
			wm8994_write(codec, WM8994_ANALOGUE_HP_1, val);
												
			// Disable Output mixer setting
			val = wm8994_read(codec, WM8994_OUTPUT_MIXER_1);
			val &= ~(WM8994_DAC1L_TO_HPOUT1L_MASK | WM8994_MIXINL_TO_MIXOUTL_MASK);
			wm8994_write(codec, WM8994_OUTPUT_MIXER_1, val); 
			
			val = wm8994_read(codec, WM8994_OUTPUT_MIXER_2);
			val &= ~(WM8994_DAC1R_TO_HPOUT1R_MASK | WM8994_MIXINR_TO_MIXOUTR_MASK);
			wm8994_write(codec, WM8994_OUTPUT_MIXER_2, val); 

			//---------------------------------------------------
			// Disable common setting for FM radio
			
			// Disable IN2 and MIXIN
			val = wm8994_read(codec, WM8994_POWER_MANAGEMENT_2);
			val &= ~(WM8994_TSHUT_ENA_MASK | WM8994_TSHUT_OPDIS_MASK | WM8994_OPCLK_ENA_MASK | 
					WM8994_MIXINL_ENA_MASK | WM8994_MIXINR_ENA_MASK | WM8994_IN2L_ENA_MASK | WM8994_IN2R_ENA_MASK);
			wm8994_write(codec, WM8994_POWER_MANAGEMENT_2, val);
			
			// Disable Input mixer setting
			val = wm8994_read(codec, WM8994_INPUT_MIXER_2);
			val &= ~(WM8994_IN2LN_TO_IN2L_MASK | WM8994_IN2RN_TO_IN2R_MASK);
			wm8994_write(codec, WM8994_INPUT_MIXER_2, val); 	
			
			if(!wm8994->testmode_config_flag)
			{
				// Disable IN2L to MIXINL
				val = wm8994_read(codec, WM8994_INPUT_MIXER_3);
				val &= ~(WM8994_IN2L_TO_MIXINL_MASK);
				wm8994_write(codec, WM8994_INPUT_MIXER_3, val);
				
				//Disable IN2R to MIXINR
				val = wm8994_read(codec, WM8994_INPUT_MIXER_4);
				val &= ~(WM8994_IN2R_TO_MIXINR_MASK);
				wm8994_write(codec, WM8994_INPUT_MIXER_4, val); 	
			}
							
			// Mute IN2L PGA volume
			val = wm8994_read(codec, WM8994_LEFT_LINE_INPUT_3_4_VOLUME);
			val &= ~(WM8994_IN2L_VU_MASK | WM8994_IN2L_MUTE_MASK | WM8994_IN2L_VOL_MASK);
			val |= (WM8994_IN2L_VU | WM8994_IN2L_MUTE);
			wm8994_write(codec, WM8994_LEFT_LINE_INPUT_3_4_VOLUME, val); 
			
			val = wm8994_read(codec, WM8994_RIGHT_LINE_INPUT_3_4_VOLUME);
			val &= ~(WM8994_IN2R_VU_MASK | WM8994_IN2R_MUTE_MASK | WM8994_IN2R_VOL_MASK);
			val |= (WM8994_IN2R_VU |WM8994_IN2R_MUTE);	
			wm8994_write(codec, WM8994_RIGHT_LINE_INPUT_3_4_VOLUME, val); 

			//---------------------------------------------------
			// Disable path setting for mixing
			val = wm8994_read(codec,WM8994_SPEAKER_MIXER);
			val &= ~(WM8994_DAC1L_TO_SPKMIXL_MASK);
			wm8994_write(codec, WM8994_SPEAKER_MIXER, val);			

			// Disable DAC1L to HPOUT1L path
			val = wm8994_read(codec, WM8994_OUTPUT_MIXER_1);
			val &= ~(WM8994_DAC1L_TO_HPOUT1L_MASK | WM8994_DAC1L_TO_MIXOUTL_MASK);
			wm8994_write(codec,WM8994_OUTPUT_MIXER_1, val);
			
			// Disable DAC1R to HPOUT1R path
			val = wm8994_read(codec, WM8994_OUTPUT_MIXER_2);
			val &= ~(WM8994_DAC1R_TO_HPOUT1R_MASK | WM8994_DAC1R_TO_MIXOUTR_MASK);
			wm8994_write(codec, WM8994_OUTPUT_MIXER_2, val);				
			break;
		
		case FMR_SPK :				
			// Output mixer setting
			val = wm8994_read(codec, WM8994_SPEAKER_MIXER);
			val &= ~(WM8994_MIXINL_TO_SPKMIXL_MASK | WM8994_MIXINR_TO_SPKMIXR_MASK);
			wm8994_write(codec, WM8994_SPEAKER_MIXER, val);
			break;

		case FMR_HP :
			//Disable end point for preventing pop up noise.
			// Disable Output mixer setting
			val = wm8994_read(codec, WM8994_OUTPUT_MIXER_1);
			val &= ~(WM8994_DAC1L_TO_HPOUT1L_MASK | WM8994_MIXINL_TO_MIXOUTL_MASK);
			wm8994_write(codec, WM8994_OUTPUT_MIXER_1, val); 
			
			val = wm8994_read(codec, WM8994_OUTPUT_MIXER_2);
			val &= ~(WM8994_DAC1R_TO_HPOUT1R_MASK | WM8994_MIXINR_TO_MIXOUTR_MASK);
			wm8994_write(codec, WM8994_OUTPUT_MIXER_2, val); 
			break;

		case FMR_SPK_MIX :
			//Mute the DAC path
			val = wm8994_read(codec,WM8994_SPEAKER_MIXER);
			val &= ~(WM8994_DAC1L_TO_SPKMIXL_MASK);
			wm8994_write(codec, WM8994_SPEAKER_MIXER, val);			
			break;

		case FMR_HP_MIX :					
			// Disable DAC1L to HPOUT1L path
			val = wm8994_read(codec, WM8994_OUTPUT_MIXER_1);
			val &= ~(WM8994_DAC1L_TO_HPOUT1L_MASK | WM8994_DAC1L_TO_MIXOUTL_MASK);
			wm8994_write(codec,WM8994_OUTPUT_MIXER_1, val);
			
			// Disable DAC1R to HPOUT1R path
			val = wm8994_read(codec, WM8994_OUTPUT_MIXER_2);
			val &= ~(WM8994_DAC1R_TO_HPOUT1R_MASK | WM8994_DAC1R_TO_MIXOUTR_MASK);
			wm8994_write(codec, WM8994_OUTPUT_MIXER_2, val);	
			break;

		case FMR_SPK_HP_MIX :		
			//Mute the DAC path
			val = wm8994_read(codec,WM8994_SPEAKER_MIXER);
			val &= ~(WM8994_DAC1L_TO_SPKMIXL_MASK);
			wm8994_write(codec, WM8994_SPEAKER_MIXER, val);
			
			// Disable DAC1L to HPOUT1L path
			val = wm8994_read(codec, WM8994_OUTPUT_MIXER_1);
			val &= ~(WM8994_DAC1L_TO_HPOUT1L_MASK | WM8994_DAC1L_TO_MIXOUTL_MASK);
			wm8994_write(codec,WM8994_OUTPUT_MIXER_1, val);
			
			// Disable DAC1R to HPOUT1R path
			val = wm8994_read(codec, WM8994_OUTPUT_MIXER_2);
			val &= ~(WM8994_DAC1R_TO_HPOUT1R_MASK | WM8994_DAC1R_TO_MIXOUTR_MASK);
			wm8994_write(codec, WM8994_OUTPUT_MIXER_2, val);	
			break;

		default:
			DEBUG_LOG_ERR("fmradio path[%d] is not invaild!\n", path);
			return;
			break;
		}
}

void wm8994_record_headset_mic(struct snd_soc_codec *codec) 
{
	struct wm8994_priv *wm8994 = codec->private_data;
	
	u16 val;

	audio_ctrl_mic_bias_gpio(1);
	
	DEBUG_LOG("Recording through Headset Mic\n");

	//Enable mic bias, vmid, bias generator
	val = wm8994_read(codec,WM8994_POWER_MANAGEMENT_1 );
	val &= ~(WM8994_BIAS_ENA_MASK | WM8994_VMID_SEL_MASK | WM8994_MICB1_ENA_MASK  );
	val |= (WM8994_BIAS_ENA | WM8994_VMID_SEL_NORMAL |WM8994_MICB1_ENA  );  
	wm8994_write(codec,WM8994_POWER_MANAGEMENT_1,val);

	//Enable Right Input Mixer,Enable IN1R PGA - Temporary inserted for blocking MIC and FM radio mixing - DW Shim 2010.03.04
//	val = wm8994_read(codec,WM8994_POWER_MANAGEMENT_2 );
//	val &= ~(WM8994_IN1R_ENA_MASK |WM8994_MIXINR_ENA_MASK  );
//	val |= (WM8994_MIXINR_ENA | WM8994_IN1R_ENA );
	val = (WM8994_MIXINR_ENA | WM8994_IN1R_ENA );
	wm8994_write(codec,WM8994_POWER_MANAGEMENT_2,val);

	if(!wm8994->testmode_config_flag)
	{	
		// Enable volume,unmute Right Line	
		val = wm8994_read(codec,WM8994_RIGHT_LINE_INPUT_1_2_VOLUME);	
		val &= ~WM8994_IN1R_MUTE_MASK;	// Unmute IN1R
		val |= (WM8994_IN1R_VU | TUNING_RECORD_SUB_INPUTMIX_VOL);
		wm8994_write(codec,WM8994_RIGHT_LINE_INPUT_1_2_VOLUME,val);
		
		// unmute right pga, set volume 
		val = wm8994_read(codec,WM8994_INPUT_MIXER_4 );
		val&= ~(WM8994_IN1R_TO_MIXINR_MASK | WM8994_IN1R_MIXINR_VOL_MASK | WM8994_MIXOUTR_MIXINR_VOL_MASK);
		val |= (WM8994_IN1R_TO_MIXINR);//0db
		wm8994_write(codec,WM8994_INPUT_MIXER_4 ,val);
	}
	
	//connect in1rn to inr1 and in1rp to inrp - Temporary inserted for blocking MIC and FM radio mixing - DW Shim 2010.03.04
//	val = wm8994_read(codec,WM8994_INPUT_MIXER_2);
//	val &= ~( WM8994_IN1RN_TO_IN1R_MASK | WM8994_IN1RP_TO_IN1R_MASK);
//	val |= (WM8994_IN1RN_TO_IN1R | WM8994_IN1RP_TO_IN1R)  ;	
	val = (WM8994_IN1RN_TO_IN1R | WM8994_IN1RP_TO_IN1R)  ; 
	wm8994_write(codec,WM8994_INPUT_MIXER_2,val);
	
	//Digital Paths	
	//Enable right ADC and time slot
	val = wm8994_read(codec,WM8994_POWER_MANAGEMENT_4);
	val &= ~(WM8994_ADCR_ENA_MASK |WM8994_AIF1ADC1R_ENA_MASK );
	val |= (WM8994_AIF1ADC1R_ENA | WM8994_ADCR_ENA  );
	wm8994_write(codec,WM8994_POWER_MANAGEMENT_4 ,val);

	//ADC Right mixer routing
	val = wm8994_read(codec,WM8994_AIF1_ADC1_RIGHT_MIXER_ROUTING);
	val &= ~( WM8994_ADC1R_TO_AIF1ADC1R_MASK);
	val |= WM8994_ADC1R_TO_AIF1ADC1R;
	wm8994_write(codec,WM8994_AIF1_ADC1_RIGHT_MIXER_ROUTING,val);

	val = wm8994_read(codec, WM8994_AIF1_MASTER_SLAVE);
	val |= (WM8994_AIF1_MASTER_SLAVE, WM8994_AIF1_MSTR | WM8994_AIF1_CLK_FRC | WM8994_AIF1_LRCLK_FRC);	// Master mode
	wm8994_write(codec, WM8994_AIF1_MASTER_SLAVE, val);

	wm8994_write( codec, WM8994_GPIO_1, 0xA101 );   // GPIO1 is Input Enable
}

void wm8994_record_main_mic(struct snd_soc_codec *codec) 
{
	struct wm8994_priv *wm8994 = codec->private_data;

	u16 val;

	DEBUG_LOG("Recording through Main Mic\n");
	audio_ctrl_mic_bias_gpio(1);
	
	//Enable micbias,vmid,mic1
	val = wm8994_read(codec,WM8994_POWER_MANAGEMENT_1 );
	val &= ~(WM8994_BIAS_ENA_MASK | WM8994_VMID_SEL_MASK | WM8994_MICB1_ENA_MASK  );
	val |= (WM8994_BIAS_ENA | WM8994_VMID_SEL_NORMAL |WM8994_MICB1_ENA  );  
	wm8994_write(codec,WM8994_POWER_MANAGEMENT_1,val);

	//Enable left input mixer and IN1L PGA	- Temporary inserted for blocking MIC and FM radio mixing - DW Shim 2010.03.04
//	val = wm8994_read(codec,WM8994_POWER_MANAGEMENT_2  );
//	val &= ~( WM8994_IN1L_ENA_MASK | WM8994_MIXINL_ENA_MASK );
//	val |= (WM8994_MIXINL_ENA |WM8994_IN1L_ENA );
	val = (WM8994_MIXINL_ENA |WM8994_IN1L_ENA );
	wm8994_write(codec,WM8994_POWER_MANAGEMENT_2,val);

	if(!wm8994->testmode_config_flag)
	{	
		// Unmute IN1L PGA, update volume
		val = wm8994_read(codec, WM8994_LEFT_LINE_INPUT_1_2_VOLUME);	
		val &= ~(WM8994_IN1L_MUTE_MASK | WM8994_IN1L_VOL_MASK);
		val |= (WM8994_IN1L_VU |TUNING_RECORD_MAIN_INPUTLINE_VOL); //volume
		wm8994_write(codec, WM8994_LEFT_LINE_INPUT_1_2_VOLUME, val);
	
		//Unmute the PGA
		val = wm8994_read(codec, WM8994_INPUT_MIXER_3);
		val&= ~(WM8994_IN1L_TO_MIXINL_MASK | WM8994_IN1L_MIXINL_VOL_MASK | WM8994_MIXOUTL_MIXINL_VOL_MASK);
		val |= (WM8994_IN1L_MIXINL_VOL | WM8994_IN1L_TO_MIXINL);	//30db
		wm8994_write(codec, WM8994_INPUT_MIXER_3, val); 
	}
	
	//Connect IN1LN ans IN1LP to the inputs - Temporary inserted for blocking MIC and FM radio mixing - DW Shim 2010.03.04
//	val = wm8994_read(codec,WM8994_INPUT_MIXER_2 );	
//	val &= (WM8994_IN1LN_TO_IN1L_MASK | WM8994_IN1LP_TO_IN1L_MASK );
//	val |= ( WM8994_IN1LP_TO_IN1L | WM8994_IN1LN_TO_IN1L );
	val = ( WM8994_IN1LP_TO_IN1L | WM8994_IN1LN_TO_IN1L);
	wm8994_write(codec, WM8994_INPUT_MIXER_2, val);

	//Digital Paths
	
	val = wm8994_read(codec,WM8994_POWER_MANAGEMENT_4 );
	val &= ~(WM8994_ADCL_ENA_MASK |WM8994_AIF1ADC1L_ENA_MASK  );
	val |= ( WM8994_AIF1ADC1L_ENA | WM8994_ADCL_ENA );
	wm8994_write(codec,WM8994_POWER_MANAGEMENT_4 ,val);
	//Enable timeslots
	val = wm8994_read(codec,WM8994_AIF1_ADC1_LEFT_MIXER_ROUTING );
	val |=WM8994_ADC1L_TO_AIF1ADC1L ;  
	wm8994_write(codec,WM8994_AIF1_ADC1_LEFT_MIXER_ROUTING ,val);
	
	val = wm8994_read(codec, WM8994_AIF1_MASTER_SLAVE);
	val |= (WM8994_AIF1_MASTER_SLAVE, WM8994_AIF1_MSTR | WM8994_AIF1_CLK_FRC | WM8994_AIF1_LRCLK_FRC);	// Master mode
	wm8994_write(codec, WM8994_AIF1_MASTER_SLAVE, val);

	wm8994_write( codec, WM8994_GPIO_1, 0xA101 );   // GPIO1 is Input Enable
}

void wm8994_set_playback_receiver(struct snd_soc_codec *codec)
{
	struct wm8994_priv *wm8994 = codec->private_data;

	u16 val;

	DEBUG_LOG("");
	
	if(!wm8994->testmode_config_flag)
	{
	        val = wm8994_read(codec, WM8994_OUTPUT_MIXER_5);
		val &= ~(WM8994_DACL_MIXOUTL_VOL_MASK);
	        val |= TUNING_RCV_OUTMIX5_VOL << WM8994_DACL_MIXOUTL_VOL_SHIFT;
	        wm8994_write(codec,WM8994_OUTPUT_MIXER_5, val );
	
		val = wm8994_read(codec, WM8994_OUTPUT_MIXER_6);
		val &= ~(WM8994_DACR_MIXOUTR_VOL_MASK);
		val |= TUNING_RCV_OUTMIX6_VOL << WM8994_DACR_MIXOUTR_VOL_SHIFT;
		wm8994_write(codec,WM8994_OUTPUT_MIXER_6, val );

		val = wm8994_read(codec, WM8994_LEFT_OPGA_VOLUME);
		val &= ~(WM8994_MIXOUTL_MUTE_N_MASK | WM8994_MIXOUTL_VOL_MASK);
		val |= (WM8994_MIXOUT_VU |WM8994_MIXOUTL_MUTE_N | TUNING_RCV_OPGAL_VOL);
		wm8994_write(codec,WM8994_LEFT_OPGA_VOLUME, val );

		val = wm8994_read(codec, WM8994_RIGHT_OPGA_VOLUME);
		val &= ~(WM8994_MIXOUTR_MUTE_N_MASK | WM8994_MIXOUTR_VOL_MASK);
		val |= (WM8994_MIXOUT_VU | WM8994_MIXOUTR_MUTE_N | TUNING_RCV_OPGAR_VOL);
		wm8994_write(codec,WM8994_RIGHT_OPGA_VOLUME, val );
	
	        val = wm8994_read(codec, WM8994_HPOUT2_VOLUME);
		val &= ~(WM8994_HPOUT2_MUTE_MASK | WM8994_HPOUT2_VOL_MASK);
	        val |= TUNING_HPOUT2_VOL << WM8994_HPOUT2_VOL_SHIFT;
	        wm8994_write(codec,WM8994_HPOUT2_VOLUME, val );

		//Unmute DAC1 left
		val = wm8994_read(codec,WM8994_DAC1_LEFT_VOLUME );
		val &= ~(WM8994_DAC1L_MUTE_MASK | WM8994_DAC1L_VOL_MASK);
		val |= TUNING_DAC1L_VOL; 
		wm8994_write(codec,WM8994_DAC1_LEFT_VOLUME ,val);
	
		//Unmute and volume ctrl RightDAC
		val = wm8994_read(codec, WM8994_DAC1_RIGHT_VOLUME ); 
		val &= ~(WM8994_DAC1R_MUTE_MASK | WM8994_DAC1R_VOL_MASK);
		val |= TUNING_DAC1R_VOL;
		wm8994_write(codec,WM8994_DAC1_RIGHT_VOLUME,val);
	}

	val = wm8994_read(codec,WM8994_OUTPUT_MIXER_1);
        val &= ~(WM8994_DAC1L_TO_MIXOUTL_MASK);
        val |= (WM8994_DAC1L_TO_MIXOUTL);
        wm8994_write(codec,WM8994_OUTPUT_MIXER_1,val);

        val = wm8994_read(codec,WM8994_OUTPUT_MIXER_2);
        val &= ~(WM8994_DAC1R_TO_MIXOUTR_MASK);
        val |= (WM8994_DAC1R_TO_MIXOUTR);
        wm8994_write(codec,WM8994_OUTPUT_MIXER_2,val);

	val = wm8994_read(codec,WM8994_HPOUT2_MIXER);
        val &= ~(WM8994_MIXOUTLVOL_TO_HPOUT2_MASK | WM8994_MIXOUTRVOL_TO_HPOUT2_MASK);
        val |= (WM8994_MIXOUTRVOL_TO_HPOUT2 | WM8994_MIXOUTLVOL_TO_HPOUT2);
        wm8994_write(codec,WM8994_HPOUT2_MIXER,val);

        val = wm8994_read(codec,WM8994_POWER_MANAGEMENT_5);
        val &= ~(WM8994_DAC1R_ENA_MASK | WM8994_DAC1L_ENA_MASK | WM8994_AIF1DAC1R_ENA_MASK | WM8994_AIF1DAC1L_ENA_MASK);
        val |= (WM8994_AIF1DAC1L_ENA | WM8994_AIF1DAC1R_ENA | WM8994_DAC1L_ENA | WM8994_DAC1R_ENA);
        wm8994_write(codec,WM8994_POWER_MANAGEMENT_5,val);

        val = wm8994_read(codec,WM8994_AIF1_DAC1_FILTERS_1);
	val &= ~(WM8994_AIF1DAC1_MUTE_MASK);
	val |= WM8994_AIF1DAC1_UNMUTE;
        wm8994_write(codec,WM8994_AIF1_DAC1_FILTERS_1,val);

	val = wm8994_read(codec,WM8994_DAC1_LEFT_MIXER_ROUTING);
        val &= ~(WM8994_AIF1DAC1L_TO_DAC1L_MASK);
        val |= (WM8994_AIF1DAC1L_TO_DAC1L);
        wm8994_write(codec,WM8994_DAC1_LEFT_MIXER_ROUTING,val);

        val = wm8994_read(codec,WM8994_DAC1_RIGHT_MIXER_ROUTING);
        val &= ~(WM8994_AIF1DAC1R_TO_DAC1R_MASK);
        val |= (WM8994_AIF1DAC1R_TO_DAC1R);
        wm8994_write(codec,WM8994_DAC1_RIGHT_MIXER_ROUTING,val);

	val = wm8994_read(codec, WM8994_CLOCKING_1);
        val &= ~(WM8994_DSP_FS1CLK_ENA_MASK | WM8994_DSP_FSINTCLK_ENA_MASK);
        val |= (WM8994_DSP_FS1CLK_ENA | WM8994_DSP_FSINTCLK_ENA);
        wm8994_write(codec, WM8994_CLOCKING_1, val);

	val = wm8994_read(codec,WM8994_POWER_MANAGEMENT_3);
        val &= ~(WM8994_MIXOUTLVOL_ENA_MASK | WM8994_MIXOUTRVOL_ENA_MASK | WM8994_MIXOUTL_ENA_MASK | WM8994_MIXOUTR_ENA_MASK);
        val |= (WM8994_MIXOUTL_ENA | WM8994_MIXOUTR_ENA | WM8994_MIXOUTRVOL_ENA | WM8994_MIXOUTLVOL_ENA);
        wm8994_write(codec,WM8994_POWER_MANAGEMENT_3,val);
		
	val = wm8994_read(codec,WM8994_POWER_MANAGEMENT_1 );
	val &= ~(WM8994_BIAS_ENA_MASK | WM8994_VMID_SEL_MASK | WM8994_HPOUT2_ENA_MASK );
	val |= (WM8994_BIAS_ENA | WM8994_VMID_SEL_NORMAL | WM8994_HPOUT2_ENA );
	wm8994_write(codec,WM8994_POWER_MANAGEMENT_1,val);
}


void wm8994_set_playback_headset(struct snd_soc_codec *codec)
{
	struct wm8994_priv *wm8994 = codec->private_data;

	u16 val;
	
	u16 TestReturn1=NULL;
	u16 TestReturn2=NULL;
	u16 TestLow1=NULL;
	u16 TestHigh1=NULL;
	u8 TestLow=0;
	u8 TestHigh=0;
	
	DEBUG_LOG("");

	//Disable end point for preventing pop up noise.
	val = wm8994_read(codec, 0x102  ); 	
	val &= ~(0x0003);
	val |= 0x0003;
	wm8994_write(codec,0x102,val);
	
	val = wm8994_read(codec, 0x56  ); 	
	val &= ~(0x0003);
	val |= 0x0003;
	wm8994_write(codec,0x56,val);
	
	val = wm8994_read(codec, 0x102  ); 	
	val &= ~(0x0003);
	val |= 0x0003;
	wm8994_write(codec,0x102,0x0003);
	
	val = wm8994_read(codec, WM8994_CLASS_W_1  ); 	
	val &= ~(0x0005);
	val |= 0x0005;
	wm8994_write(codec,WM8994_CLASS_W_1,val);

	
	val = wm8994_read(codec, WM8994_DC_SERVO_2  ); 	
	val &= ~(0x03E0);
	val |= 0x03E0;
	wm8994_write(codec,WM8994_DC_SERVO_2,val);
	
	//Enable vmid,bias, hp left and right
	val = wm8994_read(codec,WM8994_POWER_MANAGEMENT_1 );
	val &= ~(WM8994_BIAS_ENA_MASK | WM8994_VMID_SEL_MASK |WM8994_HPOUT1L_ENA_MASK |WM8994_HPOUT1R_ENA_MASK);
	val |= (WM8994_BIAS_ENA | WM8994_VMID_SEL_NORMAL |WM8994_HPOUT1R_ENA	 |WM8994_HPOUT1L_ENA);  
	wm8994_write(codec,WM8994_POWER_MANAGEMENT_1,val);
			
	val = wm8994_read(codec, WM8994_ANALOGUE_HP_1  ); 	
	val &= ~(0x0022);
	val |= 0x0022;
	wm8994_write(codec,WM8994_ANALOGUE_HP_1,val);
	
	//Enable Charge Pump	
	val = wm8994_read(codec, WM8994_CHARGE_PUMP_1);
	val &= ~WM8994_CP_ENA_MASK ;
	val |= WM8994_CP_ENA | WM8994_CP_ENA_DEFAULT ; // this is from wolfson  	
	wm8994_write(codec, WM8994_CHARGE_PUMP_1, val);

	msleep(20);	// 5ms delay
	DEBUG_LOG("Sleep 20ms");	

	//Enable Dac1 and DAC2 and the Timeslot0 for AIF1	
	val = wm8994_read(codec, WM8994_POWER_MANAGEMENT_5 ); 	
	val &= ~(WM8994_DAC1R_ENA_MASK | WM8994_DAC1L_ENA_MASK|WM8994_AIF1DAC1R_ENA_MASK |  WM8994_AIF1DAC1L_ENA_MASK );
	val |= (WM8994_AIF1DAC1L_ENA | WM8994_AIF1DAC1R_ENA  | WM8994_DAC1L_ENA |WM8994_DAC1R_ENA );
	wm8994_write(codec, WM8994_POWER_MANAGEMENT_5 ,val);
			
	// Enable DAC1L to HPOUT1L path
	val = wm8994_read(codec, WM8994_OUTPUT_MIXER_1);
	val &=  ~(WM8994_DAC1L_TO_HPOUT1L_MASK | WM8994_DAC1L_TO_MIXOUTL_MASK);
	val |= WM8994_DAC1L_TO_MIXOUTL ;  	
	wm8994_write(codec,WM8994_OUTPUT_MIXER_1, val);


	// Enable DAC1R to HPOUT1R path
	val = wm8994_read(codec, WM8994_OUTPUT_MIXER_2);
	val &= ~(WM8994_DAC1R_TO_HPOUT1R_MASK | WM8994_DAC1R_TO_MIXOUTR_MASK);
	val |= WM8994_DAC1R_TO_MIXOUTR;
	wm8994_write(codec, WM8994_OUTPUT_MIXER_2, val);


	// Enable MIXOUT
	val = wm8994_read(codec, WM8994_POWER_MANAGEMENT_3);
	val &= ~(WM8994_MIXOUTLVOL_ENA_MASK | WM8994_MIXOUTRVOL_ENA_MASK | WM8994_MIXOUTL_ENA_MASK | WM8994_MIXOUTR_ENA_MASK);
	val |= (WM8994_MIXOUTLVOL_ENA | WM8994_MIXOUTRVOL_ENA | WM8994_MIXOUTL_ENA | WM8994_MIXOUTR_ENA);
	wm8994_write(codec, WM8994_POWER_MANAGEMENT_3, 0x0030);

	val = wm8994_read(codec, WM8994_DC_SERVO_1  ); 	
	val &= ~(0x0303);
	val |= 0x0303;
	wm8994_write(codec,WM8994_DC_SERVO_1,val);
	
	msleep(160);	// 160ms delay

	DEBUG_LOG("Sleep 160ms");	
	
	TestReturn1=wm8994_read(codec,WM8994_DC_SERVO_4);
	TestLow=(u8)(TestReturn1 & 0xff);
	TestHigh=(u8)((TestReturn1>>8) & 0xff);

	TestLow1=((u16)TestLow-5)&0x00ff;
	TestHigh1=(((u16)TestHigh-5)<<8)&0xff00;
	TestReturn2=TestLow1|TestHigh1;
	wm8994_write(codec,WM8994_DC_SERVO_4, TestReturn2);

	val = wm8994_read(codec, WM8994_DC_SERVO_1  ); 	
	val &= ~(0x000F);
	val |= 0x000F;
	wm8994_write(codec,WM8994_DC_SERVO_1,val);

	// Headset Control
	if(!wm8994->testmode_config_flag)
	{
	        val = wm8994_read(codec, WM8994_OUTPUT_MIXER_5);
		val &= ~(WM8994_DACL_MIXOUTL_VOL_MASK);
	        val |= TUNING_EAR_OUTMIX5_VOL << WM8994_DACL_MIXOUTL_VOL_SHIFT;
	        wm8994_write(codec,WM8994_OUTPUT_MIXER_5, val );
	
		val = wm8994_read(codec, WM8994_OUTPUT_MIXER_6);
		val &= ~(WM8994_DACR_MIXOUTR_VOL_MASK);
		val |= TUNING_EAR_OUTMIX6_VOL << WM8994_DACR_MIXOUTR_VOL_SHIFT;
		wm8994_write(codec,WM8994_OUTPUT_MIXER_6, val );

		val = wm8994_read(codec,WM8994_LEFT_OUTPUT_VOLUME);
		val &= ~(WM8994_HPOUT1L_MUTE_N_MASK | WM8994_HPOUT1L_VOL_MASK);
		val |= (WM8994_HPOUT1_VU | WM8994_HPOUT1L_MUTE_N | TUNING_MP3_OUTPUTL_VOL);
		wm8994_write(codec, WM8994_LEFT_OUTPUT_VOLUME, val);

		val = wm8994_read(codec,WM8994_RIGHT_OUTPUT_VOLUME);
		val &= ~(WM8994_HPOUT1R_MUTE_N_MASK | WM8994_HPOUT1R_VOL_MASK);
		val |= (WM8994_HPOUT1_VU | WM8994_HPOUT1R_MUTE_N | TUNING_MP3_OUTPUTR_VOL);
		wm8994_write(codec, WM8994_RIGHT_OUTPUT_VOLUME, val);

		val = wm8994_read(codec, WM8994_LEFT_OPGA_VOLUME);
		val &= ~(WM8994_MIXOUTL_MUTE_N_MASK | WM8994_MIXOUTL_VOL_MASK);
		val |= (WM8994_MIXOUT_VU | WM8994_MIXOUTL_MUTE_N | TUNING_MP3_OPGAL_VOL);
		wm8994_write(codec,WM8994_LEFT_OPGA_VOLUME, val );
	
		val = wm8994_read(codec, WM8994_RIGHT_OPGA_VOLUME);
		val &= ~(WM8994_MIXOUTR_MUTE_N_MASK | WM8994_MIXOUTR_VOL_MASK);
		val |= (WM8994_MIXOUT_VU | WM8994_MIXOUTR_MUTE_N | TUNING_MP3_OPGAR_VOL);
		wm8994_write(codec,WM8994_RIGHT_OPGA_VOLUME, val );
	
	
	}

	//Configuring the Digital Paths
	// Unmute the AF1DAC1	
	val = wm8994_read(codec, WM8994_AIF1_DAC1_FILTERS_1 ); 	
	val &= ~(WM8994_AIF1DAC1_MUTE_MASK);
	val |= WM8994_AIF1DAC1_UNMUTE;
	wm8994_write(codec,WM8994_AIF1_DAC1_FILTERS_1 ,val);
	
	msleep(20);
	DEBUG_LOG("Sleep 20ms");	

	
	// Intermediate HP settings
	val = wm8994_read(codec, WM8994_ANALOGUE_HP_1); 	
	val &= ~(WM8994_HPOUT1R_DLY_MASK |WM8994_HPOUT1R_OUTP_MASK |WM8994_HPOUT1R_RMV_SHORT_MASK |
		WM8994_HPOUT1L_DLY_MASK |WM8994_HPOUT1L_OUTP_MASK | WM8994_HPOUT1L_RMV_SHORT_MASK);
	val |= (WM8994_HPOUT1L_RMV_SHORT | WM8994_HPOUT1L_OUTP|WM8994_HPOUT1L_DLY |WM8994_HPOUT1R_RMV_SHORT | 
		WM8994_HPOUT1R_OUTP | WM8994_HPOUT1R_DLY);
	wm8994_write(codec, WM8994_ANALOGUE_HP_1, val);
	
	// Enable the Timeslot0 to DAC1L
	val = wm8994_read(codec, WM8994_DAC1_LEFT_MIXER_ROUTING  ); 	
	val &= ~(WM8994_AIF1DAC1L_TO_DAC1L_MASK);
	val |= WM8994_AIF1DAC1L_TO_DAC1L;
	wm8994_write(codec,WM8994_DAC1_LEFT_MIXER_ROUTING ,val);


	//Enable the Timeslot0 to DAC1R
	val = wm8994_read(codec, WM8994_DAC1_RIGHT_MIXER_ROUTING  ); 	
	val &= ~(WM8994_AIF1DAC1R_TO_DAC1R_MASK);
	val |= WM8994_AIF1DAC1R_TO_DAC1R;
	wm8994_write(codec,WM8994_DAC1_RIGHT_MIXER_ROUTING ,val);
		
	//Unmute DAC1 left
	val = wm8994_read(codec,WM8994_DAC1_LEFT_VOLUME );
	val &= ~(WM8994_DAC1L_MUTE_MASK | WM8994_DAC1L_VOL_MASK);
	val |= TUNING_DAC1L_VOL; 
	wm8994_write(codec,WM8994_DAC1_LEFT_VOLUME ,val);
	
	//Unmute and volume ctrl RightDAC
	val = wm8994_read(codec, WM8994_DAC1_RIGHT_VOLUME ); 
	val &= ~(WM8994_DAC1R_MUTE_MASK | WM8994_DAC1R_VOL_MASK);
	val |= TUNING_DAC1R_VOL; //0 db volume	
	wm8994_write(codec,WM8994_DAC1_RIGHT_VOLUME,val);
	
		
}

void wm8994_set_playback_speaker(struct snd_soc_codec *codec)
{
	struct wm8994_priv *wm8994 = codec->private_data;

	u16 val;

	DEBUG_LOG("");

#ifdef STEREO_SPEAKER_SUPPORT
	//Disable end point for preventing pop up noise.
	val = wm8994_read(codec,WM8994_POWER_MANAGEMENT_1);
	val &= ~(WM8994_SPKOUTL_ENA_MASK|WM8994_SPKOUTR_ENA_MASK);
	wm8994_write(codec, WM8994_POWER_MANAGEMENT_1, val);

	val = wm8994_read(codec, WM8994_POWER_MANAGEMENT_3);
	val &= ~(WM8994_SPKLVOL_ENA_MASK|WM8994_SPKRVOL_ENA_MASK);
	val |= (WM8994_SPKLVOL_ENA|WM8994_SPKRVOL_ENA);
	wm8994_write(codec, WM8994_POWER_MANAGEMENT_3, val);

	// Speaker Volume Control
	if(!wm8994->testmode_config_flag)
	{
		// Unmute the SPKMIXVOLUME
		val = wm8994_read(codec, WM8994_SPKMIXL_ATTENUATION);
		val &= ~(WM8994_SPKMIXL_VOL_MASK);
		val |= TUNING_SPKMIXL_ATTEN;	
		wm8994_write(codec, WM8994_SPKMIXL_ATTENUATION, val);
		
		val = wm8994_read(codec,WM8994_SPKMIXR_ATTENUATION);
		val &= ~(WM8994_SPKMIXR_VOL_MASK);
		val |= TUNING_SPKMIXR_ATTEN;
		wm8994_write(codec, WM8994_SPKMIXR_ATTENUATION, val);

		val = wm8994_read(codec,WM8994_SPEAKER_VOLUME_LEFT);
		val &= ~(WM8994_SPKOUTL_MUTE_N_MASK | WM8994_SPKOUTL_VOL_MASK);
		val |= (WM8994_SPKOUT_VU | WM8994_SPKOUTL_MUTE_N | TUNING_MP3_SPKL_VOL);
		wm8994_write(codec, WM8994_SPEAKER_VOLUME_LEFT, val);

		val = wm8994_read(codec,WM8994_SPEAKER_VOLUME_RIGHT);
		val &= ~(WM8994_SPKOUTR_MUTE_N_MASK | WM8994_SPKOUTR_VOL_MASK);
		val |= (WM8994_SPKOUT_VU | WM8994_SPKOUTR_MUTE_N | TUNING_MP3_SPKR_VOL);
		wm8994_write(codec, WM8994_SPEAKER_VOLUME_RIGHT, val);

		val = wm8994_read(codec,WM8994_CLASSD);
		val &= ~(WM8994_SPKOUTL_BOOST_MASK | WM8994_SPKOUTR_BOOST_MASK);
		val |= (TUNING_MP3_CLASSD_VOL << WM8994_SPKOUTL_BOOST_SHIFT)
           |(TUNING_MP3_CLASSD_VOL << WM8994_SPKOUTR_BOOST_SHIFT);
		wm8994_write(codec, WM8994_CLASSD, val);
	
		//Unmute DAC1 left
		val = wm8994_read(codec,WM8994_DAC1_LEFT_VOLUME );
		val &= ~(WM8994_DAC1L_MUTE_MASK | WM8994_DAC1L_VOL_MASK);
		val |=   TUNING_DAC1L_VOL; 
		wm8994_write(codec,WM8994_DAC1_LEFT_VOLUME ,val);
	
		//Unmute and volume ctrl RightDAC
		val = wm8994_read(codec, WM8994_DAC1_RIGHT_VOLUME ); 
		val &= ~(WM8994_DAC1R_MUTE_MASK | WM8994_DAC1R_VOL_MASK);
		val |=   TUNING_DAC1R_VOL; 
		wm8994_write(codec,WM8994_DAC1_RIGHT_VOLUME,val);
	}

	val = wm8994_read(codec,WM8994_SPKOUT_MIXERS);
	val &= ~(WM8994_SPKMIXL_TO_SPKOUTL_MASK|WM8994_SPKMIXR_TO_SPKOUTL_MASK 
           |WM8994_SPKMIXR_TO_SPKOUTR_MASK);
	val |=  (WM8994_SPKMIXL_TO_SPKOUTL|WM8994_SPKMIXR_TO_SPKOUTR);
	wm8994_write(codec, WM8994_SPKOUT_MIXERS, val);

	//Unmute the DAC path
	val = wm8994_read(codec,WM8994_SPEAKER_MIXER);
	val &= ~(WM8994_DAC1L_TO_SPKMIXL_MASK|WM8994_DAC1R_TO_SPKMIXR_MASK);
	val |=  (WM8994_DAC1L_TO_SPKMIXL|WM8994_DAC1R_TO_SPKMIXR);
	wm8994_write(codec, WM8994_SPEAKER_MIXER, val);

	// Eable DAC1 Left and Right timeslot 0.
	val = wm8994_read(codec,WM8994_POWER_MANAGEMENT_5);	
	val &= ~( WM8994_DAC1L_ENA_MASK|WM8994_DAC1R_ENA_MASK 
           |WM8994_AIF1DAC1L_ENA_MASK|WM8994_AIF1DAC1R_ENA_MASK);
	val |=  ( WM8994_AIF1DAC1L_ENA|WM8994_AIF1DAC1R_ENA 
           |WM8994_DAC1L_ENA|WM8994_DAC1R_ENA);
	wm8994_write(codec, WM8994_POWER_MANAGEMENT_5, val);   

	//Unmute DAC1
	val = wm8994_read(codec, WM8994_AIF1_DAC1_FILTERS_1);
	val &= ~(WM8994_AIF1DAC1_MUTE_MASK);
	val |= WM8994_AIF1DAC1_UNMUTE;
	wm8994_write(codec, WM8994_AIF1_DAC1_FILTERS_1, val);

	//enable timeslot0 to Left DAC
	val = wm8994_read(codec, WM8994_DAC1_LEFT_MIXER_ROUTING);
	val &= ~(WM8994_AIF1DAC1L_TO_DAC1L_MASK);
	val |=   WM8994_AIF1DAC1L_TO_DAC1L;
	wm8994_write(codec, WM8994_DAC1_LEFT_MIXER_ROUTING, val);
	
	//enable timeslot0 to Right DAC
	val = wm8994_read(codec, WM8994_DAC1_RIGHT_MIXER_ROUTING);
	val &= ~(WM8994_AIF1DAC1R_TO_DAC1R_MASK);
	val |=   WM8994_AIF1DAC1R_TO_DAC1R;
	wm8994_write(codec, WM8994_DAC1_RIGHT_MIXER_ROUTING, val);

	//Enbale bias,vmid and Left/Right speaker
	val = wm8994_read(codec,WM8994_POWER_MANAGEMENT_1);
	val &= ~( WM8994_BIAS_ENA_MASK|WM8994_VMID_SEL_MASK
           |WM8994_SPKOUTL_ENA_MASK|WM8994_SPKOUTR_ENA_MASK);
	val |=  ( WM8994_BIAS_ENA|WM8994_VMID_SEL_NORMAL 
           |WM8994_SPKOUTL_ENA|WM8994_SPKOUTR_ENA);  
	wm8994_write(codec, WM8994_POWER_MANAGEMENT_1, val);

#else //STEREO_SPEAKER_SUPPORT

	//Disable end point for preventing pop up noise.
	val = wm8994_read(codec,WM8994_POWER_MANAGEMENT_1);
	val &= ~(WM8994_SPKOUTL_ENA_MASK);
	wm8994_write(codec, WM8994_POWER_MANAGEMENT_1, val);

	val = wm8994_read(codec, WM8994_POWER_MANAGEMENT_3);
	val &= ~(WM8994_SPKLVOL_ENA_MASK);
	val |= WM8994_SPKLVOL_ENA;
	wm8994_write(codec, WM8994_POWER_MANAGEMENT_3, val);

	// Speaker Volume Control
	if(!wm8994->testmode_config_flag)
	{
		// Unmute the SPKMIXVOLUME
		val = wm8994_read(codec, WM8994_SPKMIXL_ATTENUATION);
		val &= ~(WM8994_SPKMIXL_VOL_MASK);
		val |= TUNING_SPKMIXL_ATTEN;	
		wm8994_write(codec, WM8994_SPKMIXL_ATTENUATION, val);
		
		val = wm8994_read(codec,WM8994_SPKMIXR_ATTENUATION);
		val &= ~(WM8994_SPKMIXR_VOL_MASK);
		wm8994_write(codec, WM8994_SPKMIXR_ATTENUATION, val);

		val = wm8994_read(codec,WM8994_SPEAKER_VOLUME_LEFT);
		val &= ~(WM8994_SPKOUTL_MUTE_N_MASK | WM8994_SPKOUTL_VOL_MASK);
		val |= (WM8994_SPKOUT_VU | WM8994_SPKOUTL_MUTE_N | TUNING_MP3_SPKL_VOL);
		wm8994_write(codec, WM8994_SPEAKER_VOLUME_LEFT, val);

		val = wm8994_read(codec,WM8994_SPEAKER_VOLUME_RIGHT);
		val &= ~(WM8994_SPKOUTR_MUTE_N_MASK | WM8994_SPKOUTR_VOL_MASK);
		wm8994_write(codec, WM8994_SPEAKER_VOLUME_RIGHT, val);

		val = wm8994_read(codec,WM8994_CLASSD);
		val &= ~(WM8994_SPKOUTL_BOOST_MASK);
		val |= TUNING_MP3_CLASSD_VOL << WM8994_SPKOUTL_BOOST_SHIFT;
		wm8994_write(codec, WM8994_CLASSD, val);
	
		//Unmute DAC1 left
		val = wm8994_read(codec,WM8994_DAC1_LEFT_VOLUME );
		val &= ~(WM8994_DAC1L_MUTE_MASK | WM8994_DAC1L_VOL_MASK);
		val |= TUNING_DAC1L_VOL; 
		wm8994_write(codec,WM8994_DAC1_LEFT_VOLUME ,val);
	
		//Unmute and volume ctrl RightDAC
		val = wm8994_read(codec, WM8994_DAC1_RIGHT_VOLUME ); 
		val &= ~(WM8994_DAC1R_MUTE_MASK | WM8994_DAC1R_VOL_MASK);
		val |= TUNING_DAC1R_VOL; //0 db volume	
		wm8994_write(codec,WM8994_DAC1_RIGHT_VOLUME,val);
	}

	val = wm8994_read(codec,WM8994_SPKOUT_MIXERS);
	val &= ~(WM8994_SPKMIXL_TO_SPKOUTL_MASK | 
		WM8994_SPKMIXR_TO_SPKOUTL_MASK | WM8994_SPKMIXR_TO_SPKOUTR_MASK);
	val |= WM8994_SPKMIXL_TO_SPKOUTL;
	wm8994_write(codec, WM8994_SPKOUT_MIXERS, val);

	//Unmute the DAC path
	val = wm8994_read(codec,WM8994_SPEAKER_MIXER);
	val &= ~(WM8994_DAC1L_TO_SPKMIXL_MASK);
	val |= WM8994_DAC1L_TO_SPKMIXL;
	wm8994_write(codec, WM8994_SPEAKER_MIXER, val);

	// Eable DAC1 Left and timeslot left
	val = wm8994_read(codec,WM8994_POWER_MANAGEMENT_5);	
	val &= ~( WM8994_DAC1L_ENA_MASK | WM8994_AIF1DAC1L_ENA_MASK);
	val |= (WM8994_AIF1DAC1L_ENA | WM8994_DAC1L_ENA);
	wm8994_write(codec, WM8994_POWER_MANAGEMENT_5, val);   

	//Unmute
	val = wm8994_read(codec, WM8994_AIF1_DAC1_FILTERS_1);
	val &= ~(WM8994_AIF1DAC1_MUTE_MASK);
	val |= WM8994_AIF1DAC1_UNMUTE;
	wm8994_write(codec, WM8994_AIF1_DAC1_FILTERS_1, val);

	//enable timeslot0 to left dac
	val = wm8994_read(codec, WM8994_DAC1_LEFT_MIXER_ROUTING);
	val &= ~(WM8994_AIF1DAC1L_TO_DAC1L_MASK);
	val |= WM8994_AIF1DAC1L_TO_DAC1L;
	wm8994_write(codec, WM8994_DAC1_LEFT_MIXER_ROUTING, val);
	
	//Enbale bias,vmid and Left speaker
	val = wm8994_read(codec,WM8994_POWER_MANAGEMENT_1);
	val &= ~(WM8994_BIAS_ENA_MASK | WM8994_VMID_SEL_MASK |WM8994_SPKOUTL_ENA_MASK);
	val |= (WM8994_BIAS_ENA | WM8994_VMID_SEL_NORMAL | WM8994_SPKOUTL_ENA);  
	wm8994_write(codec, WM8994_POWER_MANAGEMENT_1, val);
#endif //STEREO_SPEAKER_SUPPORT
}

void wm8994_set_playback_speaker_headset(struct snd_soc_codec *codec)
{
	struct wm8994_priv *wm8994 = codec->private_data;

	u16 val;
	int ret;

	DEBUG_LOG("testmode_config_flag = [%d]",wm8994->testmode_config_flag);

#ifdef STEREO_SPEAKER_SUPPORT
	//Disable end point for preventing pop up noise.
	val = wm8994_read(codec, WM8994_POWER_MANAGEMENT_1);
	val &= ~(WM8994_HPOUT1L_ENA_MASK |WM8994_HPOUT1R_ENA_MASK|WM8994_SPKOUTL_ENA_MASK);
	ret = wm8994_write(codec,WM8994_POWER_MANAGEMENT_1,val);

	//------------------  Ear Path Settings ------------------

	// Headset Control
	if(!wm8994->testmode_config_flag)
	{
    val = wm8994_read(codec, WM8994_OUTPUT_MIXER_5);
    val &= ~(WM8994_DACL_MIXOUTL_VOL_MASK);
    val |= TUNING_EAR_OUTMIX5_VOL << WM8994_DACL_MIXOUTL_VOL_SHIFT;
    wm8994_write(codec,WM8994_OUTPUT_MIXER_5, val );
	
		val = wm8994_read(codec, WM8994_OUTPUT_MIXER_6);
		val &= ~(WM8994_DACR_MIXOUTR_VOL_MASK);
		val |= TUNING_EAR_OUTMIX6_VOL << WM8994_DACR_MIXOUTR_VOL_SHIFT;
		wm8994_write(codec,WM8994_OUTPUT_MIXER_6, val );

		val = wm8994_read(codec,WM8994_LEFT_OUTPUT_VOLUME);
		val &= ~(WM8994_HPOUT1L_MUTE_N_MASK | WM8994_HPOUT1L_VOL_MASK);
		val |= (WM8994_HPOUT1_VU | WM8994_HPOUT1L_MUTE_N | TUNING_MP3_OUTPUTR_VOL);
		wm8994_write(codec, WM8994_LEFT_OUTPUT_VOLUME, val);

		val = wm8994_read(codec,WM8994_RIGHT_OUTPUT_VOLUME);
		val &= ~(WM8994_HPOUT1R_MUTE_N_MASK | WM8994_HPOUT1R_VOL_MASK);
		val |= (WM8994_HPOUT1R_MUTE_N | TUNING_MP3_OUTPUTR_VOL);
		wm8994_write(codec, WM8994_RIGHT_OUTPUT_VOLUME, val);
	}

	// Enbale DAC1L to HPOUT1L path
	val = wm8994_read(codec, WM8994_OUTPUT_MIXER_1);
	val &=  ~(WM8994_DAC1L_TO_HPOUT1L_MASK | WM8994_DAC1L_TO_MIXOUTL_MASK);
	val |=  WM8994_DAC1L_TO_MIXOUTL;
	wm8994_write(codec,WM8994_OUTPUT_MIXER_1, val);

	// Enbale DAC1R to HPOUT1R path
	val = wm8994_read(codec, WM8994_OUTPUT_MIXER_2);
	val &= ~(WM8994_DAC1R_TO_HPOUT1R_MASK | WM8994_DAC1R_TO_MIXOUTR_MASK);
	val |= WM8994_DAC1R_TO_MIXOUTR;
	wm8994_write(codec, WM8994_OUTPUT_MIXER_2, val);

	//Enable Charge Pump	
	val = wm8994_read(codec, WM8994_CHARGE_PUMP_1);
	val &= ~WM8994_CP_ENA_MASK ;
	val = WM8994_CP_ENA | WM8994_CP_ENA_DEFAULT ; // this is from wolfson  	
	wm8994_write(codec, WM8994_CHARGE_PUMP_1, val);

	// Intermediate HP settings
	val = wm8994_read(codec, WM8994_ANALOGUE_HP_1); 	
	val &= ~(WM8994_HPOUT1R_DLY_MASK |WM8994_HPOUT1R_OUTP_MASK |WM8994_HPOUT1R_RMV_SHORT_MASK |
		WM8994_HPOUT1L_DLY_MASK |WM8994_HPOUT1L_OUTP_MASK | WM8994_HPOUT1L_RMV_SHORT_MASK);
	val |= (WM8994_HPOUT1L_RMV_SHORT | WM8994_HPOUT1L_OUTP|WM8994_HPOUT1L_DLY |WM8994_HPOUT1R_RMV_SHORT | 
		WM8994_HPOUT1R_OUTP | WM8994_HPOUT1R_DLY);
	wm8994_write(codec, WM8994_ANALOGUE_HP_1, val);
	

	//------------------  Speaker Path Settings ------------------

	// Speaker Volume Control
	if(!wm8994->testmode_config_flag)
	{
		// Unmute the SPKMIXVOLUME
		val = wm8994_read(codec, WM8994_SPKMIXL_ATTENUATION);
		val &= ~(WM8994_SPKMIXL_VOL_MASK);
		val |= TUNING_SPKMIXL_ATTEN;	
		wm8994_write(codec, WM8994_SPKMIXL_ATTENUATION, val);
		
		val = wm8994_read(codec,WM8994_SPKMIXR_ATTENUATION);
		val &= ~(WM8994_SPKMIXR_VOL_MASK);
		val |= TUNING_SPKMIXR_ATTEN;
		wm8994_write(codec, WM8994_SPKMIXR_ATTENUATION, val);

		val = wm8994_read(codec,WM8994_SPEAKER_VOLUME_LEFT);
		val &= ~(WM8994_SPKOUTL_MUTE_N_MASK | WM8994_SPKOUTL_VOL_MASK);
		val |= (WM8994_SPKOUT_VU | WM8994_SPKOUTL_MUTE_N | TUNING_MP3_SPKL_VOL);
		wm8994_write(codec, WM8994_SPEAKER_VOLUME_LEFT, val);

		val = wm8994_read(codec,WM8994_SPEAKER_VOLUME_RIGHT);
		val &= ~(WM8994_SPKOUTR_MUTE_N_MASK | WM8994_SPKOUTR_VOL_MASK);
		val |= (WM8994_SPKOUT_VU | WM8994_SPKOUTR_MUTE_N | TUNING_MP3_SPKR_VOL);
		wm8994_write(codec, WM8994_SPEAKER_VOLUME_RIGHT, val);

		val = wm8994_read(codec,WM8994_CLASSD);
		val &= ~(WM8994_SPKOUTL_BOOST_MASK | WM8994_SPKOUTR_BOOST_MASK);
		val |= (TUNING_MP3_CLASSD_VOL << WM8994_SPKOUTL_BOOST_SHIFT)
           |(TUNING_MP3_CLASSD_VOL << WM8994_SPKOUTR_BOOST_SHIFT);
		wm8994_write(codec, WM8994_CLASSD, val);
	
		//Unmute DAC1 left
		val = wm8994_read(codec,WM8994_DAC1_LEFT_VOLUME );
		val &= ~(WM8994_DAC1L_MUTE_MASK | WM8994_DAC1L_VOL_MASK);
		val |=   TUNING_DAC1L_VOL; 
		wm8994_write(codec,WM8994_DAC1_LEFT_VOLUME ,val);
	
		//Unmute and volume ctrl RightDAC
		val = wm8994_read(codec, WM8994_DAC1_RIGHT_VOLUME ); 
		val &= ~(WM8994_DAC1R_MUTE_MASK | WM8994_DAC1R_VOL_MASK);
		val |=   TUNING_DAC1R_VOL; 
		wm8994_write(codec,WM8994_DAC1_RIGHT_VOLUME,val);
	}

	val = wm8994_read(codec,WM8994_SPKOUT_MIXERS);
	val &= ~(WM8994_SPKMIXL_TO_SPKOUTL_MASK|WM8994_SPKMIXR_TO_SPKOUTL_MASK 
           |WM8994_SPKMIXR_TO_SPKOUTR_MASK);
	val |=  (WM8994_SPKMIXL_TO_SPKOUTL|WM8994_SPKMIXR_TO_SPKOUTR);
	wm8994_write(codec, WM8994_SPKOUT_MIXERS, val);

	//Unmute the DAC path
	val = wm8994_read(codec,WM8994_SPEAKER_MIXER);
	val &= ~(WM8994_DAC1L_TO_SPKMIXL_MASK|WM8994_DAC1R_TO_SPKMIXR_MASK);
	val |=  (WM8994_DAC1L_TO_SPKMIXL|WM8994_DAC1R_TO_SPKMIXR);
	wm8994_write(codec, WM8994_SPEAKER_MIXER, val);

	//------------------  Common Settings ------------------

	//Configuring the Digital Paths

	//Enable DAC1 and DAC2 and the Timeslot0 for AIF1	
	val = wm8994_read(codec,WM8994_POWER_MANAGEMENT_5 );	
	val &= ~( WM8994_DAC1R_ENA_MASK|WM8994_DAC1L_ENA_MASK
           |WM8994_AIF1DAC1R_ENA_MASK|WM8994_AIF1DAC1L_ENA_MASK );
	val |=  ( WM8994_AIF1DAC1L_ENA|WM8994_AIF1DAC1R_ENA|WM8994_DAC1L_ENA|WM8994_DAC1R_ENA );
	wm8994_write(codec,WM8994_POWER_MANAGEMENT_5,val);   

	// Unmute the AIF1DAC1	
	val = wm8994_read(codec, WM8994_AIF1_DAC1_FILTERS_1 ); 	
	val &= ~(WM8994_AIF1DAC1_MUTE_MASK);
	val |= WM8994_AIF1DAC1_UNMUTE ;
	wm8994_write(codec,WM8994_AIF1_DAC1_FILTERS_1 ,val);

	// Enable the Timeslot0 to DAC1L
	val = wm8994_read(codec, WM8994_DAC1_LEFT_MIXER_ROUTING  ); 	
	val &= ~(WM8994_AIF1DAC1L_TO_DAC1L_MASK);
	val |= WM8994_AIF1DAC1L_TO_DAC1L;
	wm8994_write(codec,WM8994_DAC1_LEFT_MIXER_ROUTING ,val);

	//Enable the Timeslot0 to DAC1R
	val = wm8994_read(codec, WM8994_DAC1_RIGHT_MIXER_ROUTING  ); 	
	val &= ~(WM8994_AIF1DAC1R_TO_DAC1R_MASK);
	val |= WM8994_AIF1DAC1R_TO_DAC1R;
	wm8994_write(codec,WM8994_DAC1_RIGHT_MIXER_ROUTING ,val);
	
  //Enable Speaker Volume(Left/Right)
	val = wm8994_read(codec, WM8994_POWER_MANAGEMENT_3);
	val &= ~(WM8994_SPKLVOL_ENA_MASK|WM8994_SPKRVOL_ENA_MASK);
	val |=  (WM8994_SPKLVOL_ENA|WM8994_SPKRVOL_ENA);
	wm8994_write(codec, WM8994_POWER_MANAGEMENT_3 ,val);

	//Enbale bias,vmid, hp left and right and Left/Right speaker
	val = wm8994_read(codec, WM8994_POWER_MANAGEMENT_1);
	val &= ~( WM8994_BIAS_ENA_MASK|WM8994_VMID_SEL_MASK
		       |WM8994_HPOUT1L_ENA_MASK|WM8994_HPOUT1R_ENA_MASK
           |WM8994_SPKOUTL_ENA_MASK|WM8994_SPKOUTR_ENA_MASK);
	val |=  ( WM8994_BIAS_ENA|WM8994_VMID_SEL_NORMAL
           |WM8994_HPOUT1R_ENA|WM8994_HPOUT1L_ENA
           |WM8994_SPKOUTL_ENA|WM8994_SPKOUTR_ENA);
	ret = wm8994_write(codec,WM8994_POWER_MANAGEMENT_1,val);

#else //STEREO_SPEAKER_SUPPORT

	//Disable end point for preventing pop up noise.
	val = wm8994_read(codec, WM8994_POWER_MANAGEMENT_1);
	val &= ~(WM8994_HPOUT1L_ENA_MASK |WM8994_HPOUT1R_ENA_MASK|WM8994_SPKOUTL_ENA_MASK);
	ret = wm8994_write(codec,WM8994_POWER_MANAGEMENT_1,val);

	//------------------  Ear Path Settings ------------------

	// Headset Control
	if(!wm8994->testmode_config_flag)
	{
	        val = wm8994_read(codec, WM8994_OUTPUT_MIXER_5);
		val &= ~(WM8994_DACL_MIXOUTL_VOL_MASK);
	        val |= TUNING_EAR_OUTMIX5_VOL << WM8994_DACL_MIXOUTL_VOL_SHIFT;
	        wm8994_write(codec,WM8994_OUTPUT_MIXER_5, val );
	
		val = wm8994_read(codec, WM8994_OUTPUT_MIXER_6);
		val &= ~(WM8994_DACR_MIXOUTR_VOL_MASK);
		val |= TUNING_EAR_OUTMIX6_VOL << WM8994_DACR_MIXOUTR_VOL_SHIFT;
		wm8994_write(codec,WM8994_OUTPUT_MIXER_6, val );

		val = wm8994_read(codec,WM8994_LEFT_OUTPUT_VOLUME);
		val &= ~(WM8994_HPOUT1L_MUTE_N_MASK | WM8994_HPOUT1L_VOL_MASK);
		val |= (WM8994_HPOUT1_VU | WM8994_HPOUT1L_MUTE_N | TUNING_DUAL_OUTPUTL_VOL);
		wm8994_write(codec, WM8994_LEFT_OUTPUT_VOLUME, val);

		val = wm8994_read(codec,WM8994_RIGHT_OUTPUT_VOLUME);
		val &= ~(WM8994_HPOUT1R_MUTE_N_MASK | WM8994_HPOUT1R_VOL_MASK);
		val |= (WM8994_HPOUT1_VU | WM8994_HPOUT1R_MUTE_N | TUNING_DUAL_OUTPUTR_VOL);
		wm8994_write(codec, WM8994_RIGHT_OUTPUT_VOLUME, val);
	}

	// Enbale DAC1L to HPOUT1L path
	val = wm8994_read(codec, WM8994_OUTPUT_MIXER_1);
	val &=  ~(WM8994_DAC1L_TO_HPOUT1L_MASK | WM8994_DAC1L_TO_MIXOUTL_MASK);
	val |=  WM8994_DAC1L_TO_MIXOUTL;
	wm8994_write(codec,WM8994_OUTPUT_MIXER_1, val);

	// Enbale DAC1R to HPOUT1R path
	val = wm8994_read(codec, WM8994_OUTPUT_MIXER_2);
	val &= ~(WM8994_DAC1R_TO_HPOUT1R_MASK | WM8994_DAC1R_TO_MIXOUTR_MASK);
	val |= WM8994_DAC1R_TO_MIXOUTR;
	wm8994_write(codec, WM8994_OUTPUT_MIXER_2, val);

	//Enable Charge Pump	
	val = wm8994_read(codec, WM8994_CHARGE_PUMP_1);
	val &= ~WM8994_CP_ENA_MASK ;
	val = WM8994_CP_ENA | WM8994_CP_ENA_DEFAULT ; // this is from wolfson  	
	wm8994_write(codec, WM8994_CHARGE_PUMP_1, val);

	// Intermediate HP settings
	val = wm8994_read(codec, WM8994_ANALOGUE_HP_1); 	
	val &= ~(WM8994_HPOUT1R_DLY_MASK |WM8994_HPOUT1R_OUTP_MASK |WM8994_HPOUT1R_RMV_SHORT_MASK |
		WM8994_HPOUT1L_DLY_MASK |WM8994_HPOUT1L_OUTP_MASK | WM8994_HPOUT1L_RMV_SHORT_MASK);
	val |= (WM8994_HPOUT1L_RMV_SHORT | WM8994_HPOUT1L_OUTP|WM8994_HPOUT1L_DLY |WM8994_HPOUT1R_RMV_SHORT | 
		WM8994_HPOUT1R_OUTP | WM8994_HPOUT1R_DLY);
	wm8994_write(codec, WM8994_ANALOGUE_HP_1, val);
	
	//Configuring the Digital Paths
	
	//Enable the Timeslot0 to DAC1R
	val = wm8994_read(codec, WM8994_DAC1_RIGHT_MIXER_ROUTING  ); 	
	val &= ~(WM8994_AIF1DAC1R_TO_DAC1R_MASK);
	val |= WM8994_AIF1DAC1R_TO_DAC1R;
	wm8994_write(codec,WM8994_DAC1_RIGHT_MIXER_ROUTING ,val);

	//------------------  Speaker Path Settings ------------------

	// Speaker Volume Control
	if(!wm8994->testmode_config_flag)
	{
		// Unmute the SPKMIXVOLUME
		val = wm8994_read(codec, WM8994_SPKMIXL_ATTENUATION);
		val &= ~(WM8994_SPKMIXL_VOL_MASK);
		val |= TUNING_SPKMIXL_ATTEN;	
		wm8994_write(codec, WM8994_SPKMIXL_ATTENUATION, val);
		
		val = wm8994_read(codec,WM8994_SPKMIXR_ATTENUATION);
		val &= ~(WM8994_SPKMIXR_VOL_MASK);
		wm8994_write(codec, WM8994_SPKMIXR_ATTENUATION, val);

		val = wm8994_read(codec,WM8994_SPEAKER_VOLUME_LEFT);
		val &= ~(WM8994_SPKOUTL_MUTE_N_MASK | WM8994_SPKOUTL_VOL_MASK);
		val |= (WM8994_SPKOUTL_MUTE_N | TUNING_MP3_SPKL_VOL);
		wm8994_write(codec, WM8994_SPEAKER_VOLUME_LEFT, val);

		val = wm8994_read(codec,WM8994_SPEAKER_VOLUME_RIGHT);
		val &= ~(WM8994_SPKOUTR_MUTE_N_MASK | WM8994_SPKOUTR_VOL_MASK);
		wm8994_write(codec, WM8994_SPEAKER_VOLUME_RIGHT, val);

		val = wm8994_read(codec,WM8994_CLASSD);
		val &= ~(WM8994_SPKOUTL_BOOST_MASK);
		val |= TUNING_MP3_CLASSD_VOL << WM8994_SPKOUTL_BOOST_SHIFT;
		wm8994_write(codec, WM8994_CLASSD, val);
	
		//Unmute DAC1 left
		val = wm8994_read(codec,WM8994_DAC1_LEFT_VOLUME );
		val &= ~(WM8994_DAC1L_MUTE_MASK | WM8994_DAC1L_VOL_MASK);
		val |= TUNING_DAC1L_VOL; 
		wm8994_write(codec,WM8994_DAC1_LEFT_VOLUME ,val);
	
		//Unmute and volume ctrl RightDAC
		val = wm8994_read(codec, WM8994_DAC1_RIGHT_VOLUME ); 
		val &= ~(WM8994_DAC1R_MUTE_MASK | WM8994_DAC1R_VOL_MASK);
		val |= TUNING_DAC1R_VOL; //0 db volume	
		wm8994_write(codec,WM8994_DAC1_RIGHT_VOLUME,val);
	}

	val = wm8994_read(codec,WM8994_SPKOUT_MIXERS );
	val &= ~(WM8994_SPKMIXL_TO_SPKOUTL_MASK | 
		WM8994_SPKMIXR_TO_SPKOUTL_MASK | WM8994_SPKMIXR_TO_SPKOUTR_MASK);
	val |= WM8994_SPKMIXL_TO_SPKOUTL;
	wm8994_write(codec,WM8994_SPKOUT_MIXERS,val );

	//Unmute the DAC path
	val = wm8994_read(codec,WM8994_SPEAKER_MIXER );
	val &= ~(WM8994_DAC1L_TO_SPKMIXL_MASK );
	val |= WM8994_DAC1L_TO_SPKMIXL ;
	wm8994_write(codec,WM8994_SPEAKER_MIXER ,val);

	//------------------  Common Settings ------------------

	//Enable DAC1 and DAC2 and the Timeslot0 for AIF1	
	val = wm8994_read(codec,WM8994_POWER_MANAGEMENT_5 );	
	val &= ~(WM8994_DAC1R_ENA_MASK | WM8994_DAC1L_ENA_MASK|WM8994_AIF1DAC1R_ENA_MASK |  WM8994_AIF1DAC1L_ENA_MASK );
	val |= (WM8994_AIF1DAC1L_ENA | WM8994_AIF1DAC1R_ENA  | WM8994_DAC1L_ENA |WM8994_DAC1R_ENA );
	wm8994_write(codec,WM8994_POWER_MANAGEMENT_5,val);   

	// Unmute the AIF1DAC1	
	val = wm8994_read(codec, WM8994_AIF1_DAC1_FILTERS_1 ); 	
	val &= ~(WM8994_AIF1DAC1_MUTE_MASK);
	val |= WM8994_AIF1DAC1_UNMUTE ;
	wm8994_write(codec,WM8994_AIF1_DAC1_FILTERS_1 ,val);

	// Enable the Timeslot0 to DAC1L
	val = wm8994_read(codec, WM8994_DAC1_LEFT_MIXER_ROUTING  ); 	
	val &= ~(WM8994_AIF1DAC1L_TO_DAC1L_MASK);
	val |= WM8994_AIF1DAC1L_TO_DAC1L;
	wm8994_write(codec,WM8994_DAC1_LEFT_MIXER_ROUTING ,val);
	
	//Enbale bias,vmid, hp left and right and Left speaker
	val = wm8994_read(codec, WM8994_POWER_MANAGEMENT_3);
	val &= ~(WM8994_MIXOUTLVOL_ENA_MASK | WM8994_MIXOUTRVOL_ENA_MASK | WM8994_MIXOUTL_ENA_MASK | WM8994_MIXOUTR_ENA_MASK | WM8994_SPKLVOL_ENA_MASK);
	val |= (WM8994_MIXOUTLVOL_ENA | WM8994_MIXOUTRVOL_ENA | WM8994_MIXOUTL_ENA | WM8994_MIXOUTR_ENA | WM8994_SPKLVOL_ENA);
	wm8994_write(codec, WM8994_POWER_MANAGEMENT_3 ,val);
	
	val = wm8994_read(codec, WM8994_POWER_MANAGEMENT_1);
	val &= ~(WM8994_BIAS_ENA_MASK | WM8994_VMID_SEL_MASK |
		WM8994_HPOUT1L_ENA_MASK |WM8994_HPOUT1R_ENA_MASK|WM8994_SPKOUTL_ENA_MASK);
	val |= (WM8994_BIAS_ENA | WM8994_VMID_SEL_NORMAL |
		WM8994_HPOUT1R_ENA	 |WM8994_HPOUT1L_ENA |WM8994_SPKOUTL_ENA);
	ret = wm8994_write(codec,WM8994_POWER_MANAGEMENT_1,val);
#endif //STEREO_SPEAKER_SUPPORT
}                                                      

void wm8994_set_voicecall_receiver(struct snd_soc_codec *codec)
{
	struct wm8994_priv *wm8994 = codec->private_data;

	int val;
	
	DEBUG_LOG("");

	audio_ctrl_mic_bias_gpio(1);

	wm8994_write(codec, WM8994_CHARGE_PUMP_1, WM8994_CP_ENA_DEFAULT);	// Turn off charge pump.
	
	// Analogue Input Configuration -Main MIC
	wm8994_write(codec, WM8994_POWER_MANAGEMENT_2, 
		WM8994_TSHUT_ENA | WM8994_TSHUT_OPDIS | WM8994_MIXINL_ENA | WM8994_IN1L_ENA);
	
	wm8994_write(codec, WM8994_INPUT_MIXER_2, WM8994_IN1LP_TO_IN1L | WM8994_IN1LN_TO_IN1L); 	// differential(3) or single ended(1)
		
	/* Digital Path Enables and Unmutes*/	
	wm8994_write(codec, WM8994_POWER_MANAGEMENT_4, WM8994_AIF2ADCL_ENA | WM8994_ADCL_ENA);	
	wm8994_write(codec, WM8994_POWER_MANAGEMENT_5 , 
		WM8994_AIF2DACL_ENA | WM8994_AIF2DACR_ENA | WM8994_AIF1DAC1L_ENA | WM8994_AIF1DAC1R_ENA | 
		WM8994_DAC1L_ENA | WM8994_DAC1R_ENA);
	wm8994_write(codec, WM8994_POWER_MANAGEMENT_6, 0x0000);
	
	wm8994_write(codec,WM8994_AIF1_DAC1_FILTERS_1, 0x0000 );  	//AIF1DAC1 Unmute, Mono Mix diable, Fast Ramp
	wm8994_write(codec, WM8994_AIF2_DAC_FILTERS_1, 0x0000 );	//AIF2DAC Unmute, Mono Mix diable, Fast Ramp

	wm8994_write(codec, WM8994_DAC2_MIXER_VOLUMES, 0x000C);	
	wm8994_write(codec, WM8994_DAC2_LEFT_VOLUME, 0x01C0 );	
	wm8994_write(codec, WM8994_DAC2_RIGHT_VOLUME, 0x01C0 );	

	// AIF1 & AIF2 Output is connected to DAC1	
	wm8994_write(codec, WM8994_DAC1_LEFT_MIXER_ROUTING, WM8994_AIF2DACL_TO_DAC1L | WM8994_AIF1DAC1L_TO_DAC1L);	
	wm8994_write(codec, WM8994_DAC1_RIGHT_MIXER_ROUTING, WM8994_AIF2DACR_TO_DAC1R | WM8994_AIF1DAC1R_TO_DAC1R);	

	// Tx -> AIF2 Path
	wm8994_write(codec, WM8994_DAC2_LEFT_MIXER_ROUTING, WM8994_ADC1_TO_DAC2L);	
	wm8994_write(codec, WM8994_DAC2_RIGHT_MIXER_ROUTING, WM8994_ADC1_TO_DAC2R);

	if(!wm8994->testmode_config_flag)
	{	
		// Unmute IN1L PGA, update volume
		val = wm8994_read(codec, WM8994_LEFT_LINE_INPUT_1_2_VOLUME);	
		val &= ~(WM8994_IN1L_MUTE_MASK | WM8994_IN1L_VOL_MASK);
		val |= (WM8994_IN1L_VU |TUNING_CALL_RCV_INPUTMIX_VOL); //volume
		wm8994_write(codec, WM8994_LEFT_LINE_INPUT_1_2_VOLUME, val);
	
		//Unmute the PGA
		val = wm8994_read(codec, WM8994_INPUT_MIXER_3);
		val&= ~(WM8994_IN1L_TO_MIXINL_MASK | WM8994_IN1L_MIXINL_VOL_MASK | WM8994_MIXOUTL_MIXINL_VOL_MASK);
		val |= (WM8994_IN1L_TO_MIXINL |WM8994_IN1L_MIXINL_VOL);//30db
		wm8994_write(codec, WM8994_INPUT_MIXER_3, val); 
	
		// Volume Control - Output
		val = wm8994_read(codec, WM8994_OUTPUT_MIXER_5);
		val &= ~(WM8994_DACL_MIXOUTL_VOL_MASK);
		val |= TUNING_RCV_OUTMIX5_VOL << WM8994_DACL_MIXOUTL_VOL_SHIFT;
		wm8994_write(codec,WM8994_OUTPUT_MIXER_5, val );
		
		val = wm8994_read(codec, WM8994_OUTPUT_MIXER_6);
		val &= ~(WM8994_DACR_MIXOUTR_VOL_MASK);
		val |= TUNING_RCV_OUTMIX6_VOL << WM8994_DACR_MIXOUTR_VOL_SHIFT;
		wm8994_write(codec,WM8994_OUTPUT_MIXER_6, val );
	
		val = wm8994_read(codec, WM8994_LEFT_OPGA_VOLUME);
		val &= ~(WM8994_MIXOUTL_MUTE_N_MASK | WM8994_MIXOUTL_VOL_MASK);
		val |= (WM8994_MIXOUT_VU | WM8994_MIXOUTL_MUTE_N | TUNING_RCV_OPGAL_VOL);
		wm8994_write(codec,WM8994_LEFT_OPGA_VOLUME, val );
	
		val = wm8994_read(codec, WM8994_RIGHT_OPGA_VOLUME);
		val &= ~(WM8994_MIXOUTR_MUTE_N_MASK | WM8994_MIXOUTR_VOL_MASK);
		val |= (WM8994_MIXOUT_VU | WM8994_MIXOUTR_MUTE_N | TUNING_RCV_OPGAR_VOL);
		wm8994_write(codec,WM8994_RIGHT_OPGA_VOLUME, val );
		
		val = wm8994_read(codec, WM8994_HPOUT2_VOLUME);
		val &= ~(WM8994_HPOUT2_MUTE_MASK | WM8994_HPOUT2_VOL_MASK);
		val |= TUNING_HPOUT2_VOL << WM8994_HPOUT2_VOL_SHIFT;
		wm8994_write(codec,WM8994_HPOUT2_VOLUME, val );
	
		//Unmute DAC1 left
		val = wm8994_read(codec,WM8994_DAC1_LEFT_VOLUME );
		val &= ~(WM8994_DAC1L_MUTE_MASK | WM8994_DAC1L_VOL_MASK);
		val |= TUNING_DAC1L_VOL; 
		wm8994_write(codec,WM8994_DAC1_LEFT_VOLUME ,val);
		
		//Unmute and volume ctrl RightDAC
		val = wm8994_read(codec, WM8994_DAC1_RIGHT_VOLUME ); 
		val &= ~(WM8994_DAC1R_MUTE_MASK | WM8994_DAC1R_VOL_MASK);
		val |= TUNING_DAC1R_VOL; //0 db volume	
		wm8994_write(codec,WM8994_DAC1_RIGHT_VOLUME,val);
	}

	// Output Mixing
	wm8994_write(codec, WM8994_OUTPUT_MIXER_1, WM8994_DAC1L_TO_MIXOUTL);
	wm8994_write(codec, WM8994_OUTPUT_MIXER_2, WM8994_DAC1R_TO_MIXOUTR );
	
#if 0
	/*Clocking*/	
	wm8994_write(codec, WM8994_CLOCKING_1, 
		WM8994_DSP_FS1CLK_ENA | WM8994_DSP_FS2CLK_ENA | WM8994_DSP_FSINTCLK_ENA);	
	wm8994_write(codec,WM8994_CLOCKING_2,
		0x1 << WM8994_TOCLK_DIV_SHIFT | 0x6 << WM8994_DBCLK_DIV_SHIFT |0x3 << WM8994_OPCLK_DIV_SHIFT);
	wm8994_write(codec,WM8994_AIF2_RATE, 0x3 << WM8994_AIF2CLK_RATE_SHIFT);	
	
	// AIF2 Interface
	wm8994_write(codec,WM8994_AIF2_CONTROL_1, 	//Left Justified, BCLK invert, LRCLK Invert
		WM8994_AIF2ADCR_SRC | WM8994_AIF2_BCLK_INV |WM8994_AIF2_LRCLK_INV | 0x1 << WM8994_AIF2_FMT_SHIFT);	
	wm8994_write(codec,WM8994_AIF2_CONTROL_2, 0x0001 );
	wm8994_write(codec,WM8994_AIF2_MASTER_SLAVE, 0/*WM8994_AIF2_MSTR*/);	//Master
	
	/*FLL2 Setting*/	
	wm8994_write(codec,WM8994_FLL2_CONTROL_2, 0x2F00 );	
	wm8994_write(codec,WM8994_FLL2_CONTROL_3, 0x3126 );	
	wm8994_write(codec,WM8994_FLL2_CONTROL_4, 0x0100 );
	wm8994_write(codec,WM8994_FLL2_CONTROL_5, 0x0C88 );
	wm8994_write(codec,WM8994_AIF2_CLOCKING_1, 0x0019 ); //FLL2 Enable,AIFCLK2 Enable	
	wm8994_write(codec,WM8994_FLL2_CONTROL_1, WM8994_FLL2_FRACN_ENA | WM8994_FLL2_ENA);	
#else
	/*FLL2  Setting*/	
	wm8994_write(codec,0x244, 0x0C83 ); //BCLK2  
	wm8994_write(codec,0x241, 0x2F00 ); //48  
	wm8994_write(codec,0x243, 0x0600 );   
	wm8994_write(codec,0x240, 0x0001 ); //FLL2 Enable, Integer Mode
	wm8994_write(codec,0x208, 0x0007 );  

	wm8994_write(codec,0x204, 0x0019 ); //FLL2 En, AIF2 Clock Enable.   
	wm8994_write(codec,0x211, 0x0003 );   
	wm8994_write(codec,0x310, 0x4118 ); //DSP Mode 
	wm8994_write(codec,0x311, 0x4014 ); //u_Law Companding 
#endif 
	
	/*GPIO Configuration*/
	wm8994_write(codec,WM8994_GPIO_3, 0x8100 );	
	wm8994_write(codec,WM8994_GPIO_4, 0x8100 );	
	wm8994_write(codec,WM8994_GPIO_5, 0x8100 );	
	wm8994_write(codec,WM8994_GPIO_6, 0xA101 );	
	wm8994_write(codec,WM8994_GPIO_7, 0x0100 );

	// Analogue Output Configuration
	wm8994_write(codec, WM8994_POWER_MANAGEMENT_3, 
		WM8994_MIXOUTLVOL_ENA | WM8994_MIXOUTRVOL_ENA |WM8994_MIXOUTL_ENA | WM8994_MIXOUTR_ENA);
	wm8994_write(codec, WM8994_HPOUT2_MIXER, WM8994_MIXOUTLVOL_TO_HPOUT2 |WM8994_MIXOUTRVOL_TO_HPOUT2); 
	wm8994_write(codec, WM8994_POWER_MANAGEMENT_1, 
		WM8994_HPOUT2_ENA | WM8994_VMID_SEL_NORMAL | WM8994_BIAS_ENA);

	//	wm8994_write(codec, WM8994_INPUT_MIXER_4, 0x0000 ); 
}


void wm8994_set_voicecall_headset(struct snd_soc_codec *codec)
{
	struct wm8994_priv *wm8994 = codec->private_data;

	int val;

	DEBUG_LOG("");

	audio_ctrl_mic_bias_gpio(1);
	
	/*DCS*/
	wm8994_write(codec,WM8994_DC_SERVO_4, 0x9C9C );   
	wm8994_write(codec,WM8994_DC_SERVO_2, 0x054E );   
	wm8994_write(codec,WM8994_DC_SERVO_1, 0x300f );   
	wm8994_write(codec,WM8994_DC_SERVO_ANA_1, 0x0080 );   
	wm8994_write(codec,WM8994_DC_SERVO_ANA_2, 0x0080 );   
	
	/* Power - UP seqeunce- COLD Start Up*/
	wm8994_write(codec,WM8994_ANTIPOP_2, 0x006C );   
	wm8994_write(codec,WM8994_POWER_MANAGEMENT_1, 0x0003 );   
	wm8994_write(codec,WM8994_DC_SERVO_ANA_1, 0x00FF );  
	wm8994_write(codec,WM8994_DC_SERVO_ANA_2, 0x00FF );  
	wm8994_write(codec,WM8994_CHARGE_PUMP_1, 0x9F25 );  
	wm8994_write(codec,WM8994_POWER_MANAGEMENT_1, 0x0313 );  
	wm8994_write(codec,WM8994_ANALOGUE_HP_1, 0x0022 );  
	wm8994_write(codec,WM8994_DC_SERVO_1, 0x333F );  
	wm8994_write(codec,WM8994_ANALOGUE_HP_1, 0x00EE );  
	
	/*Analogue Input Configuration*/
	wm8994_write(codec,WM8994_POWER_MANAGEMENT_2, 0x6110 );
	wm8994_write(codec,WM8994_INPUT_MIXER_2, 0x0003 );	 
	
	/*Analogue Output Configuration*/	
	wm8994_write(codec,WM8994_OUTPUT_MIXER_1, 0x0100 );   
	wm8994_write(codec,WM8994_OUTPUT_MIXER_2, 0x0100 );   
	wm8994_write(codec,WM8994_CHARGE_PUMP_1, 0x9F25 );   
	wm8994_write(codec,WM8994_ANALOGUE_HP_1, 0x00EE );   
	
	/*Digital Path Enables and Unmutes*/	
	wm8994_write(codec,WM8994_POWER_MANAGEMENT_4, 0x3003 );   
	wm8994_write(codec,WM8994_POWER_MANAGEMENT_5, 0x3303 );   
	wm8994_write(codec,WM8994_POWER_MANAGEMENT_6, 0x0000 );
	wm8994_write(codec,WM8994_AIF1_DAC1_FILTERS_1, 0x0000 );  
	wm8994_write(codec,WM8994_AIF2_DAC_FILTERS_1, 0x0000 );  
	wm8994_write(codec,WM8994_DAC1_LEFT_MIXER_ROUTING, 0x0005 );  
	wm8994_write(codec,WM8994_DAC1_RIGHT_MIXER_ROUTING, 0x0005 );
	wm8994_write(codec,WM8994_DAC2_LEFT_VOLUME, 0x01C0 );
	wm8994_write(codec,WM8994_DAC2_RIGHT_VOLUME, 0x01C0 );

	if(wm8994->hw_version == 3)	// H/W Rev D
	{
		wm8994_write(codec, WM8994_DAC2_LEFT_MIXER_ROUTING, WM8994_ADC2_TO_DAC2L | WM8994_ADC1_TO_DAC2L);	
		wm8994_write(codec,WM8994_DAC2_MIXER_VOLUMES, 0x018C );  
		wm8994_write(codec,WM8994_SIDETONE, 0x01C0);
	}
	else		// H/W Rev B
	{
		wm8994_write(codec,WM8994_DAC2_MIXER_VOLUMES, 0x000C );  
		wm8994_write(codec, WM8994_DAC2_LEFT_MIXER_ROUTING, WM8994_ADC1_TO_DAC2L);
		wm8994_write(codec,WM8994_SIDETONE, 0x01C1 );
	}

	// Tx -> AIF2 Path
	wm8994_write(codec, WM8994_DAC2_RIGHT_MIXER_ROUTING, WM8994_ADC1_TO_DAC2R);

#if 0
	/*Clocking*/	
	wm8994_write(codec,WM8994_CLOCKING_1, 0x000E );  
	
	/*AIF2 Interface*/	
	wm8994_write(codec,WM8994_AIF2_CONTROL_2, 0x0000 );
	wm8994_write(codec,WM8994_AIF2_MASTER_SLAVE, 0x4000 );	//Master
	wm8994_write(codec,WM8994_AIF2_CONTROL_1, 0x4188 );	//Left Justified, BCLK invert, LRCLK Invert
	
	/*FLL2	Setting*/	
	wm8994_write(codec,WM8994_FLL2_CONTROL_2, 0x2F00 );  
	wm8994_write(codec,WM8994_FLL2_CONTROL_3, 0x3126 ); 
	wm8994_write(codec,WM8994_FLL2_CONTROL_4, 0x0100 );  
	wm8994_write(codec,WM8994_FLL2_CONTROL_5, 0x0C88 );  
	wm8994_write(codec,WM8994_AIF2_CLOCKING_1, 0x0019 );  
	wm8994_write(codec,WM8994_FLL2_CONTROL_1, 0x0005 );  
#else
	/*FLL2  Setting*/	
	wm8994_write(codec,0x244, 0x0C83 ); //BCLK2  
	wm8994_write(codec,0x241, 0x2F00 ); //48  
	wm8994_write(codec,0x243, 0x0600 );   
	wm8994_write(codec,0x240, 0x0001 ); //FLL2 Enable, Integer Mode

	/*Clocking*/		
	wm8994_write(codec,0x208, 0x0007 );  

	/*AIF2 Interface*/	
	wm8994_write(codec,0x204, 0x0019 ); //FLL2 En, AIF2 Clock Enable.   
	wm8994_write(codec,0x211, 0x0003 );   
	wm8994_write(codec,0x310, 0x4118 ); //DSP Mode 
	wm8994_write(codec,0x311, 0x4014 ); //u_Law Companding 
#endif 
	
	/*GPIO Configuration*/	
	wm8994_write(codec,WM8994_GPIO_1, 0xA101 );  
	wm8994_write(codec,WM8994_GPIO_2, 0x8100 );  
	wm8994_write(codec,WM8994_GPIO_3, 0x8100 );  
	wm8994_write(codec,WM8994_GPIO_4, 0x8100 );  
	wm8994_write(codec,WM8994_GPIO_5, 0x8100 );  
	wm8994_write(codec,WM8994_GPIO_6, 0xA101 );  
	wm8994_write(codec,WM8994_GPIO_7, 0x0100 );  

	/* Unmute*/
	if(!wm8994->testmode_config_flag)
	{
		val = wm8994_read(codec,WM8994_RIGHT_LINE_INPUT_1_2_VOLUME);	
		val &= ~(WM8994_IN1R_MUTE_MASK | WM8994_IN1R_VOL_MASK);	// Unmute IN1R
		val |= (WM8994_IN1R_VU | TUNING_CALL_EAR_INPUTMIX_VOL);
		wm8994_write(codec,WM8994_RIGHT_LINE_INPUT_1_2_VOLUME,val);
	
		// unmute right pga, set volume 
		val = wm8994_read(codec,WM8994_INPUT_MIXER_4 );
		val&= ~(WM8994_IN1R_TO_MIXINR_MASK | WM8994_IN1R_MIXINR_VOL_MASK | WM8994_MIXOUTR_MIXINR_VOL_MASK);
		val |= (WM8994_IN1R_TO_MIXINR);//0db
		wm8994_write(codec,WM8994_INPUT_MIXER_4 ,val);

		// Volume Control - Output
	        val = wm8994_read(codec, WM8994_OUTPUT_MIXER_5);
		val &= ~(WM8994_DACL_MIXOUTL_VOL_MASK);
	        val |= TUNING_EAR_OUTMIX5_VOL << WM8994_DACL_MIXOUTL_VOL_SHIFT;
	        wm8994_write(codec,WM8994_OUTPUT_MIXER_5, val );
	
		val = wm8994_read(codec, WM8994_OUTPUT_MIXER_6);
		val &= ~(WM8994_DACR_MIXOUTR_VOL_MASK);
		val |= TUNING_EAR_OUTMIX6_VOL << WM8994_DACR_MIXOUTR_VOL_SHIFT;
		wm8994_write(codec,WM8994_OUTPUT_MIXER_6, val );

		val = wm8994_read(codec,WM8994_LEFT_OUTPUT_VOLUME);
		val &= ~(WM8994_HPOUT1L_MUTE_N_MASK | WM8994_HPOUT1L_VOL_MASK);
		val |= (WM8994_HPOUT1_VU | WM8994_HPOUT1L_MUTE_N | TUNING_CALL_OUTPUTL_VOL);
		wm8994_write(codec, WM8994_LEFT_OUTPUT_VOLUME, val);

		val = wm8994_read(codec,WM8994_RIGHT_OUTPUT_VOLUME);
		val &= ~(WM8994_HPOUT1R_MUTE_N_MASK | WM8994_HPOUT1R_VOL_MASK);
		val |= (WM8994_HPOUT1_VU | WM8994_HPOUT1R_MUTE_N | TUNING_CALL_OUTPUTR_VOL);
		wm8994_write(codec, WM8994_RIGHT_OUTPUT_VOLUME, val);

		val = wm8994_read(codec, WM8994_LEFT_OPGA_VOLUME);
		val &= ~(WM8994_MIXOUTL_MUTE_N_MASK | WM8994_MIXOUTL_VOL_MASK);
		val |= (WM8994_MIXOUT_VU | WM8994_MIXOUTL_MUTE_N | TUNING_CALL_OPGAL_VOL);
		wm8994_write(codec,WM8994_LEFT_OPGA_VOLUME, val );
	
		val = wm8994_read(codec, WM8994_RIGHT_OPGA_VOLUME);
		val &= ~(WM8994_MIXOUTR_MUTE_N_MASK | WM8994_MIXOUTR_VOL_MASK);
		val |= (WM8994_MIXOUT_VU | WM8994_MIXOUTR_MUTE_N | TUNING_CALL_OPGAR_VOL);
		wm8994_write(codec,WM8994_RIGHT_OPGA_VOLUME, val );
	
		//Unmute DAC1 left
		val = wm8994_read(codec,WM8994_DAC1_LEFT_VOLUME );
		val &= ~(WM8994_DAC1L_MUTE_MASK | WM8994_DAC1L_VOL_MASK);
		val |= TUNING_DAC1L_VOL; 
		wm8994_write(codec,WM8994_DAC1_LEFT_VOLUME ,val);
	
		//Unmute and volume ctrl RightDAC
		val = wm8994_read(codec, WM8994_DAC1_RIGHT_VOLUME ); 
		val &= ~(WM8994_DAC1R_MUTE_MASK | WM8994_DAC1R_VOL_MASK);
		val |= TUNING_DAC1R_VOL; //0 db volume	
		wm8994_write(codec,WM8994_DAC1_RIGHT_VOLUME,val);
	}
	
	/*DCS*/
	wm8994_write(codec,WM8994_DC_SERVO_4, 0x9C9C );  
	wm8994_write(codec,WM8994_DC_SERVO_2, 0x054E ); 
	wm8994_write(codec,WM8994_DC_SERVO_1, 0x300f );  
	wm8994_write(codec,WM8994_DC_SERVO_ANA_1, 0x0080 ); 
	wm8994_write(codec,WM8994_DC_SERVO_ANA_2, 0x0080 ); 
}

void wm8994_set_voicecall_speaker(struct snd_soc_codec *codec)
{
	struct wm8994_priv *wm8994 = codec->private_data;

	int val;

	DEBUG_LOG("");

	audio_ctrl_mic_bias_gpio(1);

	wm8994_write(codec,0x39, 0x006C );/*wm8994_write(codec,WM8994_ANTIPOP_2, WM8994_VMID_RAMP_ENA|WM8994_VMID_BUF_ENA|WM8994_STARTUP_BIAS_ENA );*/
	/*Analogue Configuration*/	
	wm8994_write(codec,0x01, 0x0003 );/*wm8994_write(codec,WM8994_POWER_MANAGEMENT_1, WM8994_VMID_SEL_NORMAL|WM8994_BIAS_ENA );*/   
	/*Analogue Input Configuration*/
	wm8994_write(codec,0x02, 0x6240 );/*wm8994_write(codec,WM8994_POWER_MANAGEMENT_2, WM8994_TSHUT_ENA|WM8994_TSHUT_OPDIS|WM8994_MIXINL_ENA|WM8994_IN1L_ENA );*/    

	/*Analogue Output Configuration*/	
	wm8994_write(codec,0x03, 0x0300 );

	wm8994_write(codec, WM8994_INPUT_MIXER_2, WM8994_IN1LP_TO_IN1L | WM8994_IN1LN_TO_IN1L); 	// differential(3) or single ended(1)
	
	wm8994_write(codec, WM8994_POWER_MANAGEMENT_4, WM8994_AIF2ADCL_ENA | WM8994_ADCL_ENA);	
	wm8994_write(codec, WM8994_POWER_MANAGEMENT_5 , 
		WM8994_AIF2DACL_ENA | WM8994_AIF2DACR_ENA | WM8994_AIF1DAC1L_ENA | WM8994_AIF1DAC1R_ENA | 
		WM8994_DAC1L_ENA | WM8994_DAC1R_ENA);
	wm8994_write(codec, WM8994_POWER_MANAGEMENT_6, 0x0000);

	if(!wm8994->testmode_config_flag)
	{
		// Volume Control - Input
		val = wm8994_read(codec, WM8994_INPUT_MIXER_3);
		val&= ~(WM8994_IN1L_TO_MIXINL_MASK | WM8994_IN1L_MIXINL_VOL_MASK | WM8994_MIXOUTL_MIXINL_VOL_MASK);
		val |= (WM8994_IN1L_TO_MIXINL);//0db
		wm8994_write(codec, WM8994_INPUT_MIXER_3, val); 
	
		val = wm8994_read(codec, WM8994_LEFT_LINE_INPUT_1_2_VOLUME );	
		val &= ~(WM8994_IN1L_MUTE_MASK | WM8994_IN1L_VOL_MASK);	// Unmute IN1L
		val |= (WM8994_IN1L_VU | TUNING_CALL_SPK_INPUTMIX_VOL);
		wm8994_write(codec, WM8994_LEFT_LINE_INPUT_1_2_VOLUME , val);
	
		// Volume Control - Output
		// Unmute the SPKMIXVOLUME
		val = wm8994_read(codec, WM8994_SPKMIXL_ATTENUATION);
		val &= ~(WM8994_SPKMIXL_VOL_MASK);
		val |= TUNING_SPKMIXL_ATTEN;	
		wm8994_write(codec, WM8994_SPKMIXL_ATTENUATION, val);
		
		val = wm8994_read(codec,WM8994_SPKMIXR_ATTENUATION);
		val &= ~(WM8994_SPKMIXR_VOL_MASK);
		val |= TUNING_SPKMIXR_ATTEN;	
		wm8994_write(codec, WM8994_SPKMIXR_ATTENUATION, val);

		val = wm8994_read(codec,WM8994_SPEAKER_VOLUME_LEFT);
		val &= ~(WM8994_SPKOUTL_MUTE_N_MASK | WM8994_SPKOUTL_VOL_MASK);
		val |= (WM8994_SPKOUTL_MUTE_N | TUNING_CALL_SPKL_VOL);
		wm8994_write(codec, WM8994_SPEAKER_VOLUME_LEFT, val);

		val = wm8994_read(codec,WM8994_SPEAKER_VOLUME_RIGHT);
		val &= ~(WM8994_SPKOUTR_MUTE_N_MASK | WM8994_SPKOUTR_VOL_MASK);
		wm8994_write(codec, WM8994_SPEAKER_VOLUME_RIGHT, val);

		val = wm8994_read(codec,WM8994_CLASSD);
		val &= ~(WM8994_SPKOUTL_BOOST_MASK);
		val |= TUNING_CALL_CLASSD_VOL << WM8994_SPKOUTL_BOOST_SHIFT;
		wm8994_write(codec, WM8994_CLASSD, val);
	
		//Unmute DAC1 left
		val = wm8994_read(codec,WM8994_DAC1_LEFT_VOLUME );
		val &= ~(WM8994_DAC1L_MUTE_MASK | WM8994_DAC1L_VOL_MASK);
		val |= TUNING_DAC1L_VOL; 
		wm8994_write(codec,WM8994_DAC1_LEFT_VOLUME ,val);
	
		//Unmute and volume ctrl RightDAC
		val = wm8994_read(codec, WM8994_DAC1_RIGHT_VOLUME ); 
		val &= ~(WM8994_DAC1R_MUTE_MASK | WM8994_DAC1R_VOL_MASK);
		val |= TUNING_DAC1R_VOL; //0 db volume	
		wm8994_write(codec,WM8994_DAC1_RIGHT_VOLUME,val);
	}

	val = wm8994_read(codec,WM8994_SPKOUT_MIXERS );
	val &= ~(WM8994_SPKMIXL_TO_SPKOUTL_MASK | 
		WM8994_SPKMIXR_TO_SPKOUTL_MASK | WM8994_SPKMIXR_TO_SPKOUTR_MASK);
	val |= WM8994_SPKMIXL_TO_SPKOUTL;
	wm8994_write(codec,WM8994_SPKOUT_MIXERS,val );
	
	wm8994_write(codec,0x36, 0x0003 );    
	/* Digital Path Enables and Unmutes*/	
	wm8994_write(codec,0x520, 0x0000 );   
	wm8994_write(codec,0x601, 0x0005 );   
	wm8994_write(codec,0x602, 0x0005 );   
	wm8994_write(codec,0x603, 0x000C );   
	wm8994_write(codec,0x612, 0x01C0 );   
	wm8994_write(codec,0x613, 0x01C0 );   
	wm8994_write(codec,0x620, 0x0000 );   
	wm8994_write(codec,0x621, 0x01C0 );   

	// Tx -> AIF2 Path
	wm8994_write(codec, WM8994_DAC2_LEFT_MIXER_ROUTING, WM8994_ADC1_TO_DAC2L);	
	wm8994_write(codec, WM8994_DAC2_RIGHT_MIXER_ROUTING, WM8994_ADC1_TO_DAC2R);

#if 0
	/*Clocking*/		
	wm8994_write(codec,0x208, 0x000E );   
	wm8994_write(codec,0x210, 0x0073 );   

	/*AIF2 Interface*/	
	wm8994_write(codec,0x311, 0x0000 );
	wm8994_write(codec,0x312, 0x4000 );   //Master
	wm8994_write(codec,0x310, 0x4188 );   //Left Justified, BCLK invert, LRCLK Invert
	wm8994_write(codec,0x204, 0x0019 );   

	/*FLL2  Setting*/	
	wm8994_write(codec,0x240, 0x0005 );  
	wm8994_write(codec,0x241, 0x2F00 );   
	wm8994_write(codec,0x242, 0x3126 );   
	wm8994_write(codec,0x244, 0x0C88 );   
	wm8994_write(codec,0x243, 0x0100 );   
#else
	/*FLL2  Setting*/	
	wm8994_write(codec,0x244, 0x0C83 ); //BCLK2  
	wm8994_write(codec,0x241, 0x2F00 ); //48  
	wm8994_write(codec,0x243, 0x0600 );   
	wm8994_write(codec,0x240, 0x0001 ); //FLL2 Enable, Integer Mode

	/*Clocking*/		
	wm8994_write(codec,0x208, 0x0007 );  

	/*AIF2 Interface*/	
	wm8994_write(codec,0x204, 0x0019 ); //FLL2 En, AIF2 Clock Enable.   
	wm8994_write(codec,0x211, 0x0003 );   
	wm8994_write(codec,0x310, 0x4118 ); //DSP Mode 
	wm8994_write(codec,0x311, 0x4014 ); //u_Law Companding 
#endif 


	/*GPIO Configuration*/	
	wm8994_write(codec,0x701, 0x8100 );   
	wm8994_write(codec,0x702, 0x8100 );   
	wm8994_write(codec,0x703, 0x8100 );   
	wm8994_write(codec,0x704, 0x8100 );   
	wm8994_write(codec,0x705, 0xA101 );   
	wm8994_write(codec,0x706, 0x0100 );   

	wm8994_write(codec, WM8994_POWER_MANAGEMENT_1, 
		WM8994_SPKOUTL_ENA | WM8994_VMID_SEL_NORMAL | WM8994_BIAS_ENA);
}

void wm8994_set_voicecall_bluetooth(struct snd_soc_codec *codec)
{
	struct wm8994_priv *wm8994 = codec->private_data;

	DEBUG_LOG("");

	wm8994_write(codec, WM8994_POWER_MANAGEMENT_1, 0x0007);		//fast vmid
	msleep(50);
	wm8994_write(codec, WM8994_POWER_MANAGEMENT_1, 0x0003);		//normal vmid

	/*Digital Path Enables and Unmutes*/		 
	wm8994_write(codec, WM8994_POWER_MANAGEMENT_2, 0x00);
	wm8994_write(codec, WM8994_POWER_MANAGEMENT_3, 0x00);
	wm8994_write(codec, WM8994_POWER_MANAGEMENT_4, 0x3000);
	wm8994_write(codec, WM8994_POWER_MANAGEMENT_5, 
		WM8994_AIF2DACL_ENA | WM8994_AIF2DACR_ENA | WM8994_AIF1DAC1L_ENA | WM8994_AIF1DAC1R_ENA| WM8994_DAC2L_ENA | WM8994_DAC2R_ENA);
	//for BT DTMF Play
	//Rx Path: AIF2ADCDAT2 select
	//CP(CALL) Path:GPIO5/DACDAT2 select
	//AP(DTMF) Path: DACDAT1 select
	//Tx Path: GPIO8/DACDAT3 select
	wm8994_write(codec, WM8994_POWER_MANAGEMENT_6, 0x0003<<WM8994_AIF2_ADCDAT_SRC_SHIFT);
	
	wm8994_write(codec, WM8994_AIF2_DAC_FILTERS_1, 0x00 ); 

	wm8994_write(codec, WM8994_DAC2_LEFT_MIXER_ROUTING, WM8994_AIF2DACL_TO_DAC2L | WM8994_AIF1DAC1L_TO_DAC2L);
	wm8994_write(codec, WM8994_DAC2_RIGHT_MIXER_ROUTING, WM8994_AIF2DACR_TO_DAC2R | WM8994_AIF1DAC1R_TO_DAC2R);

	wm8994_write(codec, WM8994_DAC2_LEFT_VOLUME, 0x1C0 );
	wm8994_write(codec, WM8994_DAC2_RIGHT_VOLUME, 0x1C0 ); 

	/*Clocking*/
	wm8994_write(codec, WM8994_CLOCKING_1, WM8994_DSP_FS1CLK_ENA | WM8994_DSP_FS2CLK_ENA | WM8994_DSP_FSINTCLK_ENA);
	wm8994_write(codec, WM8994_AIF2_RATE, 0x0003<<WM8994_AIF2CLK_RATE_SHIFT);	//8kHz

	/*AIF1Interface*/	
	wm8994_write(codec, WM8994_AIF1_CONTROL_2, 0x0000);	//Left & Right DAC receives left interface data
	wm8994_write(codec, WM8994_AIF1_MASTER_SLAVE, WM8994_AIF1_MSTR | WM8994_AIF1_CLK_FRC | WM8994_AIF1_LRCLK_FRC);	// Master mode

	/*AIF2 Interface*/	
	wm8994_write(codec, WM8994_AIF2_CONTROL_2, 0x0000);	//Left & Right DAC receives left interface data
	wm8994_write(codec, WM8994_AIF2_MASTER_SLAVE, WM8994_AIF2_MSTR | WM8994_AIF2_CLK_FRC | WM8994_AIF2_LRCLK_FRC);	// Master mode
	wm8994_write(codec, WM8994_AIF2_CONTROL_1, 	//Left Justified, BCLK invert, LRCLK Invert
		WM8994_AIF2ADCR_SRC | WM8994_AIF2_BCLK_INV |WM8994_AIF2_LRCLK_INV | 0x1 << WM8994_AIF2_FMT_SHIFT);	

	wm8994_write(codec, WM8994_OVERSAMPLING, 0X0000);

	/*FLL2	Setting*/	
	wm8994_write(codec, WM8994_FLL2_CONTROL_2, 0x2F00);	// REG 221 FLL1 Cntr2, FLL1 Setting
	wm8994_write(codec, WM8994_FLL2_CONTROL_3, 0x3126);	// REG 222 FLL1 Cntr3, K Value
	wm8994_write(codec, WM8994_FLL2_CONTROL_4, 0x0100);	// REG 223 FLL1 Cntr4, N Value
	wm8994_write(codec, WM8994_FLL2_CONTROL_5, 0x0C88);	// REG 224 FLL1 Cntr5
	wm8994_write(codec, WM8994_AIF2_CLOCKING_1, 0x0019);	// REG 200 Enable AIF1 Clock, AIF1 Clock Source = FLL1
	wm8994_write(codec, WM8994_FLL2_CONTROL_1, WM8994_FLL2_FRACN_ENA | WM8994_FLL2_ENA);

	/*GPIO Configuration*/		
	wm8994_write(codec, WM8994_GPIO_1, 0xA101);  
	wm8994_write(codec, WM8994_GPIO_3, WM8994_GP3_DIR | WM8994_GP3_DB);	
	wm8994_write(codec, WM8994_GPIO_4, WM8994_GP4_DIR | WM8994_GP4_DB);
	wm8994_write(codec, WM8994_GPIO_5, WM8994_GP5_DIR | WM8994_GP5_DB);	
	wm8994_write(codec, WM8994_GPIO_6, WM8994_GP6_DIR | WM8994_GP6_PD | WM8994_GP6_DB |0x1 << WM8994_GP6_FN_SHIFT);
	wm8994_write(codec, WM8994_GPIO_7, WM8994_GP7_DB);
	wm8994_write(codec, WM8994_GPIO_8, WM8994_GP8_DIR | WM8994_GP8_DB);
	wm8994_write(codec, WM8994_GPIO_9, WM8994_GP9_DB);
	wm8994_write(codec, WM8994_GPIO_10, WM8994_GP10_DB);
	wm8994_write(codec, WM8994_GPIO_11, WM8994_GP11_DB); 
}
	
void wm8994_set_fmradio_common(struct snd_soc_codec *codec)
{	
	struct wm8994_priv *wm8994 = codec->private_data;
	
	u16 val;

	// Enable IN2 and MIXIN - Temporary inserted for blocking MIC and FM radio mixing - DW Shim 2010.03.04
//	val = wm8994_read(codec, WM8994_POWER_MANAGEMENT_2);
//	val &= ~(WM8994_TSHUT_ENA_MASK | WM8994_TSHUT_OPDIS_MASK | WM8994_OPCLK_ENA_MASK | 
//			WM8994_MIXINL_ENA_MASK | WM8994_MIXINR_ENA_MASK | WM8994_IN2L_ENA_MASK | WM8994_IN2R_ENA_MASK);
//	val |= (WM8994_TSHUT_ENA | WM8994_TSHUT_OPDIS | WM8994_OPCLK_ENA | WM8994_MIXINL_ENA | 
//			WM8994_MIXINR_ENA | WM8994_IN2L_ENA | WM8994_IN2R_ENA);
	val = (WM8994_TSHUT_ENA | WM8994_TSHUT_OPDIS | WM8994_OPCLK_ENA | WM8994_MIXINL_ENA | 
			WM8994_MIXINR_ENA | WM8994_IN2L_ENA | WM8994_IN2R_ENA);
	wm8994_write(codec, WM8994_POWER_MANAGEMENT_2, val);

	// Input mixer setting - Temporary inserted for blocking MIC and FM radio mixing - DW Shim 2010.02.25
//	val = wm8994_read(codec, WM8994_INPUT_MIXER_2);
//	val &= ~(WM8994_IN2LN_TO_IN2L_MASK | WM8994_IN2RN_TO_IN2R_MASK);
//	val |= (WM8994_IN2LN_TO_IN2L | WM8994_IN2RN_TO_IN2R);
	val = (WM8994_IN2LN_TO_IN2L | WM8994_IN2RN_TO_IN2R);
	wm8994_write(codec, WM8994_INPUT_MIXER_2, val); 	

	if(!wm8994->testmode_config_flag)
	{
		// IN2L to MIXINL
		val = wm8994_read(codec, WM8994_INPUT_MIXER_3);
		val &= ~(WM8994_IN2L_TO_MIXINL_MASK);
		val |= WM8994_IN2L_TO_MIXINL;
		wm8994_write(codec, WM8994_INPUT_MIXER_3, val);

		//IN2R to MIXINR
		val = wm8994_read(codec, WM8994_INPUT_MIXER_4);
		val &= ~(WM8994_IN2R_TO_MIXINR_MASK);
		val |= WM8994_IN2R_TO_MIXINR;
		wm8994_write(codec, WM8994_INPUT_MIXER_4, val);	
	}
				
	// IN2L PGA volume
	val = wm8994_read(codec, WM8994_LEFT_LINE_INPUT_3_4_VOLUME);
	val &= ~(WM8994_IN2L_VU_MASK | WM8994_IN2L_MUTE_MASK | WM8994_IN2L_VOL_MASK);
	val |= (WM8994_IN2L_VU | WM8994_IN2L_VOL_16_5dB);
	wm8994_write(codec, WM8994_LEFT_LINE_INPUT_3_4_VOLUME, val); 
	
	val = wm8994_read(codec, WM8994_RIGHT_LINE_INPUT_3_4_VOLUME);
	val &= ~(WM8994_IN2R_VU_MASK | WM8994_IN2R_MUTE_MASK | WM8994_IN2R_VOL_MASK);
	val |= (WM8994_IN2R_VU | WM8994_IN2R_VOL_16_5dB);	
	wm8994_write(codec, WM8994_RIGHT_LINE_INPUT_3_4_VOLUME, val); 		
}

void wm8994_set_fmradio_headset(struct snd_soc_codec *codec)
{	
	struct wm8994_priv *wm8994 = codec->private_data;

	u16 val;
	
	DEBUG_LOG("Routing ear path : FM Radio -> EAR Out");

	wm8994->fmradio_path = FMR_HP;

	//Disable end point for preventing pop up noise.
	val = wm8994_read(codec, WM8994_POWER_MANAGEMENT_1);
	val &= ~(WM8994_HPOUT1L_ENA_MASK |WM8994_HPOUT1R_ENA_MASK);
	wm8994_write(codec, WM8994_POWER_MANAGEMENT_1, val);

	wm8994_set_fmradio_common(codec);
	
	if(!wm8994->testmode_config_flag)
	{
		// Output setting
		val = wm8994_read(codec, WM8994_LEFT_OUTPUT_VOLUME);
		val &= ~(WM8994_HPOUT1L_MUTE_N_MASK | WM8994_HPOUT1L_VOL_MASK);
		val |= (WM8994_HPOUT1_VU | WM8994_HPOUT1L_MUTE_N | TUNING_FMRADIO_OUTPUTL_VOL);
		wm8994_write(codec, WM8994_LEFT_OUTPUT_VOLUME, val);

		val = wm8994_read(codec, WM8994_RIGHT_OUTPUT_VOLUME);
		val &= ~(WM8994_HPOUT1R_MUTE_N_MASK | WM8994_HPOUT1R_VOL_MASK);
		val |= (WM8994_HPOUT1_VU | WM8994_HPOUT1R_MUTE_N | TUNING_FMRADIO_OUTPUTR_VOL);
		wm8994_write(codec, WM8994_RIGHT_OUTPUT_VOLUME, val);
		
		val = wm8994_read(codec, WM8994_LEFT_OPGA_VOLUME);
		val &= ~(WM8994_MIXOUTL_MUTE_N_MASK | WM8994_MIXOUTL_VOL_MASK);
		val |= (WM8994_MIXOUT_VU | WM8994_MIXOUTL_MUTE_N | TUNING_FMRADIO_OPGAL_VOL);
		wm8994_write(codec,WM8994_LEFT_OPGA_VOLUME, val );
	
		val = wm8994_read(codec, WM8994_RIGHT_OPGA_VOLUME);
		val &= ~(WM8994_MIXOUTR_MUTE_N_MASK | WM8994_MIXOUTR_VOL_MASK);
		val |= (WM8994_MIXOUT_VU | WM8994_MIXOUTR_MUTE_N | TUNING_FMRADIO_OPGAR_VOL);
		wm8994_write(codec,WM8994_RIGHT_OPGA_VOLUME, val );
	}
	
	//Enable Charge Pump	
	val = wm8994_read(codec, WM8994_CHARGE_PUMP_1);
	val &= ~WM8994_CP_ENA_MASK ;
	val |= WM8994_CP_ENA | WM8994_CP_ENA_DEFAULT ; // this is from wolfson  	
	wm8994_write(codec, WM8994_CHARGE_PUMP_1, val);

	// Intermediate HP settings
	val = wm8994_read(codec, WM8994_ANALOGUE_HP_1); 	
	val &= ~(WM8994_HPOUT1R_DLY_MASK |WM8994_HPOUT1R_OUTP_MASK |WM8994_HPOUT1R_RMV_SHORT_MASK |
		WM8994_HPOUT1L_DLY_MASK |WM8994_HPOUT1L_OUTP_MASK | WM8994_HPOUT1L_RMV_SHORT_MASK);
	val |= (WM8994_HPOUT1L_RMV_SHORT | WM8994_HPOUT1L_OUTP|WM8994_HPOUT1L_DLY |WM8994_HPOUT1R_RMV_SHORT | 
		WM8994_HPOUT1R_OUTP | WM8994_HPOUT1R_DLY);
	wm8994_write(codec, WM8994_ANALOGUE_HP_1, val);

	// Enable MIXOUT
	val = wm8994_read(codec, WM8994_POWER_MANAGEMENT_3);
	val &= ~(WM8994_MIXOUTLVOL_ENA_MASK | WM8994_MIXOUTRVOL_ENA_MASK | WM8994_MIXOUTL_ENA_MASK | WM8994_MIXOUTR_ENA_MASK);
	val |= (WM8994_MIXOUTLVOL_ENA | WM8994_MIXOUTRVOL_ENA | WM8994_MIXOUTL_ENA | WM8994_MIXOUTR_ENA);
	wm8994_write(codec, WM8994_POWER_MANAGEMENT_3, val);

	// Output mixer setting
	val = wm8994_read(codec, WM8994_OUTPUT_MIXER_1);
	val &= ~(WM8994_DAC1L_TO_HPOUT1L_MASK | WM8994_MIXINL_TO_MIXOUTL_MASK);
	val |= (WM8994_MIXINL_TO_MIXOUTL);
	wm8994_write(codec, WM8994_OUTPUT_MIXER_1, val); 

	val = wm8994_read(codec, WM8994_OUTPUT_MIXER_2);
	val &= ~(WM8994_DAC1R_TO_HPOUT1R_MASK | WM8994_MIXINR_TO_MIXOUTR_MASK);
	val |= (WM8994_MIXINR_TO_MIXOUTR);
	wm8994_write(codec, WM8994_OUTPUT_MIXER_2, val); 

	//Enable vmid,bias, hp left and right
	val = wm8994_read(codec,WM8994_POWER_MANAGEMENT_1 );
	val &= ~(WM8994_BIAS_ENA_MASK | WM8994_VMID_SEL_MASK |WM8994_HPOUT1L_ENA_MASK |WM8994_HPOUT1R_ENA_MASK);
	val |= (WM8994_BIAS_ENA | WM8994_VMID_SEL_NORMAL |WM8994_HPOUT1R_ENA	 |WM8994_HPOUT1L_ENA);  
	wm8994_write(codec,WM8994_POWER_MANAGEMENT_1,val);
}
	
void wm8994_set_fmradio_speaker(struct snd_soc_codec *codec)
{
	struct wm8994_priv *wm8994 = codec->private_data;

	u16 val;

	DEBUG_LOG("Routing spk path : FM Radio -> SPK Out");

	wm8994->fmradio_path = FMR_SPK;

	//Disable end point for preventing pop up noise.
	val = wm8994_read(codec,WM8994_POWER_MANAGEMENT_1);
	val &= ~(WM8994_SPKOUTL_ENA_MASK);
	wm8994_write(codec, WM8994_POWER_MANAGEMENT_1, val);

	wm8994_set_fmradio_common(codec);

	if(!wm8994->testmode_config_flag)
	{	
		// Unmute the SPKMIXVOLUME
		val = wm8994_read(codec, WM8994_SPKMIXL_ATTENUATION);
		val &= ~(WM8994_SPKMIXL_VOL_MASK);
		val |= TUNING_SPKMIXL_ATTEN;	
		wm8994_write(codec, WM8994_SPKMIXL_ATTENUATION, val);
			
		val = wm8994_read(codec, WM8994_SPKMIXR_ATTENUATION);
		val &= ~(WM8994_SPKMIXR_VOL_MASK);
		wm8994_write(codec, WM8994_SPKMIXR_ATTENUATION, val);
	
		val = wm8994_read(codec, WM8994_SPEAKER_VOLUME_LEFT);
		val &= ~(WM8994_SPKOUTL_MUTE_N_MASK | WM8994_SPKOUTL_VOL_MASK);
		val |= (WM8994_SPKOUTL_MUTE_N | TUNING_FMRADIO_SPKL_VOL);
		wm8994_write(codec, WM8994_SPEAKER_VOLUME_LEFT, val);
	
		val = wm8994_read(codec, WM8994_SPEAKER_VOLUME_RIGHT);
		val &= ~(WM8994_SPKOUTR_MUTE_N_MASK | WM8994_SPKOUTR_VOL_MASK);
		wm8994_write(codec, WM8994_SPEAKER_VOLUME_RIGHT, val);
	
		val = wm8994_read(codec, WM8994_CLASSD);
		val &= ~(WM8994_SPKOUTL_BOOST_MASK);
		val |= TUNING_FMRADIO_CLASSD_VOL << WM8994_SPKOUTL_BOOST_SHIFT;
		wm8994_write(codec, WM8994_CLASSD, val);
		
	}
	
	/*Output MIxer-Output PGA*/
	val = wm8994_read(codec,WM8994_SPKOUT_MIXERS );
	val &= ~(WM8994_SPKMIXL_TO_SPKOUTL_MASK | 
		WM8994_SPKMIXR_TO_SPKOUTL_MASK | WM8994_SPKMIXR_TO_SPKOUTR_MASK);
	val |= (WM8994_SPKMIXL_TO_SPKOUTL);
	wm8994_write(codec, WM8994_SPKOUT_MIXERS, val);

	// Output mixer setting
	val = wm8994_read(codec, WM8994_SPEAKER_MIXER);
	val &= ~(WM8994_MIXINL_TO_SPKMIXL_MASK | WM8994_MIXINR_TO_SPKMIXR_MASK);
	val |= (WM8994_MIXINL_TO_SPKMIXL);
	wm8994_write(codec, WM8994_SPEAKER_MIXER, val);

	// Enable SPK Volume.
	val = wm8994_read(codec, WM8994_POWER_MANAGEMENT_3);
	val &= ~(WM8994_SPKRVOL_ENA_MASK | WM8994_SPKLVOL_ENA_MASK);
	val |= (WM8994_SPKLVOL_ENA);
	wm8994_write(codec, WM8994_POWER_MANAGEMENT_3, val);
	
	//Enbale bias,vmid and Left speaker
	val = wm8994_read(codec,WM8994_POWER_MANAGEMENT_1);
	val &= ~(WM8994_BIAS_ENA_MASK | WM8994_VMID_SEL_MASK |WM8994_SPKOUTL_ENA_MASK);
	val |= (WM8994_BIAS_ENA | WM8994_VMID_SEL_NORMAL | WM8994_SPKOUTL_ENA);  
	wm8994_write(codec, WM8994_POWER_MANAGEMENT_1, val);
}

void wm8994_set_fmradio_headset_mix(struct snd_soc_codec *codec)
{	
	struct wm8994_priv *wm8994 = codec->private_data;

	DEBUG_LOG("");

	wm8994->fmradio_path = FMR_HP_MIX;

	wm8994_set_playback_headset(codec);
	wm8994_set_fmradio_headset(codec);
}

void wm8994_set_fmradio_speaker_mix(struct snd_soc_codec *codec)
{
	struct wm8994_priv *wm8994 = codec->private_data;

	DEBUG_LOG("");

	wm8994->fmradio_path = FMR_SPK_MIX;

	wm8994_set_fmradio_speaker(codec);
	wm8994_set_playback_headset(codec);
}

void wm8994_set_fmradio_speaker_headset_mix(struct snd_soc_codec *codec)
{
	struct wm8994_priv *wm8994 = codec->private_data;

	DEBUG_LOG("");

	wm8994->fmradio_path = FMR_SPK_HP_MIX;

	wm8994_set_playback_speaker_headset(codec);
	wm8994_set_fmradio_headset(codec);
}

#if defined WM8994_REGISTER_DUMP
void wm8994_register_dump(struct snd_soc_codec *codec)
{
	int wm8994_register;

	for(wm8994_register = 0; wm8994_register <= 0x6; wm8994_register++)
		DEBUG_LOG_ERR("Register= [%X], Value = [%X]", wm8994_register, wm8994_read(codec, wm8994_register));

	DEBUG_LOG_ERR("Register= [%X], Value = [%X]", 0x15, wm8994_read(codec, 0x15));

	for(wm8994_register = 0x18; wm8994_register <= 0x3C; wm8994_register++)
		DEBUG_LOG_ERR("Register= [%X], Value = [%X]", wm8994_register, wm8994_read(codec, wm8994_register));

	DEBUG_LOG_ERR("Register= [%X], Value = [%X]", 0x4C, wm8994_read(codec, 0x4C));

	for(wm8994_register = 0x51; wm8994_register <= 0x5C; wm8994_register++)
		DEBUG_LOG_ERR("Register= [%X], Value = [%X]", wm8994_register, wm8994_read(codec, wm8994_register));

	DEBUG_LOG_ERR("Register= [%X], Value = [%X]", 0x60, wm8994_read(codec, 0x60));
	DEBUG_LOG_ERR("Register= [%X], Value = [%X]", 0x101, wm8994_read(codec, 0x101));
	DEBUG_LOG_ERR("Register= [%X], Value = [%X]", 0x110, wm8994_read(codec, 0x110));
	DEBUG_LOG_ERR("Register= [%X], Value = [%X]", 0x111, wm8994_read(codec, 0x111));

	for(wm8994_register = 0x200; wm8994_register <= 0x212; wm8994_register++)
		DEBUG_LOG_ERR("Register= [%X], Value = [%X]", wm8994_register, wm8994_read(codec, wm8994_register));
	
	for(wm8994_register = 0x220; wm8994_register <= 0x224; wm8994_register++)
		DEBUG_LOG_ERR("Register= [%X], Value = [%X]", wm8994_register, wm8994_read(codec, wm8994_register));
	
	for(wm8994_register = 0x240; wm8994_register <= 0x244; wm8994_register++)
		DEBUG_LOG_ERR("Register= [%X], Value = [%X]", wm8994_register, wm8994_read(codec, wm8994_register));

	for(wm8994_register = 0x300; wm8994_register <= 0x317; wm8994_register++)
		DEBUG_LOG_ERR("Register= [%X], Value = [%X]", wm8994_register, wm8994_read(codec, wm8994_register));

	for(wm8994_register = 0x400; wm8994_register <= 0x411; wm8994_register++)
		DEBUG_LOG_ERR("Register= [%X], Value = [%X]", wm8994_register, wm8994_read(codec, wm8994_register));

	for(wm8994_register = 0x420; wm8994_register <= 0x423; wm8994_register++)
		DEBUG_LOG_ERR("Register= [%X], Value = [%X]", wm8994_register, wm8994_read(codec, wm8994_register));

	for(wm8994_register = 0x440; wm8994_register <= 0x444; wm8994_register++)
		DEBUG_LOG_ERR("Register= [%X], Value = [%X]", wm8994_register, wm8994_read(codec, wm8994_register));

	for(wm8994_register = 0x450; wm8994_register <= 0x454; wm8994_register++)
		DEBUG_LOG_ERR("Register= [%X], Value = [%X]", wm8994_register, wm8994_read(codec, wm8994_register));

	for(wm8994_register = 0x480; wm8994_register <= 0x493; wm8994_register++)
		DEBUG_LOG_ERR("Register= [%X], Value = [%X]", wm8994_register, wm8994_read(codec, wm8994_register));

	for(wm8994_register = 0x4A0; wm8994_register <= 0x4B3; wm8994_register++)
		DEBUG_LOG_ERR("Register= [%X], Value = [%X]", wm8994_register, wm8994_read(codec, wm8994_register));
	
	for(wm8994_register = 0x500; wm8994_register <= 0x503; wm8994_register++)
		DEBUG_LOG_ERR("Register= [%X], Value = [%X]", wm8994_register, wm8994_read(codec, wm8994_register));
	
	DEBUG_LOG_ERR("Register= [%X], Value = [%X]", 0x510, wm8994_read(codec, 0x510));
	DEBUG_LOG_ERR("Register= [%X], Value = [%X]", 0x520, wm8994_read(codec, 0x520));
	DEBUG_LOG_ERR("Register= [%X], Value = [%X]", 0x521, wm8994_read(codec, 0x521));

	for(wm8994_register = 0x540; wm8994_register <= 0x544; wm8994_register++)
		DEBUG_LOG_ERR("Register= [%X], Value = [%X]", wm8994_register, wm8994_read(codec, wm8994_register));
	
	for(wm8994_register = 0x580; wm8994_register <= 0x593; wm8994_register++)
		DEBUG_LOG_ERR("Register= [%X], Value = [%X]", wm8994_register, wm8994_read(codec, wm8994_register));

	for(wm8994_register = 0x600; wm8994_register <= 0x614; wm8994_register++)
		DEBUG_LOG_ERR("Register= [%X], Value = [%X]", wm8994_register, wm8994_read(codec, wm8994_register));
	
	DEBUG_LOG_ERR("Register= [%X], Value = [%X]", 0x620, wm8994_read(codec, 0x620));
	DEBUG_LOG_ERR("Register= [%X], Value = [%X]", 0x621, wm8994_read(codec, 0x621));

	for(wm8994_register = 0x700; wm8994_register <= 0x70A; wm8994_register++)
		DEBUG_LOG_ERR("Register= [%X], Value = [%X]", wm8994_register, wm8994_read(codec, wm8994_register));
		
}
#endif