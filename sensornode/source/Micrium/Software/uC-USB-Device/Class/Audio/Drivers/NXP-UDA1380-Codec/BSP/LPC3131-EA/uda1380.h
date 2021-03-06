/***********************************************************************
 * $Id:: uda1380.h 6184 2011-01-17 07:51:24Z ing03005                  $
 *
 * Project: UDA1380 CODEC definitions
 *
 * Description:
 *     Register and control data for the UDA1380 CODEC
 *
 ***********************************************************************
 * Software that is described herein is for illustrative purposes only
 * which provides customers with programming information regarding the
 * products. This software is supplied "AS IS" without any warranties.
 * NXP Semiconductors assumes no responsibility or liability for the
 * use of the software, conveys no license or title under any patent,
 * copyright, or mask work right to the product. NXP Semiconductors
 * reserves the right to make changes in the software without
 * notification. NXP Semiconductors also make no representation or
 * warranty that such application will be suitable for the specified
 * use without further testing or modification.
 **********************************************************************/

#ifndef UDA1380_H
#define UDA1380_H

#include "lpc_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

/* Register offsets */
#define UDA1380_REG_EVALCLK    0x00
#define UDA1380_REG_I2S        0x01
#define UDA1380_REG_PWRCTRL    0x02
#define UDA1380_REG_ANAMIX    0x03
#define UDA1380_REG_HEADAMP    0x04
#define UDA1380_REG_MSTRVOL    0x10
#define UDA1380_REG_MIXVOL    0x11
#define UDA1380_REG_MODEBBT    0x12
#define UDA1380_REG_MSTRMUTE  0x13
#define UDA1380_REG_MIXSDO      0x14
#define UDA1380_REG_DECVOL      0x20
#define UDA1380_REG_PGA          0x21
#define UDA1380_REG_ADC          0x22
#define UDA1380_REG_AGC          0x23
#define UDA1380_REG_L3          0x7f
#define UDA1380_REG_HEADPHONE 0x18
#define UDA1380_REG_DEC          0x28

/* UDA1380_REG_EVALCLK bit defines */
#define EVALCLK_ADC_EN           0x0800  // Enable ADC clock
#define EVALCLK_DEC_EN           0x0400  // Enable decimator clock
#define EVALCLK_DAC_EN           0x0200  // Enable DAC clock
#define EVALCLK_INT_EN           0x0100  // Enable interpolator clock
#define EVALCLK_ADC_SEL_SYSCLK   0x0020  // Select SYSCLK input for ADC clock
#define EVALCLK_ADC_SEL_WSPLL    0x0000  // Select WSPLL clock for ADC clock
#define EVALCLK_DAC_SEL_SYSCLK   0x0010  // Select SYSCLK input for DAC clock
#define EVALCLK_DAC_SEL_WSPLL    0x0000  // Select WSPLL clock for DAC clock
#define EVALCLK_SYSDIV_SEL(n)    ((n) << 2) // System clock input divider select
#define EVALCLK_WSPLL_SEL6_12K   0x0000  // WSPLL input freq selection = 6.25 to 12.5K
#define EVALCLK_WSPLL_SEL12_25K  0x0001  // WSPLL input freq selection = 12.5K to 25K
#define EVALCLK_WSPLL_SEL25_50K  0x0002  // WSPLL input freq selection = 25K to 50K
#define EVALCLK_WSPLL_SEL50_100K 0x0003  // WSPLL input freq selection = 50K to 100K

/* UDA1380_REG_PWRCTRL bit defines */
#define PWR_PON_PLL_EN           0x8000  // WSPLL enable
#define PWR_PON_HP_EN            0x2000  // Headphone driver enable
#define PWR_PON_DAC_EN           0x0400  // DAC power enable
#define PWR_PON_BIAS_EN          0x0100  // Power on bias enable (for ADC, AVC, and FSDAC)
#define PWR_EN_AVC_EN            0x0080  // Analog mixer enable
#define PWR_PON_AVC_EN           0x0040  // Analog mixer power enable
#define PWR_EN_LNA_EN            0x0010  // LNA and SDC power enable
#define PWR_EN_PGAL_EN           0x0008  // PGA left power enable
#define PWR_EN_ADCL_EN           0x0004  // ADC left power enable
#define PWR_EN_PGAR_EN           0x0002  // PGA right power enable
#define PWR_EN_ADCR_EN           0x0001  // ADC right power enable

/* UDA1380_REG_MSTRMUTE bit defines */
#define MSTRMUTE_MTM_EN          0x4000  // Master mute enable
#define MSTRMUTE_MT2_EN          0x0800
#define MSTRMUTE_MT1_EN          0x0080

/* UDA1380_REG_MODEBBT bit defines */
#define MODEBBT_BOOST_FLAT       0x0000  // Bits for selecting flat boost
#define MODEBBT_BOOST_FULL       0xC000  // Bits for selecting maximum boost
#define MODEBBT_BOOST_MASK       0xC000  // Bits for selecting boost mask

/* UDA1380_REG_MVC values */
#define UDA1380_VOLUME_MAX       0x0000
#define UDA1380_VOLUME_MIN       0x00F8
#define UDA1380_VOLUME_SILENCE   0x00FC

/* UDA1380_REG_PGA bit defines */
#define UDA1380_PGA_MT_ADC       0x8000

/* UDA1380_REG_ADC bit defines */
#define REG_ADC_ADCPOL_INV       0x1000
#define REG_ADC_VGA_CTRL(x)      ((x) << 8)
#define REG_ADC_SEL_LNA          0x0008
#define REG_ADC_SEL_MIC          0x0004
#define REG_ADC_SKIP_DCFIL       0x0002
#define REG_ADC_EN_DCFIL         0x0001

/* UDA1380_REG_ADC bit defines */
#define REG_AGC_TIME(x)      ((x) << 8)
#define REG_AGC_LEVEL(x)     ((x) << 2)
#define REG_AGC_EN               0x0001

/* UDA1380_REG_PWRCTRL bit defines */
#define REG_PWRCTRL_PON_PLL     0x8000
#define REG_PWRCTRL_PON_HP      0x2000
#define REG_PWRCTRL_PON_DAC     0x0400
#define REG_PWRCTRL_PON_BIAS    0x0100
#define REG_PWRCTRL_EN_AVC      0x0080
#define REG_PWRCTRL_PON_AVC     0x0040
#define REG_PWRCTRL_PON_LNA     0x0010
#define REG_PWRCTRL_PON_PGAL    0x0008
#define REG_PWRCTRL_PON_ADCL    0x0004
#define REG_PWRCTRL_PON_PGAR    0x0002
#define REG_PWRCTRL_PON_ADCR    0x0001

/* UDA1380_REG_EVALCLK bit defines */
#define REG_EVALCLK_EN_ADC      0x0800
#define REG_EVALCLK_EN_DEC      0x0400
#define REG_EVALCLK_EN_DAC      0x0200
#define REG_EVALCLK_EN_INT      0x0100
#define REG_EVALCLK_ADC_CLK     0x0020
#define REG_EVALCLK_DAC_CLK     0x0010


#ifdef __cplusplus
}
#endif

#endif /* UDA1380_H */
