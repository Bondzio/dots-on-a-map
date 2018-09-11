/* Version: mss_v6_3 */
/*
 * fips.h
 *
 * FIPS 140 Compliance
 *
 * Copyright Mocana Corp 2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */


#ifndef __FIPS_HEADER__
#define __FIPS_HEADER__

#ifdef __cplusplus
extern "C" {
#endif


#if (defined(__ZEROIZE_TEST__))
#if (defined(__KERNEL__))
#include <linux/kernel.h>       /* for printk */
#define FIPS_PRINT              printk
#else
#include <stdio.h>              /* for printf */
#define FIPS_PRINT              printf
#endif
#endif /* (defined(__ZEROIZE_TEST__)) */


#ifdef __ENABLE_MOCANA_FIPS_MODULE__

#include "../common/random.h"

/*-------------------------------------------------------------------*/
/* MOCANA FIPS AlgoID numbers used to enable/disable FIPS algorithms */
/*-------------------------------------------------------------------*/
enum FIPSAlgoNames
{
	FIPS_ALGO_RNG         = 0,
	FIPS_ALGO_RNG_FIPS186 = 1,
	FIPS_ALGO_RNG_CTR     = 2,

	FIPS_ALGO_SHA1   = 3,
	FIPS_ALGO_SHA256 = 4,
	FIPS_ALGO_SHA512 = 5,

	FIPS_ALGO_HMAC = 6,
	FIPS_ALGO_3DES = 7,

	FIPS_ALGO_AES      = 8,
	FIPS_ALGO_AES_ECB  = 9,
	FIPS_ALGO_AES_CBC  = 10,
	FIPS_ALGO_AES_CFB  = 11,
	FIPS_ALGO_AES_OFB  = 12,
	FIPS_ALGO_AES_CCM  = 13,
	FIPS_ALGO_AES_CTR  = 14,
	FIPS_ALGO_AES_CMAC = 15,
	FIPS_ALGO_AES_GCM  = 16,
	FIPS_ALGO_AES_XTS  = 17,

	FIPS_ALGO_ECC    = 18,
	FIPS_ALGO_ECDH   = 19,
	FIPS_ALGO_ECDSA  = 20,

	FIPS_ALGO_DH     = 21,

	FIPS_ALGO_RSA    = 22,
	FIPS_ALGO_DSA    = 23,

	/*------------------------------------*/
	/* Used to get overall startup status */
	/*------------------------------------*/
	FIPS_ALGO_ALL = 24,

};

#define NUM_FIPS_ALGONAME_VALUES ((FIPS_ALGO_ALL+1))

typedef struct FIPSRuntimeConfig
{
	enum FIPSAlgoNames   randomDefaultAlgo;			/* Must be 	FIPS_ALGO_RNG_FIPS186 or ..._RNG_CTR, */
    intBoolean      useInternalEntropy;
    intBoolean		algoEnabled[NUM_FIPS_ALGONAME_VALUES];
    char            *libPath;
    char            *sigPath;
} FIPSRuntimeConfig;

MOC_EXTERN MSTATUS getFIPS_powerupStatus(int fips_algoid);

MOC_EXTERN void setFIPS_Status(int fips_algoid, MSTATUS statusValue);

MOC_EXTERN MSTATUS FIPS_getDefaultConfig(FIPSRuntimeConfig *pfips_config);

/* Main FIPS Startup tests */
MOC_EXTERN MSTATUS FIPS_powerupSelfTest(void);
MOC_EXTERN MSTATUS FIPS_powerupSelfTestEx(FIPSRuntimeConfig *pfips_config);

#endif /* __ENABLE_MOCANA_FIPS_MODULE__ */

#ifdef __cplusplus
}
#endif

#endif /* __FIPS_HEADER__ */
