/* Version: mss_v6_3 */
/*
 * fips.c
 *
 * FIPS 140-2 Self Test Compliance
 *
 * Copyright Mocana Corp 2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

#include "../common/moptions.h"

#ifdef __ENABLE_MOCANA_FIPS_MODULE__

#include "../common/mtypes.h"
#include "../common/mocana.h"
#include "../crypto/hw_accel.h"
#include "../common/mdefs.h"
#include "../common/merrors.h"
#include "../common/mrtos.h"
#include "../common/mstdlib.h"
#include "../common/vlong.h"
#include "../common/random.h"
#include "../common/debug_console.h"
#include "../common/memory_debug.h"
#include "../crypto/crypto.h"
#include "../crypto/aes.h"
#include "../crypto/aes_ecb.h"
#include "../crypto/aes_ctr.h"
#include "../crypto/aes_ccm.h"
#include "../crypto/aes_cmac.h"
#include "../crypto/aes_xts.h"
#include "../crypto/gcm.h"
#include "../crypto/des.h"
#include "../crypto/three_des.h"
#include "../crypto/md5.h"
#include "../crypto/sha1.h"
#include "../crypto/sha256.h"
#include "../crypto/sha512.h"
#include "../crypto/hmac.h"
#include "../crypto/dh.h"
#include "../crypto/rsa.h"
#include "../crypto/dsa.h"
#ifdef __ENABLE_MOCANA_ECC__
#include "../crypto/primefld.h"
#include "../crypto/primefld_priv.h"
#include "../crypto/primeec.h"
#endif /* __ENABLE_MOCANA_ECC__ */
#include "../crypto/nist_rng.h"
#include "../crypto/fips.h"
#include "../crypto/fips_priv.h"
#include "../harness/harness.h"


#if defined(__RTOS_LINUX__) || defined(__RTOS_ANDROID__)
#ifndef __ENABLE_MOCANA_CRYPTO_KERNEL_MODULE_FIPS__
#include <stdio.h>
#define __MOCANA_LINUX_SHARED_LIBRARY__
#else
#include <linux/string.h>
#include <linux/slab.h>
#endif
#endif

#ifdef __RTOS_VXWORKS__
#include <stdio.h>
#endif

#ifdef __RTOS_WIN32__
#include <stdio.h>
#include <windows.h>
#include <string.h>
#include <tchar.h>
#define DLL_NAME _T("mss_fips")
#define SIGNATURE_FILE (DLL_NAME _T(".sig"))
#endif

#ifdef __RTOS_WINCE__
#include <stdio.h>
#include <windows.h>
#include <string.h>
#include <tchar.h>
#define DLL_NAME _T("mss_ce_dll")
#define SIGNATURE_FILE (DLL_NAME _T(".sig"))
#endif

#if (defined(__KERNEL__))
#include <linux/kernel.h>       /* for printk */
#define DBG_PRINT              printk
#else
#include <stdio.h>              /* for printf */
#define DBG_PRINT              printf
#endif

static FIPSRuntimeConfig sCurrRuntimeConfig; /* What are we configured to run */
static volatile FIPSStartupStatus sCurrStatus; /* What has passed (or not). */

#define ALGOID_INRANGE(MINID,MAXID,PASSID) ( ((PASSID >= MINID) && (PASSID <= MAXID)) )

#define ALGO_TESTSHOULDRUN(ALGOID) ( (sCurrStatus.algoEnabled[ALGOID] == TRUE) )

static void FIPS_ZeroStartupStatus(FIPSRuntimeConfig *pfips_config)
{
	int i = 0;

    sCurrStatus.integrityTestStatus = ERR_FIPS_SELF_TEST_INCOMPLETE;
    sCurrStatus.globalFIPS_powerupStatus = ERR_FIPS_SELF_TEST_INCOMPLETE;
    sCurrStatus.startupState = FIPS_SS_INIT;

    for (i = FIPS_ALGO_RNG; i < FIPS_ALGO_ALL; i++)
    {
		sCurrStatus.algoEnabled[i] = pfips_config->algoEnabled[i];

    	if (sCurrStatus.algoEnabled[i])
    	{
    		sCurrStatus.algoStatus[i] = ERR_FIPS_SELF_TEST_INCOMPLETE;
    	}
    	else
    	{
    		sCurrStatus.algoStatus[i] = ERR_RTOS;
    	}
    }

}

static void FIPS_MarkAllAlgosStartupTBD(FIPSRuntimeConfig *pfips_config)
{
	int i = 0;

    for (i = FIPS_ALGO_RNG; i < FIPS_ALGO_ALL; i++)
    {
		sCurrStatus.algoEnabled[i] = pfips_config->algoEnabled[i];
   		sCurrStatus.algoStatus[i] = ERR_FIPS_SELF_TEST_INCOMPLETE;
    }

}

MOC_EXTERN MSTATUS FIPS_getDefaultConfig(FIPSRuntimeConfig *pfips_config)
{

    if (pfips_config == NULL)
    	return ERR_INVALID_ARG;

    MOC_MEMSET((void *)pfips_config, 0x00, sizeof(FIPSRuntimeConfig));

    /************************************************/
    /* Set default Random # algorithm               */
    /************************************************/
if (RANDOM_DEFAULT_ALGO == MODE_RNG_FIPS186)
	 pfips_config->randomDefaultAlgo = FIPS_ALGO_RNG_FIPS186;
else if (RANDOM_DEFAULT_ALGO == MODE_DRBG_CTR)
	 pfips_config->randomDefaultAlgo = FIPS_ALGO_RNG_CTR;
else
    pfips_config->randomDefaultAlgo = FIPS_ALGO_RNG_FIPS186;

/************************************************/
/* Set default Random # Entropy source flag     */
/************************************************/
#ifdef __DISABLE_MOCANA_RAND_ENTROPY_THREADS__
	pfips_config->useInternalEntropy = FALSE;
#else
	pfips_config->useInternalEntropy = TRUE;
#endif

    /************************************************/
    /* Set default lib & sig Path to null           */
    /************************************************/
	pfips_config->libPath = NULL; /* default to compile time path */
	pfips_config->sigPath = NULL; /* default to compile time path */

    /****************************************************************/
    /* Set list of startup tests to run based on compile time flags */
    /****************************************************************/
	pfips_config->algoEnabled[FIPS_ALGO_RNG] = TRUE;
	pfips_config->algoEnabled[FIPS_ALGO_RNG_FIPS186] = TRUE;

#ifdef __ENABLE_MOCANA_RNG_DRBG_CTR__
	pfips_config->algoEnabled[FIPS_ALGO_RNG_CTR] = TRUE;
#else
	pfips_config->algoEnabled[FIPS_ALGO_RNG_CTR] = FALSE;
#endif

#ifdef __ENABLE_MOCANA_FIPS_SHA1__
	pfips_config->algoEnabled[FIPS_ALGO_SHA1]   = TRUE;
#else
	pfips_config->algoEnabled[FIPS_ALGO_SHA1]   = FALSE;
#endif

#ifdef __ENABLE_MOCANA_FIPS_SHA256__
	pfips_config->algoEnabled[FIPS_ALGO_SHA256] = TRUE;
#else
	pfips_config->algoEnabled[FIPS_ALGO_SHA256] = FALSE;
#endif

#ifdef __ENABLE_MOCANA_FIPS_SHA512__
	pfips_config->algoEnabled[FIPS_ALGO_SHA512] = TRUE;
#else
	pfips_config->algoEnabled[FIPS_ALGO_SHA512] = FALSE;
#endif

	pfips_config->algoEnabled[FIPS_ALGO_HMAC] = TRUE;

#ifdef __ENABLE_MOCANA_FIPS_3DES__
	pfips_config->algoEnabled[FIPS_ALGO_3DES] = TRUE;
#else
	pfips_config->algoEnabled[FIPS_ALGO_3DES] = FALSE;
#endif

#ifdef __ENABLE_MOCANA_FIPS_AES__
	pfips_config->algoEnabled[FIPS_ALGO_AES] = TRUE;
	pfips_config->algoEnabled[FIPS_ALGO_AES_ECB] = TRUE;
	pfips_config->algoEnabled[FIPS_ALGO_AES_CBC] = TRUE;
	pfips_config->algoEnabled[FIPS_ALGO_AES_CFB] = TRUE;
	pfips_config->algoEnabled[FIPS_ALGO_AES_OFB] = TRUE;
	pfips_config->algoEnabled[FIPS_ALGO_AES_CCM] = TRUE;
	pfips_config->algoEnabled[FIPS_ALGO_AES_CTR] = TRUE;
	pfips_config->algoEnabled[FIPS_ALGO_AES_CMAC] = TRUE;
	pfips_config->algoEnabled[FIPS_ALGO_AES_XTS] = TRUE;
#else
	pfips_config->algoEnabled[FIPS_ALGO_AES] = FALSE;
	pfips_config->algoEnabled[FIPS_ALGO_AES_ECB] = FALSE;
	pfips_config->algoEnabled[FIPS_ALGO_AES_CBC] = FALSE;
	pfips_config->algoEnabled[FIPS_ALGO_AES_CFB] = FALSE;
	pfips_config->algoEnabled[FIPS_ALGO_AES_OFB] = FALSE;
	pfips_config->algoEnabled[FIPS_ALGO_AES_CCM] = FALSE;
	pfips_config->algoEnabled[FIPS_ALGO_AES_CTR] = FALSE;
	pfips_config->algoEnabled[FIPS_ALGO_AES_CMAC] = FALSE;
	pfips_config->algoEnabled[FIPS_ALGO_AES_XTS] = FALSE;
#endif

#ifdef __ENABLE_MOCANA_GCM__
	pfips_config->algoEnabled[FIPS_ALGO_AES_GCM] = TRUE;
#else
	pfips_config->algoEnabled[FIPS_ALGO_AES_GCM] = FALSE;
#endif

#ifdef __ENABLE_MOCANA_ECC__
	pfips_config->algoEnabled[FIPS_ALGO_ECC] = TRUE;
#else
	pfips_config->algoEnabled[FIPS_ALGO_ECC] = FALSE;
#endif

#if (!defined(__ENABLE_MOCANA_CRYPTO_KERNEL_MODULE_FIPS__))

#ifdef __ENABLE_MOCANA_FIPS_ECDH__
	pfips_config->algoEnabled[FIPS_ALGO_ECDH] = TRUE;
#else
	pfips_config->algoEnabled[FIPS_ALGO_ECDH] = FALSE;
#endif

#ifdef __ENABLE_MOCANA_FIPS_ECDSA__
	pfips_config->algoEnabled[FIPS_ALGO_ECDSA] = TRUE;
#else
	pfips_config->algoEnabled[FIPS_ALGO_ECDSA] = FALSE;
#endif

	pfips_config->algoEnabled[FIPS_ALGO_DH] = TRUE;

#ifdef __ENABLE_MOCANA_FIPS_RSA__
	pfips_config->algoEnabled[FIPS_ALGO_RSA] = TRUE;
#else
	pfips_config->algoEnabled[FIPS_ALGO_RSA] = FALSE;
#endif

#ifdef __ENABLE_MOCANA_FIPS_DSA__
	pfips_config->algoEnabled[FIPS_ALGO_DSA] = TRUE;
#else
	pfips_config->algoEnabled[FIPS_ALGO_DSA] = FALSE;
#endif

#else /* (!defined(__ENABLE_MOCANA_CRYPTO_KERNEL_MODULE_FIPS__)) */
	pfips_config->algoEnabled[FIPS_ALGO_ECDH]  = FALSE;
	pfips_config->algoEnabled[FIPS_ALGO_ECDSA] = FALSE;
	pfips_config->algoEnabled[FIPS_ALGO_DH]    = FALSE;
	pfips_config->algoEnabled[FIPS_ALGO_RSA]   = FALSE;
	pfips_config->algoEnabled[FIPS_ALGO_DSA]   = FALSE;
#endif /* (!defined(__ENABLE_MOCANA_CRYPTO_KERNEL_MODULE_FIPS__)) */

	/*****************************************
	 * Fix ECC inter-related parameters
     * */
	if ( (pfips_config->algoEnabled[FIPS_ALGO_ECDH] != TRUE) &&
		 (pfips_config->algoEnabled[FIPS_ALGO_ECDSA] != TRUE) )
	{
		/* If all of these ECC-based tests are FALSE, then ECC must be FALSE. */
		pfips_config->algoEnabled[FIPS_ALGO_ECC] = FALSE;
	}

	pfips_config->algoEnabled[FIPS_ALGO_ALL] = FALSE;
	/*****************************************/


	/*****************************************
	 * If libPath and sigPath are NULL, then we will use the compile time paths.
	 *
	 */
	pfips_config->libPath = NULL;
	pfips_config->sigPath = NULL;

	return OK;
}

static MSTATUS FIPS_validateAndCopyConfig(FIPSRuntimeConfig *pfips_config)
{
	MSTATUS status = ERR_FIPS;
	FIPSRuntimeConfig *pcompiletime = &sCurrRuntimeConfig;
	int i;

    if (pfips_config == NULL)
    	return ERR_INVALID_ARG;

    /* start w/ compile-time config. */
	if (OK != (status = FIPS_getDefaultConfig(pcompiletime)))
		goto exit;

	/* Validate the config passed in */
	status = ERR_FIPS;

	/* First validate RNG parameters */
	if ( (pfips_config->algoEnabled[FIPS_ALGO_RNG] != TRUE) ||
			(   (pfips_config->algoEnabled[FIPS_ALGO_RNG_FIPS186] != TRUE) &&
					(pfips_config->algoEnabled[FIPS_ALGO_RNG_CTR] != TRUE) ))
	{
#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
        DEBUG_PRINTNL( DEBUG_CRYPTO, (sbyte *) (sbyte *)"FIPS_validateAndCopyConfig() No RNG algo enabled." );
#endif
		goto exit;
	}

	/* Validate Entropy compile-time and run-time flags */
#ifdef __DISABLE_MOCANA_RAND_ENTROPY_THREADS__
	if (pfips_config->useInternalEntropy == TRUE)
	{
#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
        DEBUG_PRINTNL( DEBUG_CRYPTO, (sbyte *) (sbyte *)"FIPS_validateAndCopyConfig() useInternalEntropy not enabled in binary." );
#endif
		goto exit;
	}
#endif

	/* Validate default RNG Algo */
	if (ALGOID_INRANGE(FIPS_ALGO_RNG_FIPS186,FIPS_ALGO_RNG_CTR,pfips_config->randomDefaultAlgo))
	{
		if (pfips_config->algoEnabled[pfips_config->randomDefaultAlgo] != TRUE)
		{
#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
			DEBUG_PRINTNL( DEBUG_CRYPTO, (sbyte *) (sbyte *)"FIPS_validateAndCopyConfig() RNG Default Algo not enabled." );
#endif
			goto exit;
		}
	}
	else
	{
#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
		DEBUG_PRINTNL( DEBUG_CRYPTO, (sbyte *) (sbyte *)"FIPS_validateAndCopyConfig() invalid RNG Default Algo parameter." );
#endif
		goto exit;
	}

	/* Validate ECC related parameters */
	if ( (pfips_config->algoEnabled[FIPS_ALGO_ECC] != TRUE) &&
	     (  (pfips_config->algoEnabled[FIPS_ALGO_ECDH] == TRUE) ||
			(pfips_config->algoEnabled[FIPS_ALGO_ECDSA] == TRUE) ))
	{
		/* If any of these are TRUE, then ECC must be TRUE. */
#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
		DEBUG_PRINTNL( DEBUG_CRYPTO, (sbyte *) (sbyte *)"FIPS_validateAndCopyConfig() ECC not enabled." );
#endif
		goto exit;
	}

	/* Validate ECC related parameters */
	if ( (pfips_config->algoEnabled[FIPS_ALGO_ECC] == TRUE) &&
	     (  (pfips_config->algoEnabled[FIPS_ALGO_ECDH] != TRUE) &&
			(pfips_config->algoEnabled[FIPS_ALGO_ECDSA] != TRUE) ))
	{
		/* If ECC is TRUE, then at least one of these must be TRUE. */
#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
		DEBUG_PRINTNL( DEBUG_CRYPTO, (sbyte *) (sbyte *)"FIPS_validateAndCopyConfig() ECC derivative tests not enabled." );
#endif
		goto exit;
	}

	/* FIPS Integ test needs SHA1 & HMAC */
	if ( (pfips_config->algoEnabled[FIPS_ALGO_SHA1] != TRUE) ||
		 (pfips_config->algoEnabled[FIPS_ALGO_HMAC] != TRUE) )
	{
#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
		DEBUG_PRINTNL( DEBUG_CRYPTO, (sbyte *) (sbyte *)"FIPS_validateAndCopyConfig() SHA1 & HMAC not enabled." );
#endif
		goto exit;
	}

	/* Make sure all enabled algos were compiled in */
    for (i = FIPS_ALGO_RNG; i < FIPS_ALGO_ALL; i++)
    {
    	if ( (pcompiletime->algoEnabled[i] == FALSE) &&
    		 (pfips_config->algoEnabled[i] == TRUE) )
    	{
#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
		DEBUG_PRINTNL( DEBUG_CRYPTO, (sbyte *) (sbyte *)"FIPS_validateAndCopyConfig() Algorithm not enabled in binary." );
#endif
    			goto exit;
    	}
    }

    status = OK;

    /* First shallow copy everything. */
	MOC_MEMCPY(&sCurrRuntimeConfig, pfips_config, sizeof(FIPSRuntimeConfig));

	/* Now deep copy pointers. */
	if (pfips_config->libPath != NULL)
	{
		int liblen = MOC_STRLEN((const sbyte *)pfips_config->libPath);
		if (liblen != 0)
		{
			sCurrRuntimeConfig.libPath = MALLOC(liblen+1);
			if (sCurrRuntimeConfig.libPath)
			{
				MOC_STRCBCPY( (sbyte*) sCurrRuntimeConfig.libPath, liblen+1, (const sbyte*)pfips_config->libPath);
			}
		}
	}

	/* Now deep copy pointers. */
	if (pfips_config->sigPath != NULL)
	{
		int siglen = MOC_STRLEN((const sbyte *)pfips_config->sigPath);
		if (siglen != 0)
		{
			sCurrRuntimeConfig.sigPath = MALLOC(siglen+1);
			if (sCurrRuntimeConfig.sigPath)
			{
				MOC_STRCBCPY( (sbyte*) sCurrRuntimeConfig.sigPath, siglen+1, (const sbyte*)pfips_config->sigPath);
			}
		}
	}

exit:
#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
	if (status != OK)
        DEBUG_PRINTNL( DEBUG_CRYPTO, (sbyte *) (sbyte *)"FIPS_validateAndCopyConfig() returning FAILURE" );
#endif
	return status;
}


static void FIPS_DumpStartupStatusData(void)
{
#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
	int i = 0;
	char dbgbuff[60];

	DEBUG_PRINTNL( DEBUG_CRYPTO, (sbyte *) "===================================================================");

	sprintf(dbgbuff,"sCurrStatus.startupState: %d",sCurrStatus.startupState);
    DEBUG_PRINTNL( DEBUG_CRYPTO, (sbyte *) dbgbuff );

	sprintf(dbgbuff,"sCurrStatus.globalFIPS_powerupStatus: %d",sCurrStatus.globalFIPS_powerupStatus);
    DEBUG_PRINTNL( DEBUG_CRYPTO, (sbyte *) dbgbuff );

	sprintf(dbgbuff,"sCurrStatus.integrityTestStatus: %d",sCurrStatus.integrityTestStatus);
    DEBUG_PRINTNL( DEBUG_CRYPTO, (sbyte *) dbgbuff );

	sprintf(dbgbuff,"sCurrStatus.startupShouldFail: %d",sCurrStatus.startupShouldFail);
    DEBUG_PRINTNL( DEBUG_CRYPTO, (sbyte *) dbgbuff );

	sprintf(dbgbuff,"sCurrStatus.startupFailTestNumber: %d",sCurrStatus.startupFailTestNumber);
    DEBUG_PRINTNL( DEBUG_CRYPTO, (sbyte *) dbgbuff );

    DEBUG_PRINTNL( DEBUG_CRYPTO, (sbyte *) "---------------------------------------------------------");
    for (i = FIPS_ALGO_RNG; i < FIPS_ALGO_ALL; i++)
    {
    	sprintf(dbgbuff,"sCurrStatus.algo[%d] Enabled=%s  :  Status=%d",i,((sCurrStatus.algoEnabled[i])?"TRUE ":"FALSE"), sCurrStatus.algoStatus[i]);
        DEBUG_PRINTNL( DEBUG_CRYPTO, (sbyte *) dbgbuff );
    }
    DEBUG_PRINTNL( DEBUG_CRYPTO, (sbyte *) "===================================================================");
#endif

}


MOC_EXTERN MSTATUS getFIPS_powerupStatus(int fips_algoid)
{
	MSTATUS status = OK;
	int i, children_enabled;
	int firstchild, lastchild;
	intBoolean checkchildren;

	if ((fips_algoid < FIPS_ALGO_RNG) || (fips_algoid > FIPS_ALGO_ALL))
		return ERR_FIPS;

	/* First check the global. If it is an err, then we are done. */
	if (OK != (status = sCurrStatus.globalFIPS_powerupStatus))
		goto exit;

	if (sCurrStatus.startupState != FIPS_SS_INPROCESS)
	{
		/* During Startup Selftest, we don't check individual algos, since we haven't
		 * initialized them yet...
		 */
		checkchildren = FALSE;
		switch (fips_algoid)
		{
		case FIPS_ALGO_RNG:
			/* Check the parent first */
			if (OK != (status = sCurrStatus.algoStatus[fips_algoid]))
				break;

			checkchildren = TRUE;
			firstchild = FIPS_ALGO_RNG_FIPS186;
			lastchild = FIPS_ALGO_RNG_CTR;

			break;

		case FIPS_ALGO_AES:
			/* Check the parent first */
			if (OK != (status = sCurrStatus.algoStatus[fips_algoid]))
				break;

			checkchildren = TRUE;
			firstchild = FIPS_ALGO_AES_ECB;
			lastchild = FIPS_ALGO_AES_XTS;

			break;

		case FIPS_ALGO_ALL:
			/* Check all enabled algos */
			for (i = FIPS_ALGO_RNG; i < FIPS_ALGO_ALL; i++)
			{
				if (sCurrStatus.algoEnabled[fips_algoid] == TRUE)
				{
					if (OK != (status = sCurrStatus.algoStatus[fips_algoid]))
						break;
				}
			}
			break;

		default:
			/* General case: all other algos: Just return the status of that algo
			 * which will be ERR_FIPS_SELF_TEST_INCOMPLETE if its startup test wasn't run.
			 */
			status = sCurrStatus.algoStatus[fips_algoid];
			break;

		}

		if (checkchildren == TRUE)
		{
			/* Check the children */
			children_enabled = 0;
			for (i = firstchild; i <= lastchild; i++)
			{
				if (sCurrStatus.algoEnabled[i] == TRUE)
					children_enabled++;
				if (OK != (status = sCurrStatus.algoStatus[fips_algoid]))
					break;
			}
			if (children_enabled == 0)
				status = ERR_FIPS_SELF_TEST_INCOMPLETE;
		}

	} /* endif not in FIPS_SS_INPROCESS state */


exit:

#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
	if (status != OK)
	{
		char dbgbuff[60];
		sprintf(dbgbuff,"getFIPS_powerupStatus(%d) returning %d\n",fips_algoid,status);
        DEBUG_PRINTNL( DEBUG_CRYPTO, (sbyte *) dbgbuff );
	}
#endif

	return status;

}

MOC_EXTERN void setFIPS_Status(int fips_algoid, MSTATUS statusValue)
{
#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
	char dbgbuff[60];
#endif

	if ((fips_algoid < FIPS_ALGO_RNG) || (fips_algoid > FIPS_ALGO_ALL))
		return;

#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
    /* Shouldn't we be using DB_PRINT here instead of sprintf ? */
	sprintf(dbgbuff,"setFIPS_Status(%d) setting %d",fips_algoid,statusValue);
    DEBUG_PRINTNL( DEBUG_CRYPTO, (sbyte *) dbgbuff );
#endif

	sCurrStatus.algoStatus[fips_algoid] = statusValue;
	if (statusValue != OK)
	{
		/* If anything breaks, break them all */
		sCurrStatus.globalFIPS_powerupStatus = statusValue;

		sCurrStatus.globalFIPS_powerupStatus = statusValue;
	}

}

MOC_EXTERN void setFIPS_Status_Once(int fips_algoid, MSTATUS statusValue)
{

	if ((fips_algoid < FIPS_ALGO_RNG) || (fips_algoid >FIPS_ALGO_ALL))
		return;

	if (statusValue == OK)
	{
		if (sCurrStatus.algoStatus[fips_algoid] == ERR_FIPS_SELF_TEST_INCOMPLETE)
		{
			setFIPS_Status(fips_algoid, statusValue);
		}
	}
	else
	{
		setFIPS_Status(fips_algoid, statusValue);
	}
}


/*--------------------------------------------------------*/
static void FIPS_StartupTests_SaveResults(MSTATUS status)
{
	int i;

	if (status == OK)
	{
	    for (i = FIPS_ALGO_RNG; i < FIPS_ALGO_ALL; i++)
	    {
			if (sCurrStatus.algoEnabled[i])
			{
				if (sCurrStatus.algoStatus[i] != OK)
				{
#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
                    char debugprintbuf[80];
#endif

                    status = sCurrStatus.algoStatus[i]; /* Not OK. */
#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
                    /* Shouldn't we be using DB_PRINT here instead of sprintf ? */
				    sprintf(debugprintbuf,"FIPS_StartupTests_SaveResults: Test %d FAILED with %d",i,sCurrStatus.algoStatus[i]);
				    DEBUG_PRINTNL(DEBUG_CRYPTO, (sbyte *)debugprintbuf);
#endif
				}
			}
			else
			{   /* If it is disabled, mark it as incomplete too. */
				sCurrStatus.algoStatus[i] = ERR_FIPS_SELF_TEST_INCOMPLETE;
			}
	    }
	}

#ifdef __ENABLE_MOCANA_FIPS_INTEG_TEST__
	if (sCurrStatus.integrityTestStatus != OK)
	{
			status = sCurrStatus.integrityTestStatus;
	}
#endif

	/*******************************/
	/* This is the important part. */
	/*******************************/
	sCurrStatus.globalFIPS_powerupStatus = status;
	sCurrStatus.startupState = FIPS_SS_DONE;

#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
	if (status == OK)
		DEBUG_PRINTNL(DEBUG_CRYPTO, (sbyte *) "FIPS_StartupTests_SaveResults: status = OK");
	else
		DEBUG_PRINTNL(DEBUG_CRYPTO, (sbyte *) "FIPS_StartupTests_SaveResults: status = FAILED");
#endif

}

/*--------------------------------------------------------*/
static intBoolean FIPS_StartupTests_ShouldBeRun(void)
{
#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
    DEBUG_PRINTNL(DEBUG_CRYPTO, (sbyte *) "FIPS_StartupTests_ShouldBeRun: Always: TRUE");
#endif
	return TRUE; /* Always run them */
}


/*--------------------------------------------------------*/
static void FIPS_StartupTests_ForceToRun(void)
{
	/* reset shared flag to force others to do it again */
	FIPS_ZeroStartupStatus(&sCurrRuntimeConfig);
}

/*------------------------------------------------------------------*/
/* FIPS force failure tests */
/*------------------------------------------------------------------*/
/* flag to say if we have already run startup tests.                */
/* In a flat shared memory (single-process RTOS), this is enough.   */
/*------------------------------------------------------------------*/
MOC_EXTERN void FIPS_triggerStartupFail(ubyte4 tn)
{
	sCurrStatus.startupShouldFail = 1;
	sCurrStatus.startupFailTestNumber = tn;
}

MOC_EXTERN void FIPS_resetStartupFail(void)
{
	FIPS_ZeroStartupStatus(&sCurrRuntimeConfig);
    sCurrStatus.startupShouldFail = 0;
    sCurrStatus.startupFailTestNumber = 0;
}


/*------------------------------------------------------------------*/

typedef struct aesCcmTestPacketDescr
{
    ubyte4          keyLen;
    ubyte           key[32];
    ubyte4          nonceLen;
    ubyte           nonce[16];
    ubyte4          packetLen;
    ubyte4          packetHeaderLen;
    ubyte           packet[36];
    ubyte4          resultLen;
    ubyte           result[50];

} aesCcmTestPacketDescr;

static aesCcmTestPacketDescr mAesCcmTestPackets =
{
     16,        /* key */
     {0xD7,0x82,0x8D,0x13, 0xB2,0xB0,0xBD,0xC3, 0x25,0xA7,0x62,0x36, 0xDF,0x93,0xCC,0x6B},
     13,        /* nonce */
     {0x00,0x8D,0x49,0x3B, 0x30,0xAE,0x8B,0x3C, 0x96,0x96,0x76,0x6C, 0xFA},
     33, 12,    /* packet */
     {0x6E,0x37,0xA6,0xEF, 0x54,0x6D,0x95,0x5D, 0x34,0xAB,0x60,0x59, 0xAB,0xF2,0x1C,0x0B,
      0x02,0xFE,0xB8,0x8F, 0x85,0x6D,0xF4,0xA3, 0x73,0x81,0xBC,0xE3, 0xCC,0x12,0x85,0x17,
      0xD4},
     43,        /* result */
     {0x6E,0x37,0xA6,0xEF, 0x54,0x6D,0x95,0x5D, 0x34,0xAB,0x60,0x59, 0xF3,0x29,0x05,0xB8,
      0x8A,0x64,0x1B,0x04, 0xB9,0xC9,0xFF,0xB5, 0x8C,0xC3,0x90,0x90, 0x0F,0x3D,0xA1,0x2A,
      0xB1,0x6D,0xCE,0x9E, 0x82,0xEF,0xA1,0x6D, 0xA6,0x20,0x59}
};


/*------------------------------------------------------------------*/

typedef struct
{

    ubyte   key[32];
    ubyte   tweak[16];
    ubyte   plainText[32];
    ubyte   cipherText[32];
} aesXtsTestPacketDescr;

static aesXtsTestPacketDescr AES_XTS_TESTCASE =
{
    /* 32 bytes key */
    { 0xa6,0x41,0xc6,0x36,0x21,0x95,0x93,0x5b,0x07,0x4f,0xbd,0x71,0xc5,0xa4,0xa9,0xb8,
        0x45,0x36,0x81,0x9c,0xa3,0x8f,0x06,0x67,0x10,0x19,0x5a,0xc2,0xbf,0x10,0xbf,0x2a },
    /* 16 byte tweak value */
    { 0x4f,0xab,0x20,0x5f,0x60,0xf2,0x8a,0x2b,0xd0,0x5d,0xa4,0x84,0x6b,0xf2,0x05,0x60 },
    /* 32 bytes plaintext */
    { 0x16,0x2a,0x8e,0x94,0xcd,0x5c,0x71,0x63,0xc5,0x9e,0xc0,0x84,0x30,0x69,0x0d,0xdf,
        0xc4,0x10,0x01,0xdf,0x59,0x26,0x45,0xa4,0xc9,0xc1,0xc9,0x16,0x09,0x1d,0x58,0x15 },
    /* corr. 32 byte ciphertext */
    { 0x47,0x93,0x93,0x55,0x55,0x96,0xbb,0x45,0x4e,0x0d,0xd0,0x49,0xdc,0xe5,0x52,0xf1,
        0x42,0x6c,0x6e,0xf1,0x63,0x00,0x28,0xdd,0x28,0x08,0x38,0xc3,0x93,0x9a,0xe6,0xb3 }
};

/*------------------------------------------------------------------*/

typedef struct 
{
    ubyte4  entropyInputLen;
    ubyte   entropyInput[48];
    ubyte4  nonceLen;
    ubyte   nonce[16];
    ubyte4  personalizationStrLen;
    ubyte   personalizationStr[48];
    ubyte4  additionalInput1Len;
    ubyte   additionalInput1[48];
    ubyte4  entropyInputPR1Len;
    ubyte   entropyInputPR1[48];
    ubyte4  additionalInput2Len;
    ubyte   additionalInput2[48];
    ubyte4  entropyInputPR2Len;
    ubyte   entropyInputPR2[48];
    ubyte4  resultLen;
    ubyte   result[32];
} NIST_DRBG_TestVectorPR;

/* NIST SP 800-90 DRBG testvectors */
static const NIST_DRBG_TestVectorPR kCTR_DRBG_AES256_DF_PR =
{
    /* CTR_DRBG.txt */
    /* [AES-256 use df] */
    /* [PredictionResistance = True] */
    /* [EntropyInputLen = 256] */
    /* [NonceLen = 128] */
    /* [PersonalizationStringLen = 256] */
    /* [AdditionalInputLen = 256] */
    /* COUNT = 0 */
    32,
    {0x2a,0x02,0xbe,0xaa, 0xba,0xb4,0x6a,0x73, 0x53,0x85,0xa9,0x2a, 0xae,0x4a,0xdc,0xeb,
     0xe8,0x07,0xfb,0xf3, 0xbc,0xe3,0xf4,0x2e, 0x00,0x53,0x46,0x00, 0x64,0x80,0xdd,0x57},    /* EntropyInput */
    16,
    {0x2c,0x86,0xa2,0xf9, 0x70,0xb5,0xca,0xd3, 0x9a,0x08,0xdc,0xb6, 0x6b,0xce,0xe5,0x05},    /* Nonce */
    32,
    {0xdb,0x6c,0xe1,0x84, 0xbe,0x07,0xae,0x55, 0x4e,0x34,0x5d,0xb8, 0x47,0x98,0x85,0xe0,
     0x3d,0x3e,0x9f,0x60, 0xfa,0x1c,0x7d,0x57, 0x19,0xe5,0x09,0xdc, 0xe2,0x10,0x41,0xab},    /* PersonalizationString */
    32,
    {0x1d,0xc3,0x11,0x93, 0xcb,0xc4,0xf6,0xbb, 0x57,0xb0,0x09,0x70, 0xb9,0xc6,0x05,0x86,
     0x4e,0x75,0x95,0x7d, 0x3d,0xec,0xce,0xb4, 0x0b,0xe4,0xef,0xd1, 0x7b,0xab,0x56,0x6f},    /* AdditionalInput */
    32,
    {0x8f,0xb9,0xab,0xf9, 0x33,0xcc,0xbe,0xc6, 0xbd,0x8b,0x61,0x5a, 0xec,0xc6,0x4a,0x5b,
     0x03,0x21,0xe7,0x37, 0x03,0x02,0xbc,0xa5, 0x28,0xb9,0xfe,0x7a, 0xa8,0xef,0x6f,0xb0},    /* EntropyInputPR */
    32,
    {0xd6,0x98,0x63,0x48, 0x94,0x9f,0x26,0xf7, 0x1f,0x44,0x13,0x23, 0xa7,0xde,0x09,0x12,
     0x90,0x04,0xce,0xbc, 0xac,0x82,0x70,0x58, 0xba,0x7d,0xdc,0x25, 0x1e,0xe4,0xbf,0x7c},    /* AdditionalInput */
    32,
    {0xe5,0x04,0xef,0x7c, 0x8d,0x02,0xd7,0x68, 0x95,0x4c,0x64,0x34, 0x30,0x3a,0xcb,0x07,
     0xc9,0x0a,0xef,0x26, 0xc6,0x57,0x43,0xfb, 0x7d,0xbe,0xe2,0x61, 0x75,0xcd,0xee,0x34},    /* EntropyInputPR */
    16,
    {0x75,0x6d,0x16,0xef, 0x14,0xae,0xd9,0xc2, 0x28,0x0b,0x66,0xff, 0x20,0x1f,0x21,0x33}     /* ReturnedBits */
};

static const NIST_DRBG_TestVectorPR kCTR_DRBG_AES256_NoDF_PR =
{
    /* CTR_DRBG.txt */
    /* [AES-256 no df] */
    /* [PredictionResistance = True] */
    /* [EntropyInputLen = 384] */
    /* [NonceLen = 128] */
    /* [PersonalizationStringLen = 384] */
    /* [AdditionalInputLen = 384] */
    /* COUNT = 0 */
    48,
    {0x7e,0x83,0x3f,0xa6, 0x39,0xdc,0xcb,0x38, 0x17,0x6a,0xa3,0x59, 0xa9,0x8c,0x1f,0x50,
     0xd3,0xdb,0x34,0xdd, 0xa4,0x39,0x65,0xe4, 0x77,0x17,0x08,0x57, 0x49,0x04,0xbd,0x68,
     0x5c,0x7d,0x2a,0xee, 0x0c,0xf2,0xfb,0x16, 0xef,0x16,0x18,0x4d, 0x32,0x6a,0x26,0x6c},    /* EntropyInput */
    16,
    {0xa3,0x8a,0xa4,0x6d, 0xa6,0xc1,0x40,0xf8, 0xa3,0x02,0xf1,0xac, 0xf3,0xea,0x7f,0x2d},    /* Nonce */
    48,
    {0xc0,0x54,0x1e,0xa5, 0x93,0xd9,0x8b,0x2b, 0x43,0x15,0x2c,0x07, 0x26,0x25,0xc7,0x08,
     0xf0,0xb3,0x4b,0x44, 0x96,0xfe,0xc7,0xc5, 0x64,0x27,0xaa,0x78, 0x5b,0xbc,0x40,0x51,
     0xce,0x89,0x6b,0xc1, 0x3f,0x9c,0xa0,0x5c, 0x75,0x98,0x24,0xc5, 0xe1,0x3e,0x86,0xdb},    /* PersonalizationString */
    48,
    {0x0e,0xe3,0x0f,0x07, 0x90,0xe2,0xde,0x20, 0xb6,0xf7,0x6f,0xef, 0x87,0xdc,0x7f,0xc4,
     0x0d,0x9d,0x05,0x31, 0x91,0x87,0x8c,0x9a, 0x19,0x53,0xd2,0xf8, 0x20,0x91,0xa0,0xef,
     0x97,0x59,0xea,0x12, 0x1b,0x2f,0x29,0x74, 0x76,0x35,0xf7,0x71, 0x5a,0x96,0xeb,0xbc},    /* AdditionalInput */
    48,
    {0x37,0x26,0x9a,0xa6, 0x28,0xe0,0x35,0x78, 0x12,0x42,0x44,0x5c, 0x55,0xbc,0xc8,0xb6,
     0x1f,0x24,0xf3,0x32, 0x88,0x02,0x69,0xa7, 0xed,0x1d,0xb7,0x4d, 0x8b,0x44,0x12,0x21,
     0x5e,0x60,0x53,0x96, 0x3b,0xb9,0x31,0x7f, 0x2a,0x87,0xbf,0x3c, 0x07,0xbb,0x27,0x22},    /* EntropyInputPR */
    48,
    {0xf1,0x24,0x35,0xa6, 0x8c,0x93,0x28,0x7e, 0x84,0xea,0x3d,0x27, 0x44,0x18,0xc9,0x13,
     0x73,0x49,0xb9,0x83, 0x79,0x15,0x29,0x53, 0x2f,0xef,0x43,0x06, 0xe7,0xcb,0x5c,0x0f,
     0x9f,0x10,0x4c,0x60, 0x7f,0xbf,0x0c,0x37, 0x9b,0xe4,0x94,0x26, 0xe5,0x3b,0xf5,0x63},    /* AdditionalInput */
    48,
    {0xdc,0x91,0x48,0x11, 0x63,0x7b,0x79,0x41, 0x36,0x8c,0x4f,0xe2, 0xc9,0x84,0x04,0x9c,
     0xdc,0x5b,0x6c,0x8d, 0x61,0x52,0xea,0xfa, 0x92,0x3b,0xb4,0x36, 0x4c,0x06,0x4a,0xd1,
     0xb1,0x8e,0x32,0x03, 0xfd,0xa4,0xf7,0x5a, 0xa6,0x5c,0x63,0xa1, 0xb9,0x96,0xfa,0x12},    /* EntropyInputPR */
    16,
    {0x1c,0xba,0xfd,0x48, 0x0f,0xf4,0x85,0x63, 0xd6,0x7d,0x91,0x14, 0xef,0x67,0x6b,0x7f}     /* ReturnedBits */
};


/*------------------------------------------------------------------*/

#define DO_ENCRYPT 1
#define DO_DECRYPT 0


/*------------------------------------------------------------------*/

static const BulkHashAlgo SHA1Suite =
{
    SHA1_RESULT_SIZE, SHA1_BLOCK_SIZE, SHA1_allocDigest, SHA1_freeDigest,
    (BulkCtxInitFunc)SHA1_initDigest, (BulkCtxUpdateFunc)SHA1_updateDigest, (BulkCtxFinalFunc)SHA1_finalDigest
};

#if (!defined(__DISABLE_MOCANA_SHA224__))
static const BulkHashAlgo SHA224Suite =
{
    SHA224_RESULT_SIZE, SHA224_BLOCK_SIZE, SHA224_allocDigest, SHA224_freeDigest,
    (BulkCtxInitFunc)SHA224_initDigest, (BulkCtxUpdateFunc)SHA224_updateDigest, (BulkCtxFinalFunc)SHA224_finalDigest
};
#endif

#if (!defined(__DISABLE_MOCANA_SHA256__))
static const BulkHashAlgo SHA256Suite =
{
    SHA256_RESULT_SIZE, SHA256_BLOCK_SIZE, SHA256_allocDigest, SHA256_freeDigest,
    (BulkCtxInitFunc)SHA256_initDigest, (BulkCtxUpdateFunc)SHA256_updateDigest, (BulkCtxFinalFunc)SHA256_finalDigest
};
#endif

#if (!defined(__DISABLE_MOCANA_SHA384__))
static const BulkHashAlgo SHA384Suite =
{
    SHA384_RESULT_SIZE, SHA384_BLOCK_SIZE, SHA384_allocDigest, SHA384_freeDigest,
    (BulkCtxInitFunc)SHA384_initDigest, (BulkCtxUpdateFunc)SHA384_updateDigest, (BulkCtxFinalFunc)SHA384_finalDigest
};
#endif

#if (!defined(__DISABLE_MOCANA_SHA512__))
static const BulkHashAlgo SHA512Suite =
{
    SHA512_RESULT_SIZE, SHA512_BLOCK_SIZE, SHA512_allocDigest, SHA512_freeDigest,
    (BulkCtxInitFunc)SHA512_initDigest, (BulkCtxUpdateFunc)SHA512_updateDigest, (BulkCtxFinalFunc)SHA512_finalDigest
};
#endif


/*------------------------------------------------------------------*/

static const BulkEncryptionAlgo AESECBSuite =
{
    AES_BLOCK_SIZE,
    (CreateBulkCtxFunc)CreateAESECBCtx, (DeleteBulkCtxFunc)DeleteAESECBCtx, (CipherFunc)DoAESECB
};

static const BulkEncryptionAlgo AESCBCSuite =
{
    AES_BLOCK_SIZE,
    (CreateBulkCtxFunc)CreateAESCtx, (DeleteBulkCtxFunc)DeleteAESCtx, (CipherFunc)DoAES
};

static const BulkEncryptionAlgo AESCFBSuite =
{
    AES_BLOCK_SIZE,
    (CreateBulkCtxFunc)CreateAESCFBCtx, (DeleteBulkCtxFunc)DeleteAESCtx, (CipherFunc)DoAES
};

static const BulkEncryptionAlgo AESOFBSuite =
{
    AES_BLOCK_SIZE,
    (CreateBulkCtxFunc)CreateAESOFBCtx, (DeleteBulkCtxFunc)DeleteAESCtx, (CipherFunc)DoAES
};

static const BulkEncryptionAlgo AESCTRSuite =
{
    AES_BLOCK_SIZE,
    (CreateBulkCtxFunc)CreateAESCTRCtx, (DeleteBulkCtxFunc)DeleteAESCTRCtx, (CipherFunc)DoAESCTR
};

static const BulkEncryptionAlgo TDESCBCSuite =
{
    THREE_DES_BLOCK_SIZE,
    (CreateBulkCtxFunc)Create3DESCtx, (DeleteBulkCtxFunc)Delete3DESCtx, (CipherFunc)Do3DES
};


/*------------------------------------------------------------------*/

/* Extern for Linux/Win32 Crypto Module FILE Read */
#ifdef __ENABLE_MOCANA_CRYPTO_KERNEL_MODULE_FIPS__
MOC_EXTERN sbyte4 MOC_CRYPTO_fipsSelfTestInit(ubyte* filename);
MOC_EXTERN sbyte4 MOC_CRYPTO_fipsSelfTestUpdate(sbyte4 fd, ubyte* buf, ubyte4 bufLen);
MOC_EXTERN sbyte4 MOC_CRYPTO_fipsSelfTestFinal(sbyte4 fd);
#endif

/*------------------------------------------------------------------*/

/*------------------------------------------------------------------*/


/*------------------------------------------------------------------*/

#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
#ifdef __ENABLE_MOCANA_FIPS_STATUS_TIMING_MESSAGES__
static volatile ubyte4 teststarttime;
#endif /* __ENABLE_MOCANA_FIPS_STATUS_TIMING_MESSAGES__ */
static void
FIPS_startTestMsg(const char *pFunctionName, const char *pTestName)
{
#ifdef __ENABLE_MOCANA_FIPS_STATUS_TIMING_MESSAGES__
	char timebuf[40];
#endif /* __ENABLE_MOCANA_FIPS_STATUS_TIMING_MESSAGES__ */
    DEBUG_PRINT(DEBUG_CRYPTO, (sbyte *) (sbyte*)pFunctionName);
    DEBUG_PRINT(DEBUG_CRYPTO, (sbyte *) ": Starting [");
    DEBUG_PRINT(DEBUG_CRYPTO, (sbyte *) (sbyte*)pTestName);
    DEBUG_PRINTNL(DEBUG_CRYPTO, (sbyte *) "] test.");
#ifdef __ENABLE_MOCANA_FIPS_STATUS_TIMING_MESSAGES__
    teststarttime = RTOS_getUpTimeInMS();
    sprintf(timebuf,"%d",teststarttime);
    DEBUG_PRINT(DEBUG_CRYPTO, (sbyte *) "S-Time= ");
    DEBUG_PRINTNL(DEBUG_CRYPTO, (sbyte *)timebuf);
#endif /* __ENABLE_MOCANA_FIPS_STATUS_TIMING_MESSAGES__ */
}
#endif


/*------------------------------------------------------------------*/

#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
#ifdef __ENABLE_MOCANA_FIPS_STATUS_TIMING_MESSAGES__
static volatile ubyte4 testendtime;
#endif /* __ENABLE_MOCANA_FIPS_STATUS_TIMING_MESSAGES__ */
static void
FIPS_endTestMsg(const char *pFunctionName, const char *pTestName, MSTATUS status)
{
#ifdef __ENABLE_MOCANA_FIPS_STATUS_TIMING_MESSAGES__
	char timebuf[40];
#endif /* __ENABLE_MOCANA_FIPS_STATUS_TIMING_MESSAGES__ */
    DEBUG_PRINT(DEBUG_CRYPTO, (sbyte *) (sbyte*)pFunctionName);
    DEBUG_PRINT(DEBUG_CRYPTO, (sbyte *) ": Result [");
    DEBUG_PRINT(DEBUG_CRYPTO, (sbyte *) (sbyte*)pTestName);

    if (OK > status)
        DEBUG_PRINTNL(DEBUG_CRYPTO, (sbyte *) "] FAILED.");
    else
        DEBUG_PRINTNL(DEBUG_CRYPTO, (sbyte *) "] PASSED.");
#ifdef __ENABLE_MOCANA_FIPS_STATUS_TIMING_MESSAGES__
    testendtime = RTOS_getUpTimeInMS();
    sprintf(timebuf,"%d",testendtime);
    DEBUG_PRINT(DEBUG_CRYPTO, (sbyte *) "E-Time = ");
    DEBUG_PRINT(DEBUG_CRYPTO, (sbyte *)timebuf);
    DEBUG_PRINT(DEBUG_CRYPTO, (sbyte *) "  Elapsed (mil) = ");
    sprintf(timebuf,"%d",testendtime-teststarttime);
    DEBUG_PRINTNL(DEBUG_CRYPTO, (sbyte *)timebuf);
#endif /* __ENABLE_MOCANA_FIPS_STATUS_TIMING_MESSAGES__ */
}
#endif


/*------------------------------------------------------------------*/

/* Algorithm Specific FIPS Tests */
static MSTATUS
FIPS_RANDOM186_DoKAT(ubyte b, const ubyte pXKey[/*b*/], sbyte4 seedLen, const ubyte pXSeed[/*seedLen*/], sbyte4 expectLen, ubyte* pExpect)
{
    randomContext*  pRandomContext = NULL;
    ubyte*          pResult = NULL;
    sbyte4          cmpRes = 0;
    MSTATUS         status = OK;

    if (NULL == (pResult = MALLOC(expectLen)))
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    if (OK > (status = RANDOM_newFIPS186Context(&pRandomContext, b, pXKey, seedLen, pXSeed)))
        goto exit;

    if (OK > (status = RANDOM_numberGeneratorFIPS186(pRandomContext, pResult, expectLen)))
        goto exit;
    if (STARTUP_TEST_SHOULDFAIL(FIPS_FORCE_FAIL_RNG_TESTNUM))
    {
        *pResult ^= 0x01;
    }

    if (OK != MOC_MEMCMP(pResult, pExpect, expectLen, &cmpRes))
    {
        status = ERR_FIPS_RNG_KAT;
        goto exit;
    }

    if (0 != cmpRes)
    {
        status = ERR_FIPS_RNG_KAT;
        goto exit;
    }

exit:
    RANDOM_deleteFIPS186Context(&pRandomContext);

    if (NULL != pResult)
        FREE(pResult);

	setFIPS_Status_Once(FIPS_ALGO_RNG_FIPS186, status);

    return status;
}


/*------------------------------------------------------------------*/

MOC_EXTERN MSTATUS
FIPS_random186Kat(void)
{
    MSTATUS status = OK;

    /* FIPS186_VSTGEN.rsp: [Xchange - SHA1], COUNT = 0 */
    ubyte b = 20;
    ubyte XKey[] = {
        0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00
    };
    sbyte4 seedLen = 20;
    ubyte XSeed[] = {
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00
    };
    sbyte4 expectLen = 20;
    ubyte expect[] = {
        0xda,0x39,0xa3,0xee,0x5e,0x6b,0x4b,0x0d,
        0x32,0x55,0xbf,0xef,0x95,0x60,0x18,0x90,
        0xaf,0xd8,0x07,0x09,0x92,0xd5,0xf2,0x01,
        0xdc,0x6e,0x0e,0x4e,0xff,0xf2,0xfa,0xba,
        0xd4,0x19,0xd4,0xbf,0x7e,0x41,0xfa,0x58
    };


#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
    FIPS_startTestMsg("FIPS_random186Kat", "RNG");
#endif

    if (OK > (status = FIPS_RANDOM186_DoKAT(b, XKey, seedLen, XSeed, expectLen, expect)))
        goto exit;

exit:
#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
    FIPS_endTestMsg("FIPS_random186Kat", "RNG", status);
#endif

    setFIPS_Status_Once(FIPS_ALGO_RNG, status);

    return status;
}

/*------------------------------------------------------------------*/

static MSTATUS
FIPS_NIST_CTRDRBG_DoKAT(sbyte4 useDf, const NIST_DRBG_TestVectorPR *pTest, sbyte4 keyLen)
{
    MSTATUS             status = OK;
    sbyte4              cmpRes;
    randomContext*      pCtx = NULL;
    ubyte               result[256] = { 0 };
    hwAccelDescr        hwAccelCtx = 0;

    /* create appropriate context */
    if(useDf)
    {
        status = NIST_CTRDRBG_newDFContext( MOC_HASH(hwAccelCtx)
                                    &pCtx, keyLen, pTest->resultLen,
                                    pTest->entropyInput, pTest->entropyInputLen,
                                    pTest->nonce, pTest->nonceLen,
                                    pTest->personalizationStr, pTest->personalizationStrLen);
    }
    else
    {
        status = NIST_CTRDRBG_newContext( MOC_HASH(hwAccelCtx)
                                    &pCtx,
                                    pTest->entropyInput, 32, pTest->resultLen,
                                    pTest->personalizationStr, pTest->personalizationStrLen);
    }

    if (OK > status)
        goto exit;

    /* predictionResistance */
    /* generate with PR = reseed + normal generate */

    /* reseed, generate and throw away first time */
    status = NIST_CTRDRBG_reseed( MOC_HASH(hwAccelCtx)
                pCtx, pTest->entropyInputPR1, pTest->entropyInputPR1Len,
                pTest->additionalInput1, pTest->additionalInput1Len);

    if (OK > status)
        goto exit;

    if (OK > (status = NIST_CTRDRBG_generate( MOC_HASH(hwAccelCtx)
                pCtx, NULL, 0, result, pTest->resultLen * 8)))
    {
        goto exit;
    }

    /* reseed, regenerate and print */
    status = NIST_CTRDRBG_reseed( MOC_HASH(hwAccelCtx)
                pCtx, pTest->entropyInputPR2, pTest->entropyInputPR2Len,
                pTest->additionalInput2, pTest->additionalInput2Len);

    if (OK > status)
        goto exit;

    if (OK > (status = NIST_CTRDRBG_generate( MOC_HASH(hwAccelCtx)
                pCtx, NULL, 0, result, pTest->resultLen * 8)))
    {
        goto exit;
    }

    if (STARTUP_TEST_SHOULDFAIL(FIPS_FORCE_FAIL_CTRRNG_TESTNUM))
    {
        *result ^= 0x01;
    }

    if (OK != MOC_MEMCMP(result, pTest->result, pTest->resultLen, &cmpRes))
    {
        status = ERR_FIPS_NISTRNG_KAT_FAILED;
        goto exit;
    }

    if (0 != cmpRes)
    {
        status = ERR_FIPS_NISTRNG_KAT_FAILED;
        goto exit;
    }

exit:
    NIST_CTRDRBG_deleteContext( MOC_HASH(hwAccelCtx) &pCtx);

    setFIPS_Status_Once(FIPS_ALGO_RNG_CTR, status);

    return status;
}


/*------------------------------------------------------------------*/

MOC_EXTERN MSTATUS
FIPS_nistRngKat(void)
{
    MSTATUS status = OK;

#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
    FIPS_startTestMsg("FIPS_nistRngKat", "NIST-RNG");
#endif

    if (ALGO_TESTSHOULDRUN(FIPS_ALGO_RNG_CTR))
    {
        if (OK > (status = FIPS_NIST_CTRDRBG_DoKAT(1, &kCTR_DRBG_AES256_DF_PR, 32)))
            goto exit;
    }

    if (ALGO_TESTSHOULDRUN(FIPS_ALGO_RNG_CTR))
    {
        if (OK > (status = FIPS_NIST_CTRDRBG_DoKAT(0, &kCTR_DRBG_AES256_NoDF_PR, 32)))
            goto exit;
    }

exit:
#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
    FIPS_endTestMsg("FIPS_nistRngKat", "NIST-RNG", status);
#endif

    setFIPS_Status_Once(FIPS_ALGO_RNG, status);

    return status;
}


#if !defined( __DISABLE_MOCANA_RSA_SIGN__) && !defined(__DISABLE_MOCANA_RSA_DECRYPTION__) && !defined(__ENABLE_MOCANA_PKCS11_CRYPTO__)

#define MAX_KEYBLOBLEN 2000  /* More than enough */

static ubyte test_PrivateKeyBlob[] = {
  0x02, 0x01, 0x00, 0x00, 0x00, 0x03, 0x01, 0x00, 0x01, 0x00, 0x00, 0x01,
  0x00, 0xac, 0x78, 0x6c, 0x64, 0xd9, 0x3e, 0x8f, 0xd4, 0xf4, 0xc1, 0x67,
  0x5c, 0xaa, 0x7a, 0x3f, 0x1c, 0x64, 0x0b, 0x63, 0xb2, 0x45, 0xbf, 0x0c,
  0x40, 0xb8, 0x99, 0xea, 0x05, 0x2f, 0xaf, 0xfb, 0xa0, 0xbc, 0x33, 0xf9,
  0xa4, 0x8f, 0x8c, 0xcb, 0x6f, 0xd3, 0x53, 0xf8, 0x39, 0x09, 0xf3, 0xd2,
  0x40, 0x8f, 0xab, 0x74, 0x14, 0x92, 0x0d, 0x4b, 0xe2, 0x60, 0xaf, 0xfd,
  0x8d, 0xc2, 0x97, 0x59, 0x20, 0xfc, 0xb6, 0x25, 0x2c, 0x6f, 0x05, 0xa8,
  0x24, 0x80, 0xe8, 0xa1, 0xa6, 0x14, 0x1e, 0xed, 0xb9, 0x0e, 0xbd, 0x73,
  0xe5, 0xd8, 0x0b, 0x1c, 0x67, 0xdf, 0x3b, 0x2f, 0x40, 0x3b, 0xb2, 0xd7,
  0xa0, 0x24, 0x73, 0x5f, 0x80, 0x47, 0x42, 0x57, 0x8a, 0x72, 0x17, 0xe8,
  0x98, 0xaa, 0xfe, 0xe8, 0xe1, 0x98, 0x8f, 0x5b, 0x06, 0x9d, 0xe7, 0x23,
  0xc9, 0x09, 0x5a, 0x0d, 0x53, 0x8e, 0xf5, 0x19, 0x13, 0x9a, 0x8b, 0x4b,
  0x15, 0x93, 0xd8, 0x27, 0xa5, 0xcf, 0xa0, 0xc0, 0xdd, 0x1f, 0x55, 0x6d,
  0x4c, 0xf8, 0x93, 0xd9, 0x4a, 0xd2, 0x42, 0x18, 0x98, 0xa2, 0x17, 0x73,
  0x9b, 0x6f, 0x8f, 0xa4, 0x09, 0x24, 0x89, 0x6f, 0xef, 0x84, 0x96, 0xeb,
  0x37, 0x7c, 0xbf, 0x0d, 0xa9, 0x0f, 0x5e, 0x11, 0xe4, 0x6c, 0xfb, 0x7b,
  0x43, 0x82, 0x58, 0x94, 0xe2, 0x90, 0x31, 0x21, 0xb4, 0xfb, 0xba, 0x4a,
  0x9b, 0x3a, 0xa9, 0x75, 0xe0, 0x1f, 0xbd, 0xb2, 0x72, 0xae, 0xd1, 0x0e,
  0xe7, 0xe9, 0x26, 0x07, 0xcd, 0x18, 0x34, 0x9f, 0x4d, 0x27, 0x08, 0x39,
  0x25, 0xf4, 0xf1, 0xb9, 0x02, 0xe6, 0x43, 0x37, 0x75, 0x22, 0x16, 0x5f,
  0x31, 0x63, 0xab, 0xca, 0x71, 0xaa, 0x57, 0x50, 0x39, 0xec, 0x4a, 0xa0,
  0x29, 0x10, 0x21, 0x83, 0xcc, 0x9e, 0xe8, 0x72, 0xfd, 0x03, 0x6a, 0xe8,
  0x7c, 0x9d, 0x40, 0x21, 0x9d, 0x00, 0x00, 0x00, 0x80, 0xf8, 0x0f, 0x3f,
  0x62, 0x4f, 0xd4, 0xda, 0xdd, 0xee, 0x4a, 0xc2, 0x60, 0x31, 0xe1, 0x2f,
  0xbb, 0xa6, 0xaf, 0xca, 0xca, 0xfa, 0xf7, 0xb4, 0x01, 0xbc, 0x0d, 0xfb,
  0xb0, 0x74, 0xb3, 0x43, 0x3c, 0x2e, 0xc0, 0x53, 0x23, 0xfe, 0x57, 0x63,
  0xca, 0xba, 0x82, 0x7d, 0xe5, 0xef, 0x38, 0xe3, 0xbf, 0x0e, 0xd5, 0x28,
  0xd6, 0x61, 0x4d, 0x37, 0x24, 0x7e, 0x33, 0x6b, 0xec, 0xb8, 0xda, 0x80,
  0xe2, 0x25, 0x24, 0x84, 0xca, 0xe3, 0xd1, 0x55, 0xa9, 0x7c, 0x4a, 0xfc,
  0x99, 0xcc, 0x91, 0x43, 0x52, 0x19, 0xdc, 0xd9, 0x17, 0xd4, 0x28, 0x11,
  0x65, 0x97, 0xa3, 0x30, 0x0c, 0x15, 0x50, 0x14, 0x60, 0x5a, 0x3f, 0x23,
  0x9d, 0xe2, 0xa0, 0xaf, 0x73, 0x7f, 0x8e, 0x7a, 0x7f, 0x3a, 0x9d, 0x4f,
  0xa9, 0x21, 0x76, 0x71, 0x0d, 0x7f, 0xfc, 0x1d, 0x15, 0x9b, 0xd8, 0x59,
  0x74, 0xb7, 0x8e, 0xd6, 0x11, 0x00, 0x00, 0x00, 0x80, 0xb1, 0xfd, 0xc0,
  0x78, 0x89, 0x77, 0x99, 0xf4, 0x8b, 0x39, 0x7c, 0xe9, 0xda, 0xb1, 0xd9,
  0x2e, 0x8c, 0x8c, 0x05, 0xc0, 0x59, 0x61, 0xb7, 0x8b, 0x03, 0xb6, 0x1f,
  0xa0, 0x53, 0xb8, 0x2c, 0xd1, 0x2a, 0x6e, 0x33, 0x40, 0x45, 0x52, 0xdb,
  0x0e, 0x1d, 0xea, 0xd5, 0xe2, 0x4f, 0x59, 0x5e, 0x69, 0x0c, 0xb6, 0xa9,
  0xfb, 0x5a, 0x6d, 0xb5, 0xe3, 0x81, 0x88, 0x7f, 0x7d, 0x08, 0xde, 0x7f,
  0x2d, 0xfa, 0xcd, 0x8b, 0x09, 0x12, 0x69, 0x19, 0xbd, 0xe7, 0x53, 0x1b,
  0x6a, 0x58, 0x35, 0x33, 0x20, 0x74, 0xa7, 0xc0, 0xc7, 0xb1, 0x92, 0x56,
  0x1e, 0xf3, 0x7c, 0x52, 0xa4, 0xf9, 0x6e, 0x8d, 0xdc, 0x5d, 0x1a, 0x51,
  0x1e, 0x2c, 0xba, 0x37, 0x34, 0xc8, 0x45, 0xe7, 0xce, 0x71, 0xc2, 0x54,
  0x34, 0x38, 0xbd, 0xdf, 0x99, 0x65, 0x5d, 0xe3, 0x00, 0x4d, 0xfe, 0x31,
  0xd9, 0xfb, 0xa5, 0x56, 0xcd, 0x00, 0x00, 0x00, 0x80, 0xf4, 0x1b, 0xa5,
  0x27, 0x6d, 0x22, 0x2d, 0x84, 0x0a, 0x94, 0xdd, 0x35, 0x66, 0xc0, 0x90,
  0x85, 0x9c, 0xa2, 0x0f, 0xf1, 0xb2, 0x09, 0x82, 0xc5, 0xd6, 0x36, 0xf8,
  0x81, 0x0c, 0x46, 0xc0, 0x9a, 0x7f, 0xf3, 0x59, 0x9d, 0xe9, 0x14, 0x3c,
  0xaa, 0xea, 0xe1, 0xb1, 0x5d, 0x4e, 0x0d, 0xf0, 0xe9, 0x3a, 0x82, 0x7f,
  0xde, 0x80, 0x00, 0x49, 0x8c, 0x8a, 0xf8, 0xb5, 0x73, 0x4d, 0xf2, 0x10,
  0xb4, 0xfb, 0x12, 0x35, 0xef, 0xa7, 0x43, 0x80, 0x85, 0xfa, 0x3f, 0x9c,
  0xd7, 0x09, 0x1d, 0xc6, 0x5f, 0x0b, 0xfe, 0x6e, 0x50, 0xe9, 0xc1, 0xc8,
  0x64, 0xee, 0x55, 0x73, 0xd9, 0xe0, 0x3b, 0x5e, 0xe1, 0xf6, 0xcd, 0x7d,
  0x92, 0x48, 0xcc, 0x11, 0xfc, 0x9a, 0x01, 0x2f, 0x00, 0xf7, 0x40, 0x89,
  0x7d, 0x09, 0xe6, 0x11, 0x98, 0xd4, 0x62, 0xd8, 0x88, 0x44, 0x46, 0x22,
  0xba, 0x1e, 0x4c, 0xdc, 0xd1, 0x00, 0x00, 0x00, 0x80, 0x71, 0xb6, 0xda,
  0x96, 0xa7, 0xcc, 0xbf, 0x91, 0x5a, 0xb9, 0x79, 0xb2, 0xb6, 0x43, 0xd5,
  0xab, 0x45, 0xa3, 0xd7, 0xb0, 0xd1, 0xe9, 0xfa, 0x27, 0x58, 0x51, 0xac,
  0xd6, 0xf3, 0x65, 0xc1, 0x4c, 0x48, 0xbd, 0x6b, 0x04, 0xee, 0xc5, 0x46,
  0xaa, 0x38, 0x36, 0xe6, 0x3a, 0xd5, 0xd3, 0x14, 0xdc, 0x2c, 0x81, 0x2f,
  0x0c, 0x24, 0xf3, 0xde, 0xb6, 0xe0, 0xf4, 0xe1, 0xee, 0x72, 0x12, 0x24,
  0x52, 0xad, 0xdf, 0x4f, 0xaa, 0x96, 0x16, 0x8b, 0x99, 0xa6, 0x06, 0x94,
  0x87, 0x56, 0x9f, 0x76, 0x70, 0x8f, 0xd6, 0xf4, 0xf5, 0x1f, 0xdf, 0x8c,
  0x21, 0xee, 0x11, 0x49, 0x83, 0x98, 0xd0, 0x26, 0xd5, 0xd8, 0xad, 0x8d,
  0x91, 0xa7, 0xa5, 0xb8, 0xcb, 0x82, 0x00, 0x17, 0x5e, 0xef, 0x92, 0xe5,
  0xd5, 0x0f, 0x43, 0x4f, 0x6d, 0x63, 0x33, 0x9e, 0x69, 0x7d, 0x6a, 0x9f,
  0x52, 0xd2, 0xd1, 0x09, 0x29, 0x00, 0x00, 0x00, 0x80, 0x18, 0x85, 0x69,
  0x3e, 0x83, 0x19, 0x40, 0x97, 0xb9, 0xec, 0xce, 0x6f, 0x26, 0x44, 0x07,
  0x48, 0x5f, 0x76, 0xbe, 0x94, 0xa2, 0xd5, 0xb2, 0xf4, 0xb1, 0xcd, 0x59,
  0xf3, 0xce, 0xdc, 0x59, 0xa9, 0xe8, 0xe3, 0x1e, 0xc6, 0xbf, 0xb0, 0x66,
  0x97, 0x40, 0xef, 0x36, 0xdf, 0x28, 0x63, 0xfa, 0xf3, 0x0f, 0x8a, 0xad,
  0x17, 0xf3, 0xf3, 0x44, 0xf7, 0x31, 0x29, 0x48, 0xdd, 0xe3, 0x59, 0xbd,
  0xa9, 0x5c, 0x3a, 0x7d, 0xf6, 0x97, 0x63, 0x4f, 0xcf, 0x13, 0xa1, 0x9f,
  0x9a, 0xe2, 0x0f, 0x89, 0xff, 0x2d, 0x4a, 0xff, 0xf7, 0x6c, 0x04, 0x22,
  0x6c, 0x6e, 0xf1, 0xd1, 0xa3, 0x18, 0xde, 0x34, 0xe0, 0x28, 0x1b, 0xc3,
  0xa3, 0xf4, 0x45, 0xf8, 0xdd, 0xe5, 0x36, 0xa0, 0xe3, 0x92, 0x47, 0x9b,
  0xf9, 0xe0, 0x12, 0x43, 0x32, 0x62, 0xde, 0xa6, 0xbd, 0x31, 0xa3, 0x64,
  0x79, 0xf0, 0xae, 0x8a, 0xfd

};

static ubyte test_PublicKeyBlob[] = {
  0x02, 0x00, 0x00, 0x00, 0x00, 0x03, 0x01, 0x00, 0x01, 0x00, 0x00, 0x01,
  0x00, 0xac, 0x78, 0x6c, 0x64, 0xd9, 0x3e, 0x8f, 0xd4, 0xf4, 0xc1, 0x67,
  0x5c, 0xaa, 0x7a, 0x3f, 0x1c, 0x64, 0x0b, 0x63, 0xb2, 0x45, 0xbf, 0x0c,
  0x40, 0xb8, 0x99, 0xea, 0x05, 0x2f, 0xaf, 0xfb, 0xa0, 0xbc, 0x33, 0xf9,
  0xa4, 0x8f, 0x8c, 0xcb, 0x6f, 0xd3, 0x53, 0xf8, 0x39, 0x09, 0xf3, 0xd2,
  0x40, 0x8f, 0xab, 0x74, 0x14, 0x92, 0x0d, 0x4b, 0xe2, 0x60, 0xaf, 0xfd,
  0x8d, 0xc2, 0x97, 0x59, 0x20, 0xfc, 0xb6, 0x25, 0x2c, 0x6f, 0x05, 0xa8,
  0x24, 0x80, 0xe8, 0xa1, 0xa6, 0x14, 0x1e, 0xed, 0xb9, 0x0e, 0xbd, 0x73,
  0xe5, 0xd8, 0x0b, 0x1c, 0x67, 0xdf, 0x3b, 0x2f, 0x40, 0x3b, 0xb2, 0xd7,
  0xa0, 0x24, 0x73, 0x5f, 0x80, 0x47, 0x42, 0x57, 0x8a, 0x72, 0x17, 0xe8,
  0x98, 0xaa, 0xfe, 0xe8, 0xe1, 0x98, 0x8f, 0x5b, 0x06, 0x9d, 0xe7, 0x23,
  0xc9, 0x09, 0x5a, 0x0d, 0x53, 0x8e, 0xf5, 0x19, 0x13, 0x9a, 0x8b, 0x4b,
  0x15, 0x93, 0xd8, 0x27, 0xa5, 0xcf, 0xa0, 0xc0, 0xdd, 0x1f, 0x55, 0x6d,
  0x4c, 0xf8, 0x93, 0xd9, 0x4a, 0xd2, 0x42, 0x18, 0x98, 0xa2, 0x17, 0x73,
  0x9b, 0x6f, 0x8f, 0xa4, 0x09, 0x24, 0x89, 0x6f, 0xef, 0x84, 0x96, 0xeb,
  0x37, 0x7c, 0xbf, 0x0d, 0xa9, 0x0f, 0x5e, 0x11, 0xe4, 0x6c, 0xfb, 0x7b,
  0x43, 0x82, 0x58, 0x94, 0xe2, 0x90, 0x31, 0x21, 0xb4, 0xfb, 0xba, 0x4a,
  0x9b, 0x3a, 0xa9, 0x75, 0xe0, 0x1f, 0xbd, 0xb2, 0x72, 0xae, 0xd1, 0x0e,
  0xe7, 0xe9, 0x26, 0x07, 0xcd, 0x18, 0x34, 0x9f, 0x4d, 0x27, 0x08, 0x39,
  0x25, 0xf4, 0xf1, 0xb9, 0x02, 0xe6, 0x43, 0x37, 0x75, 0x22, 0x16, 0x5f,
  0x31, 0x63, 0xab, 0xca, 0x71, 0xaa, 0x57, 0x50, 0x39, 0xec, 0x4a, 0xa0,
  0x29, 0x10, 0x21, 0x83, 0xcc, 0x9e, 0xe8, 0x72, 0xfd, 0x03, 0x6a, 0xe8,
  0x7c, 0x9d, 0x40, 0x21, 0x9d

};


static ubyte expect[] = {
  0x61, 0xba, 0xfd, 0xaa, 0xf4, 0x14, 0x63, 0xf7, 0xc2, 0x4a, 0x31, 0x13,
  0xa4, 0x94, 0x4f, 0x96, 0x78, 0x0f, 0xb0, 0x0d, 0x64, 0xdd, 0x56, 0xf7,
  0xc5, 0x9d, 0x78, 0x39, 0x14, 0x02, 0xe5, 0x5a, 0xb9, 0x88, 0x56, 0x85,
  0x9d, 0xf7, 0xb3, 0x0f, 0x51, 0x30, 0xde, 0xa9, 0x74, 0xe3, 0xa3, 0xf7,
  0x5b, 0x84, 0xa2, 0x7b, 0x37, 0x9d, 0x5d, 0x97, 0x75, 0x5b, 0xad, 0xf3,
  0xb5, 0x4a, 0x58, 0x0d, 0x99, 0x23, 0x4d, 0xbc, 0x5c, 0x0b, 0x54, 0xb6,
  0x33, 0x97, 0x97, 0xd1, 0x1a, 0xab, 0x7b, 0x4a, 0x4b, 0xa2, 0x16, 0x1a,
  0x82, 0x70, 0x84, 0xed, 0x5e, 0xfe, 0x60, 0x60, 0x49, 0xb6, 0xa7, 0xeb,
  0x9e, 0x84, 0xa1, 0xe9, 0xe3, 0xe8, 0x89, 0x72, 0xe7, 0x59, 0x2f, 0x4a,
  0xe5, 0x98, 0x53, 0xc9, 0x01, 0xcc, 0xea, 0x35, 0xa1, 0xcb, 0xe3, 0x02,
  0xa1, 0x13, 0xd3, 0xdb, 0x3d, 0xad, 0x68, 0x63, 0x5b, 0x81, 0x7d, 0x9a,
  0xca, 0x1b, 0xc5, 0x79, 0x79, 0x53, 0xfe, 0x03, 0xc4, 0x48, 0x3d, 0x35,
  0x8c, 0xae, 0x2d, 0x0d, 0x22, 0x5d, 0xc7, 0x8e, 0xf0, 0x5b, 0xdf, 0xc2,
  0x89, 0x05, 0xbf, 0x58, 0x7d, 0xf5, 0x43, 0x7a, 0x0f, 0xc3, 0x3e, 0xfc,
  0x56, 0xa4, 0xfe, 0xa4, 0x18, 0x58, 0x53, 0x85, 0x91, 0x7f, 0xee, 0x43,
  0x61, 0x45, 0x07, 0xf2, 0x38, 0x38, 0xb7, 0xdd, 0x1f, 0xab, 0xb4, 0x54,
  0x72, 0xd2, 0x10, 0x55, 0xde, 0x26, 0x30, 0x7a, 0x05, 0xcf, 0x0a, 0x20,
  0xb7, 0xf8, 0xbc, 0xa7, 0xd6, 0x42, 0xcb, 0x54, 0x61, 0x8e, 0x05, 0xe8,
  0x95, 0x52, 0x33, 0x45, 0xe0, 0x34, 0x0c, 0x6d, 0xf6, 0x97, 0x57, 0xa3,
  0xfa, 0xf7, 0x15, 0x4c, 0x49, 0xe9, 0xd8, 0xf1, 0xfb, 0xa9, 0xc0, 0xe9,
  0x29, 0x0d, 0x3b, 0x3f, 0x64, 0x2a, 0xa9, 0x75, 0x2a, 0xe8, 0xc3, 0x45,
  0x10, 0xf0, 0x50, 0x03
};


static MSTATUS
FIPS_rsaKat(hwAccelDescr hwAccelCtx)
{

    sbyte           testmsg[] = "We attack at dawn and plan to succeed";
    ubyte*          pCipherText = NULL;
    ubyte*          pPlainText = NULL;
    RSAKey*         pRSAKey = NULL;
    RSAKey* 		pRSAPublicKey = NULL;
    vlong*          pQueue  = NULL;
    sbyte4          cipherTextLen = 0;
    ubyte4          plainTextLen = 0;
    MSTATUS         status = OK;
    ubyte*          pKeyBlob = NULL;
    sbyte4          resCmp;

#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
    FIPS_startTestMsg("FIPS_rsaKat", "RSA");
#endif

    /* Create the memory to hold RSA key */
    if (OK > (status = RSA_createKey(&pRSAKey)))
    {
    	DEBUG_PRINTNL(DEBUG_CRYPTO, (sbyte *) "\nAPI - RSA_createKey - Failed");
        status = ERR_FIPS_RSA_KAT_FAILED;
        goto exit;
    }

    /* Make a RSA Key from the test_PrivateKeyBlob */
    if (NULL == (pKeyBlob = MALLOC(MAX_KEYBLOBLEN)))
    {
        status = ERR_CRYPTO;
        goto exit;
    }
    MOC_MEMSET(pKeyBlob, 0, MAX_KEYBLOBLEN);
    MOC_MEMCPY(pKeyBlob, test_PrivateKeyBlob, sizeof(test_PrivateKeyBlob));

    /* Make key from Key Blob */
    if (OK > (status = RSA_keyFromByteString(MOC_RSA(hwAccelCtx) &pRSAKey, pKeyBlob, sizeof(test_PrivateKeyBlob), NULL)))
    {
    	DEBUG_PRINTNL(DEBUG_CRYPTO, (sbyte *) "\nAPI - RSA_keyFromByteString - Failed");
        goto exit;
    }

    /* Get the cipher text length */
    if (OK > (status = RSA_getCipherTextLength(pRSAKey, &cipherTextLen)))
    {
    	DEBUG_PRINTNL(DEBUG_CRYPTO, (sbyte *) "\nAPI - RSA_getCipherTextLength - Failed");
        status = ERR_FIPS_RSA_KAT_FAILED;
        goto exit;
    }

    /********** Signature Calculation and Verification ****************/

    /* Allocate memory for Cipher Text */
    pCipherText = MALLOC(cipherTextLen);
    if (NULL == pCipherText)
    {
        status = ERR_FIPS_RSA_KAT_FAILED;
        goto exit;
    }

    /* Allocate memory for Plain Text */
    pPlainText = MALLOC(cipherTextLen+1);
    if (NULL == pPlainText)
    {
        status = ERR_FIPS_RSA_KAT_FAILED;
        goto exit;
    }

    /* Clear all memory */
    MOC_MEMSET(pCipherText, 0x00, cipherTextLen);
    MOC_MEMSET(pPlainText, 0x00, cipherTextLen+1);
    plainTextLen = 0;


    if (STARTUP_TEST_SHOULDFAIL(FIPS_FORCE_FAIL_RSA_TESTNUM))
    {
        testmsg[0] ^= 0x01;
    }

    /* RSA signature/verification API aren't taking care of message digest before calculating
     * the signature, we need to do it from our own
     */
    /* Calculate the signature */
    if (OK > (status = RSA_signMessage(MOC_RSA(hwAccelCtx) pRSAKey,
                                       (const ubyte*) testmsg,
                                       (sbyte4)MOC_STRLEN(testmsg)+1,
                                       pCipherText, &pQueue)))
    {
    	DEBUG_PRINTNL(DEBUG_CRYPTO, (sbyte *) "\nAPI - RSA_signMessage - Failed");
    	status = ERR_FIPS_RSA_KAT_FAILED;
        goto exit;
    }

    /* Verify the Cipher Text with the expected data */
    MOC_MEMCMP( pCipherText, expect, cipherTextLen, &resCmp);
    if (0 != resCmp)
    {
    	DEBUG_PRINTNL(DEBUG_CRYPTO, (sbyte *) "\nAPI - RSA_expectCompare - Failed");
    	status = ERR_FIPS_RSA_KAT_FAILED;
    	goto exit;
    }

    /* Make a Public RSA Key from the test_PublicKeyBlob */
    MOC_MEMSET(pKeyBlob, 0, MAX_KEYBLOBLEN);
    MOC_MEMCPY(pKeyBlob, test_PublicKeyBlob, sizeof(test_PublicKeyBlob));

    /* Make key from Key Blob */
    if (OK > (status = RSA_keyFromByteString(MOC_RSA(hwAccelCtx) &pRSAPublicKey, pKeyBlob, sizeof(test_PublicKeyBlob), NULL)))
    {
    	DEBUG_PRINTNL(DEBUG_CRYPTO, (sbyte *) "\nAPI - RSA_keyFromByteString - Failed");
        status = ERR_CRYPTO;
        goto exit;
    }

    /* Verify the signature, it doesn't memory compare the output, we need to do that from our own */
    if (OK > (status = RSA_verifySignature(MOC_RSA(hwAccelCtx) pRSAPublicKey,
                                           pCipherText, pPlainText,
                                           &plainTextLen, &pQueue)))
    {
    	DEBUG_PRINTNL(DEBUG_CRYPTO, (sbyte *) "\nAPI - RSA_verifySigature - Failed");
        status = ERR_FIPS_RSA_KAT_FAILED;
        goto exit;
    }

    if (0 != MOC_STRCMP(testmsg, (const sbyte*) pPlainText))
    {
    	DEBUG_PRINTNL(DEBUG_CRYPTO, (sbyte *) "\nRSA sign verify failed");
        status = ERR_CRYPTO;
        goto exit;
    }

exit:
	/* Release all resources */
	FREE(pKeyBlob);
	RSA_freeKey(&pRSAKey, &pQueue);
	RSA_freeKey(&pRSAPublicKey, &pQueue);
	VLONG_freeVlongQueue(&pQueue);
	FREE(pPlainText);
	FREE(pCipherText);

#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
    FIPS_endTestMsg("FIPS_rsaKat", "RSA", status);
#endif

    setFIPS_Status_Once(FIPS_ALGO_RSA, status);

    return status;

} /* FIPS_rsaKat */

#endif


/*------------------------------------------------------------------*/

static MSTATUS
FIPS_doKatHash(hwAccelDescr hwAccelCtx, const BulkHashAlgo *pHashSuite,
               ubyte *pData, ubyte4 dataLen, ubyte *pExpect,
               intBoolean isForceFail)
{
    ubyte*      pResult = NULL;
    BulkCtx     pCtx     = NULL;
    sbyte4      cmpRes = 0;
    MSTATUS     status = OK;

    if (NULL == pHashSuite)
    {
        status = ERR_FIPS_HASH_KAT_NULL;
        goto exit;
    }

    if (OK > (status = CRYPTO_ALLOC(hwAccelCtx, pHashSuite->digestSize, TRUE, &pResult)))
        goto exit;

    if (OK > (status = pHashSuite->allocFunc(MOC_HASH(hwAccelCtx) &pCtx)))
        goto exit;

    if (OK > (status = pHashSuite->initFunc(MOC_HASH(hwAccelCtx) pCtx)))
        goto exit;

    if (OK > (status = pHashSuite->updateFunc(MOC_HASH(hwAccelCtx) pCtx, pData, dataLen)))
        goto exit;

    if (OK > (status = pHashSuite->finalFunc(MOC_HASH(hwAccelCtx) pCtx, pResult)))
        goto exit;

    if (TRUE == isForceFail)
        *pResult ^= 0x80;

    if (OK != MOC_MEMCMP(pResult, pExpect, pHashSuite->digestSize, &cmpRes))
    {
        status = ERR_FIPS_HASH_KAT_FAILED;
        goto exit;
    }

    if (0 != cmpRes)
    {
        status = ERR_FIPS_HASH_KAT_FAILED;
        goto exit;
    }

exit:
    pHashSuite->freeFunc(MOC_HASH(hwAccelCtx) &pCtx);
    CRYPTO_FREE(hwAccelCtx, TRUE, &pResult);

    return status;
}


/*------------------------------------------------------------------*/

static MSTATUS
FIPS_createHashCtx(hwAccelDescr hwAccelCtx, ubyte **ppRetHashData, ubyte4 *pRetHashDataLen)
{
    MSTATUS status;

    *pRetHashDataLen = 32;

    if (OK > (status = CRYPTO_ALLOC(hwAccelCtx, *pRetHashDataLen, TRUE, ppRetHashData)))
        goto exit;

    status = MOC_MEMSET(*ppRetHashData, 0x00, *pRetHashDataLen);

exit:
    return status;
}


/*------------------------------------------------------------------*/

static void
FIPS_deleteHashCtx(hwAccelDescr hwAccelCtx, ubyte **ppFreeHashData)
{
    CRYPTO_FREE(hwAccelCtx, TRUE, ppFreeHashData);
}


/*------------------------------------------------------------------*/

MOC_EXTERN MSTATUS
FIPS_hashKat(hwAccelDescr hwAccelCtx,
             const char *pTestName,
             const BulkHashAlgo *pHashSuite,
             ubyte *pExpect, ubyte4 expectLen,
             intBoolean isForceFail)
{
    ubyte*              pData = NULL;
    ubyte4              dataLen;
    MSTATUS             status = OK;

#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
    FIPS_startTestMsg("FIPS_hashKat", pTestName);
#endif

    if (expectLen != pHashSuite->digestSize)
    {
        status = ERR_FIPS_HASH_KAT_LEN_FAILED;
        goto exit;
    }

    if (OK > (status = FIPS_createHashCtx(hwAccelCtx, &pData, &dataLen)))
        goto exit;

    if (OK > (status = FIPS_doKatHash(hwAccelCtx, pHashSuite, pData, dataLen, pExpect, isForceFail)))
        goto exit;

exit:
#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
    FIPS_endTestMsg("FIPS_hashKat", pTestName, status);
#endif

    FIPS_deleteHashCtx(hwAccelCtx, &pData);

    return status;

} /* FIPS_hashKat */


/*------------------------------------------------------------------*/

MOC_EXTERN MSTATUS
FIPS_sha1Kat(hwAccelDescr hwAccelCtx)
{
    ubyte expect[] = {
        0xde,0x8a,0x84,0x7b,0xff,0x8c,0x34,0x3d,
        0x69,0xb8,0x53,0xa2,0x15,0xe6,0xee,0x77,
        0x5e,0xf2,0xef,0x96
    };

    MSTATUS status = FIPS_hashKat(hwAccelCtx, "SHA-1", &SHA1Suite, expect, sizeof(expect), FIPS_FORCE_FAIL_SHA1_TEST);
    setFIPS_Status_Once(FIPS_ALGO_SHA1, status);
    return status;

} /* FIPS_sha1Kat */


/*------------------------------------------------------------------*/

MOC_EXTERN MSTATUS
FIPS_sha224Kat(hwAccelDescr hwAccelCtx)
{
    ubyte expect[] = {
        0xb3,0x38,0xc7,0x6b,0xcf,0xfa,0x1a,0x0b,
        0x3e,0xad,0x8d,0xe5,0x8d,0xfb,0xff,0x47,
        0xb6,0x3a,0xb1,0x15,0x0e,0x10,0xd8,0xf1,
        0x7f,0x2b,0xaf,0xdf
    };

    MSTATUS status = FIPS_hashKat(hwAccelCtx, "SHA-224", &SHA224Suite, expect, sizeof(expect), FIPS_FORCE_FAIL_SHA224_TEST);
    setFIPS_Status_Once(FIPS_ALGO_SHA256, status);
    return status;

} /* FIPS_sha224Kat */


/*------------------------------------------------------------------*/

MOC_EXTERN MSTATUS
FIPS_sha256Kat(hwAccelDescr hwAccelCtx)
{
    ubyte expect[] = {
        0x66,0x68,0x7a,0xad,0xf8,0x62,0xbd,0x77,
        0x6c,0x8f,0xc1,0x8b,0x8e,0x9f,0x8e,0x20,
        0x08,0x97,0x14,0x85,0x6e,0xe2,0x33,0xb3,
        0x90,0x2a,0x59,0x1d,0x0d,0x5f,0x29,0x25
    };

    MSTATUS status = FIPS_hashKat(hwAccelCtx, "SHA-256", &SHA256Suite, expect, sizeof(expect), FIPS_FORCE_FAIL_SHA256_TEST);
    setFIPS_Status_Once(FIPS_ALGO_SHA256, status);
    return status;

} /* FIPS_sha256Kat */


/*------------------------------------------------------------------*/

MOC_EXTERN MSTATUS
FIPS_sha384Kat(hwAccelDescr hwAccelCtx)
{
    ubyte expect[] = {
        0xa3,0x8f,0xff,0x4b,0xa2,0x6c,0x15,0xe4,
        0xac,0x9c,0xde,0x8c,0x03,0x10,0x3a,0xc8,
        0x90,0x80,0xfd,0x47,0x54,0x5f,0xde,0x94,
        0x46,0xc8,0xf1,0x92,0x72,0x9e,0xab,0x7b,
        0xd0,0x3a,0x4d,0x5c,0x31,0x87,0xf7,0x5f,
        0xe2,0xa7,0x1b,0x0e,0xe5,0x0a,0x4a,0x40
    };

    MSTATUS status = FIPS_hashKat(hwAccelCtx, "SHA-384", &SHA384Suite, expect, sizeof(expect), FIPS_FORCE_FAIL_SHA384_TEST);
    setFIPS_Status_Once(FIPS_ALGO_SHA512, status);
    return status;

} /* FIPS_sha384Kat */


/*------------------------------------------------------------------*/

MOC_EXTERN MSTATUS
FIPS_sha512Kat(hwAccelDescr hwAccelCtx)
{
    ubyte expect[] = {
        0x50,0x46,0xad,0xc1,0xdb,0xa8,0x38,0x86,
        0x7b,0x2b,0xbb,0xfd,0xd0,0xc3,0x42,0x3e,
        0x58,0xb5,0x79,0x70,0xb5,0x26,0x7a,0x90,
        0xf5,0x79,0x60,0x92,0x4a,0x87,0xf1,0x96,
        0x0a,0x6a,0x85,0xea,0xa6,0x42,0xda,0xc8,
        0x35,0x42,0x4b,0x5d,0x7c,0x8d,0x63,0x7c,
        0x00,0x40,0x8c,0x7a,0x73,0xda,0x67,0x2b,
        0x7f,0x49,0x85,0x21,0x42,0x0b,0x6d,0xd3
    };

    MSTATUS status = FIPS_hashKat(hwAccelCtx, "SHA-512", &SHA512Suite, expect, sizeof(expect), FIPS_FORCE_FAIL_SHA512_TEST);
    setFIPS_Status_Once(FIPS_ALGO_SHA512, status);
    return status;

} /* FIPS_sha512Kat */


/*------------------------------------------------------------------*/

MOC_EXTERN MSTATUS
FIPS_aesCmacKat(hwAccelDescr hwAccelCtx)
{
    AESCMAC_Ctx ctx;
    ubyte       cmacOutput[CMAC_RESULT_SIZE];
    ubyte*       pTestMessage2 = (ubyte*) "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
    ubyte       key[16] = { 0x11,0x01,0x01,0x01, 0x01,0x01,0x01,0x31, 0x01,0x01,0x01,0x01, 0x01,0x01,0x01,0x01 };
#ifdef __MOCANA_ENABLE_FIPS_FAIL_AES_CMAC_TEST__
    ubyte       expected[CMAC_RESULT_SIZE] = { 0x85,0x58,0x16,0xad, 0xaa,0x5c,0x3b,0xbe, 0xce,0x75,0xbc,0xfd, 0xcd,0x48,0x45,0x76 };
#else
    ubyte       expected[CMAC_RESULT_SIZE] = { 0x85,0x58,0x16,0xad, 0xaa,0x5c,0x3b,0xbe, 0xce,0x75,0xbc,0xfd, 0xcd,0x48,0x45,0x75 };
#endif
    sbyte4      result;
    MSTATUS     status = OK;

#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
    FIPS_startTestMsg("FIPS_aesCmacKat", "AES-CMAC");
#endif

    /******* AES-CMAC in steps ***************************/
    MOC_MEMSET(cmacOutput, 0x00, CMAC_RESULT_SIZE);

    if (OK > (status = AESCMAC_init(MOC_HASH(hwAccelCtx) key, sizeof(key), &ctx)))
        goto exit;

    if (OK > (status = AESCMAC_update(MOC_HASH(hwAccelCtx) pTestMessage2, 10, &ctx)))
        goto exit;

    if (OK > (status = AESCMAC_update(MOC_HASH(hwAccelCtx) pTestMessage2+10 ,
                                      (sbyte4)MOC_STRLEN((const sbyte*)pTestMessage2) -  10,
                                      &ctx)))
    {
        goto exit;
    }

    if (OK > (status = AESCMAC_final(MOC_HASH(hwAccelCtx) cmacOutput, &ctx)))
        goto exit;

    if (STARTUP_TEST_SHOULDFAIL(FIPS_FORCE_FAIL_AES_CMAC_TESTNUM))
    {
        cmacOutput[0] ^= 0x01;
    }

    if ((OK > MOC_MEMCMP(cmacOutput, expected, CMAC_RESULT_SIZE, &result)) || (0 != result))
    {
        status = ERR_FIPS_HASH_KAT_FAILED;
        goto exit;
    }

exit:
#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
    FIPS_endTestMsg("FIPS_aesCmacKat", "AES-CMAC", status);
#endif

    setFIPS_Status_Once(FIPS_ALGO_AES_CMAC, status);

    return status;

} /* FIPS_aesCmacKat */


/*------------------------------------------------------------------*/

static MSTATUS
FIPS_createHmacHashCtx(hwAccelDescr hwAccelCtx, ubyte **ppRetKey, ubyte4 *pRetKeyLen,
                       ubyte **ppRetHashData, ubyte4 *pRetHashDataLen)
{
    ubyte*  pTempKey = NULL;
    ubyte*  pTempData = NULL;
    MSTATUS status;

    *pRetKeyLen = 32;
    *pRetHashDataLen = 32;

    if (OK > (status = CRYPTO_ALLOC(hwAccelCtx, *pRetKeyLen, TRUE, &pTempKey)))
        goto exit;

    if (OK > (status = CRYPTO_ALLOC(hwAccelCtx, *pRetHashDataLen, TRUE, &pTempData)))
        goto exit;

    if (OK > (status = MOC_MEMSET(pTempKey, 0x00, *pRetKeyLen)))
        goto exit;

    if (OK > (status = MOC_MEMSET(pTempData, 0x00, *pRetHashDataLen)))
        goto exit;

    *ppRetKey = pTempKey;
    *ppRetHashData = pTempData;

    pTempKey = NULL;
    pTempData = NULL;

exit:
    CRYPTO_FREE(hwAccelCtx, TRUE, &pTempData);
    CRYPTO_FREE(hwAccelCtx, TRUE, &pTempKey);

    return status;
}


/*------------------------------------------------------------------*/

static void
FIPS_deleteHmacHashCtx(hwAccelDescr hwAccelCtx, ubyte **ppFreeKey, ubyte **ppFreeHashData)
{
    CRYPTO_FREE(hwAccelCtx, TRUE, ppFreeHashData);
    CRYPTO_FREE(hwAccelCtx, TRUE, ppFreeKey);
}


/*------------------------------------------------------------------*/

static MSTATUS
FIPS_doKatHmacHash(hwAccelDescr hwAccelCtx, const BulkHashAlgo *pHashSuite, ubyte* pKey, ubyte4 keyLen, ubyte* pData, ubyte4 dataLen, ubyte* pExpect, intBoolean isForceFail)
{
    HMAC_CTX*   pHMACCtx = NULL;
    sbyte4      cmpRes = 0;
    ubyte*      pResult = NULL;
    MSTATUS     status = OK;

    if (NULL == pHashSuite)
    {
        status = ERR_FIPS_HMAC_HASH_KAT_NULL;
        goto exit;
    }

    if (OK > (status = CRYPTO_ALLOC(hwAccelCtx, pHashSuite->digestSize, TRUE, &pResult)))
        goto exit;

    if (OK > (status = HmacCreate(MOC_HASH(hwAccelCtx) &pHMACCtx, pHashSuite)))
        goto exit;

    if (OK > (status = HmacKey(MOC_HASH(hwAccelCtx) pHMACCtx, pKey, keyLen)))
        goto exit;

    if (OK > (status = HmacUpdate(MOC_HASH(hwAccelCtx) pHMACCtx, pData, dataLen)))
        goto exit;

    if (OK > (status = HmacFinal(MOC_HASH(hwAccelCtx) pHMACCtx, pResult)))
        goto exit;

    if (TRUE == isForceFail)
        *pResult ^= 0x80;

    if (OK != MOC_MEMCMP(pResult, pExpect, pHashSuite->digestSize, &cmpRes))
    {
        status = ERR_FIPS_HMAC_HASH_KAT_FAILED;
        goto exit;
    }

    if (0 != cmpRes)
    {
        status = ERR_FIPS_HMAC_HASH_KAT_FAILED;
        goto exit;
    }

exit:
    HmacDelete(MOC_HASH(hwAccelCtx) &pHMACCtx);
    CRYPTO_FREE(hwAccelCtx, TRUE, &pResult);

    return status;
}


/*------------------------------------------------------------------*/

MOC_EXTERN MSTATUS
FIPS_hmacHashKat(hwAccelDescr hwAccelCtx,
                 const char *pTestName,
                 const BulkHashAlgo *pHmacSuite,
                 ubyte *pExpect, ubyte4 expectLen,
                 intBoolean isForceFail)
{
    ubyte*              pKey = NULL;
    ubyte4              keyLen;
    ubyte*              pData = NULL;
    ubyte4              dataLen;
    MSTATUS             status = OK;

#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
    FIPS_startTestMsg("FIPS_hmacHashKat", pTestName);
#endif

    if (expectLen != pHmacSuite->digestSize)
    {
        status = ERR_FIPS_HMAC_HASH_KAT_LEN_FAILED;
        goto exit;
    }

    if (OK > (status = FIPS_createHmacHashCtx(hwAccelCtx, &pKey, &keyLen, &pData, &dataLen)))
        goto exit;

    if (OK > (status = FIPS_doKatHmacHash(hwAccelCtx, pHmacSuite, pKey, keyLen, pData, dataLen, pExpect, isForceFail)))
        goto exit;

exit:
#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
    FIPS_endTestMsg("FIPS_hmacHashKat", pTestName, status);
#endif

    FIPS_deleteHmacHashCtx(hwAccelCtx, &pKey, &pData);

    setFIPS_Status_Once(FIPS_ALGO_HMAC, status);

    return status;

} /* FIPS_hmacHashKat */


/*------------------------------------------------------------------*/

MOC_EXTERN MSTATUS
FIPS_hmacSha1Kat(hwAccelDescr hwAccelCtx)
{
    ubyte expect[] = {
        0x66,0x04,0x09,0x90,0xc7,0x99,0x2a,0x2a,
        0x00,0xd0,0x37,0xd0,0xb8,0x63,0x1c,0x0d,
        0xb1,0x78,0x58,0x97
    };

    MSTATUS status = FIPS_hmacHashKat(hwAccelCtx, "HMAC-SHA-1", &SHA1Suite, expect, sizeof(expect), FIPS_FORCE_FAIL_HMAC_SHA1_TEST);
    setFIPS_Status_Once(FIPS_ALGO_SHA1, status);
    return status;


} /* FIPS_hmacSha1Kat */


/*------------------------------------------------------------------*/

MOC_EXTERN MSTATUS
FIPS_hmacSha224Kat(hwAccelDescr hwAccelCtx)
{
    ubyte expect[] = {
        0xc8,0xed,0x8a,0x8b,0xca,0xaf,0xad,0x43,
        0xcb,0xd9,0x7d,0x82,0x95,0x04,0x3d,0xd2,
        0x31,0x34,0xcc,0xd9,0xc3,0x90,0x00,0x0e,
        0xd4,0x70,0xbb,0x48
    };

    MSTATUS status = FIPS_hmacHashKat(hwAccelCtx, "HMAC-SHA-224", &SHA224Suite, expect, sizeof(expect), FIPS_FORCE_FAIL_HMAC_SHA224_TEST);
    setFIPS_Status_Once(FIPS_ALGO_SHA256, status);
    return status;

} /* FIPS_hmacSha224Kat */


/*------------------------------------------------------------------*/

MOC_EXTERN MSTATUS
FIPS_hmacSha256Kat(hwAccelDescr hwAccelCtx)
{
    ubyte expect[] = {
        0x33,0xad,0x0a,0x1c,0x60,0x7e,0xc0,0x3b,
        0x09,0xe6,0xcd,0x98,0x93,0x68,0x0c,0xe2,
        0x10,0xad,0xf3,0x00,0xaa,0x1f,0x26,0x60,
        0xe1,0xb2,0x2e,0x10,0xf1,0x70,0xf9,0x2a
    };

    MSTATUS status = FIPS_hmacHashKat(hwAccelCtx, "HMAC-SHA-256", &SHA256Suite, expect, sizeof(expect), FIPS_FORCE_FAIL_HMAC_SHA256_TEST);
    setFIPS_Status_Once(FIPS_ALGO_SHA256, status);
    return status;

} /* FIPS_hmacSha256Kat */


/*------------------------------------------------------------------*/

MOC_EXTERN MSTATUS
FIPS_hmacSha384Kat(hwAccelDescr hwAccelCtx)
{
    ubyte expect[] = {
        0xe6,0x65,0xec,0x75,0xdc,0xa3,0x23,0xdf,
        0x31,0x80,0x40,0x60,0xe1,0xb0,0xd8,0x28,
        0xb5,0x0a,0x6a,0x8a,0x53,0x9c,0xfe,0xdd,
        0x9a,0xa0,0x07,0x4b,0x5b,0x36,0x44,0x5d,
        0xef,0xbc,0x47,0x45,0x3d,0xf8,0xd0,0xc1,
        0x4b,0x7a,0xd2,0x06,0x2e,0x7b,0xbd,0xb1
    };

    MSTATUS status = FIPS_hmacHashKat(hwAccelCtx, "HMAC-SHA-384", &SHA384Suite, expect, sizeof(expect), FIPS_FORCE_FAIL_HMAC_SHA384_TEST);
    setFIPS_Status_Once(FIPS_ALGO_SHA512, status);
    return status;

} /* FIPS_hmacSha384Kat */


/*------------------------------------------------------------------*/

MOC_EXTERN MSTATUS
FIPS_hmacSha512Kat(hwAccelDescr hwAccelCtx)
{
    ubyte expect[] = {
        0xba,0xe4,0x6c,0xeb,0xeb,0xbb,0x90,0x40,
        0x9a,0xbc,0x5a,0xcf,0x7a,0xc2,0x1f,0xdb,
        0x33,0x9c,0x01,0xce,0x15,0x19,0x2c,0x52,
        0xfb,0x9e,0x8a,0xa1,0x1a,0x8d,0xe9,0xa4,
        0xea,0x15,0xa0,0x45,0xf2,0xbe,0x24,0x5f,
        0xbb,0x98,0x91,0x6a,0x9a,0xe8,0x1b,0x35,
        0x3e,0x33,0xb9,0xc4,0x2a,0x55,0x38,0x0c,
        0x51,0x58,0x24,0x1d,0xae,0xb3,0xc6,0xdd
    };

    MSTATUS status = FIPS_hmacHashKat(hwAccelCtx, "HMAC-SHA-512", &SHA512Suite, expect, sizeof(expect), FIPS_FORCE_FAIL_HMAC_SHA512_TEST);
    setFIPS_Status_Once(FIPS_ALGO_SHA512, status);
    return status;

} /* FIPS_hmacSha512Kat */


/*------------------------------------------------------------------*/

static MSTATUS
FIPS_createSymCtx(hwAccelDescr hwAccelCtx,
                  const BulkEncryptionAlgo *pBulkEncAlgo,
                  ubyte **ppRetKey, ubyte4 keyLen,
                  ubyte **ppRetData, ubyte4 *pRetDataLen,
                  ubyte **ppRetIv)
{
    ubyte*  pTempKey = NULL;
    ubyte*  pTempData = NULL;
    ubyte*  pTempIv = NULL;
    MSTATUS status;

    *pRetDataLen = 32;

    if (OK > (status = CRYPTO_ALLOC(hwAccelCtx, keyLen, TRUE, &pTempKey)))
        goto exit;

    if (OK > (status = CRYPTO_ALLOC(hwAccelCtx, *pRetDataLen, TRUE, &pTempData)))
        goto exit;

    if (OK > (status = CRYPTO_ALLOC(hwAccelCtx, pBulkEncAlgo->blockSize, TRUE, &pTempIv)))
        goto exit;

    if (OK > (status = MOC_MEMSET(pTempKey, 0x00, keyLen)))
        goto exit;

    if (OK > (status = MOC_MEMSET(pTempData, 0x00, *pRetDataLen)))
        goto exit;

    if (OK > (status = MOC_MEMSET(pTempIv, 0x00, pBulkEncAlgo->blockSize)))
        goto exit;

    *ppRetKey = pTempKey;
    *ppRetData = pTempData;
    *ppRetIv = pTempIv;

    pTempKey = NULL;
    pTempData = NULL;
    pTempIv = NULL;

exit:
    CRYPTO_FREE(hwAccelCtx, TRUE, &pTempIv);
    CRYPTO_FREE(hwAccelCtx, TRUE, &pTempData);
    CRYPTO_FREE(hwAccelCtx, TRUE, &pTempKey);

    return status;
}


/*------------------------------------------------------------------*/

static void
FIPS_deleteSymCtx(hwAccelDescr hwAccelCtx, ubyte **ppFreeKey, ubyte **ppFreeHashData, ubyte **ppFreeIv)
{
    CRYPTO_FREE(hwAccelCtx, TRUE, ppFreeHashData);
    CRYPTO_FREE(hwAccelCtx, TRUE, ppFreeKey);
    CRYPTO_FREE(hwAccelCtx, TRUE, ppFreeIv);
}


/*------------------------------------------------------------------*/

static MSTATUS
FIPS_doKatSymmetric(hwAccelDescr hwAccelCtx,
                    const BulkEncryptionAlgo *pBulkEncAlgo,
                    ubyte* pKey, sbyte4 keyLen,
                    ubyte* pData, sbyte4 dataLen,
                    sbyte4 encrypt, ubyte* pIv,
                    ubyte* pExpect,
                    intBoolean isForceFail)
{
    BulkCtx ctx = NULL;
    sbyte4  cmpRes = 0;
    MSTATUS status = OK;

    if (NULL == pBulkEncAlgo)
    {
        status = ERR_FIPS_SYM_KAT_NULL;
        goto exit;
    }

    ctx = pBulkEncAlgo->createFunc(MOC_SYM(hwAccelCtx) pKey, keyLen, encrypt);

    if (NULL == ctx)
    {
        status = ERR_FIPS_SYM_KAT_NULL;
        goto exit;
    }

    if (OK > (status = pBulkEncAlgo->cipherFunc(MOC_SYM(hwAccelCtx) ctx, pData, dataLen, encrypt, pIv)))
        goto exit;

    if (TRUE == isForceFail)
        *pData ^= 0x80;

    if (OK != MOC_MEMCMP(pData, pExpect, dataLen, &cmpRes))
    {
        status = ERR_FIPS_SYM_KAT_FAILED;
        goto exit;
    }

    if (0 != cmpRes)
    {
        status = ERR_FIPS_SYM_KAT_FAILED;
        goto exit;
    }

exit:
    pBulkEncAlgo->deleteFunc(MOC_SYM(hwAccelCtx) &ctx);

    return status;

} /* FIPS_doKatSymmetric */


/*------------------------------------------------------------------*/

MOC_EXTERN MSTATUS
FIPS_symKat(hwAccelDescr hwAccelCtx,
            const char *pTestName,
            const BulkEncryptionAlgo *pBulkEncAlgo, ubyte4 keyLen,
            sbyte4 encrypt,
            ubyte *pExpect, ubyte4 expectLen,
            intBoolean isForceFail)
{
    ubyte*              pKey = NULL;
    ubyte*              pData = NULL;
    ubyte4              dataLen = expectLen;
    ubyte*              pIv = NULL;
    MSTATUS             status = OK;

#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
    FIPS_startTestMsg("FIPS_symKat", pTestName);
#endif

    if ((0 < pBulkEncAlgo->blockSize) && (0 != (expectLen % pBulkEncAlgo->blockSize)))
    {
        status = ERR_FIPS_SYM_KAT_LEN_FAILED;
        goto exit;
    }

    if (OK > (status = FIPS_createSymCtx(hwAccelCtx, pBulkEncAlgo, &pKey, keyLen, &pData, &dataLen, &pIv)))
        goto exit;

     if (OK > (status = FIPS_doKatSymmetric(hwAccelCtx, pBulkEncAlgo,
                                            pKey, keyLen, pData, dataLen,
                                            encrypt, pIv,
                                            pExpect, isForceFail)))
     {
         goto exit;
     }

exit:
#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
    FIPS_endTestMsg("FIPS_symKat", pTestName, status);
#endif

    FIPS_deleteSymCtx(hwAccelCtx, &pKey, &pData, &pIv);

    return status;

} /* FIPS_symKat */


/*------------------------------------------------------------------*/

MOC_EXTERN MSTATUS
FIPS_aes256CbcKat(hwAccelDescr hwAccelCtx)
{
    ubyte expect_enc[] = {
        0xdc,0x95,0xc0,0x78,0xa2,0x40,0x89,0x89,
        0xad,0x48,0xa2,0x14,0x92,0x84,0x20,0x87,
        0x08,0xc3,0x74,0x84,0x8c,0x22,0x82,0x33,
        0xc2,0xb3,0x4f,0x33,0x2b,0xd2,0xe9,0xd3
    };
    ubyte expect_dec[] = {
        0x67,0x67,0x1c,0xe1,0xfa,0x91,0xdd,0xeb,
        0x0f,0x8f,0xbb,0xb3,0x66,0xb5,0x31,0xb4,
        0x67,0x67,0x1c,0xe1,0xfa,0x91,0xdd,0xeb,
        0x0f,0x8f,0xbb,0xb3,0x66,0xb5,0x31,0xb4
    };

    MSTATUS status;

    if (OK > (status = FIPS_symKat(hwAccelCtx, "AES-CBC-256-ENC", &AESCBCSuite, 32, DO_ENCRYPT, expect_enc, sizeof(expect_enc), FIPS_FORCE_FAIL_AES_CBC_ENC_TEST)))
        goto exit;

    status = FIPS_symKat(hwAccelCtx, "AES-CBC-256-DEC", &AESCBCSuite, 32, DO_DECRYPT, expect_dec, sizeof(expect_dec), FIPS_FORCE_FAIL_AES_CBC_DEC_TEST);

exit:
	setFIPS_Status_Once(FIPS_ALGO_AES_CBC, status);
	return status;

} /* FIPS_aes256CbcKat */

/*------------------------------------------------------------------*/

MOC_EXTERN MSTATUS
FIPS_aesCfbKat(hwAccelDescr hwAccelCtx)
{
    ubyte expect_enc[] = {
        0xdc,0x95,0xc0,0x78,0xa2,0x40,0x89,0x89,
        0xad,0x48,0xa2,0x14,0x92,0x84,0x20,0x87,
        0x08,0xc3,0x74,0x84,0x8c,0x22,0x82,0x33,
        0xc2,0xb3,0x4f,0x33,0x2b,0xd2,0xe9,0xd3
    };

    ubyte expect_dec[] = {
        0xDC,0x95,0xC0,0x78,0xA2,0x40,0x89,0x89,
        0xAD,0x48,0xA2,0x14,0x92,0x84,0x20,0x87,
        0xDC,0x95,0xC0,0x78,0xA2,0x40,0x89,0x89,
        0xAD,0x48,0xA2,0x14,0x92,0x84,0x20,0x87
    };

    MSTATUS status;

    if (OK > (status = FIPS_symKat(hwAccelCtx, "AES-CFB-128-ENC", &AESCFBSuite, 32, DO_ENCRYPT, expect_enc, sizeof(expect_enc), FIPS_FORCE_FAIL_AES_CFB_ENC_TEST)))
        goto exit;

    status = FIPS_symKat(hwAccelCtx, "AES-CFB-128-DEC", &AESCFBSuite, 32, DO_DECRYPT, expect_dec, sizeof(expect_dec), FIPS_FORCE_FAIL_AES_CFB_DEC_TEST);

exit:
	setFIPS_Status_Once(FIPS_ALGO_AES_CFB, status);
	return status;

} /* FIPS_aesCfbKat */


MOC_EXTERN MSTATUS
FIPS_aesOfbKat(hwAccelDescr hwAccelCtx)
{
    ubyte expect_enc[] = {
        0xdc,0x95,0xc0,0x78,0xa2,0x40,0x89,0x89,
        0xad,0x48,0xa2,0x14,0x92,0x84,0x20,0x87,
        0x08,0xc3,0x74,0x84,0x8c,0x22,0x82,0x33,
        0xc2,0xb3,0x4f,0x33,0x2b,0xd2,0xe9,0xd3
    };

    ubyte expect_dec[] = {
        0xdc,0x95,0xc0,0x78,0xa2,0x40,0x89,0x89,
        0xad,0x48,0xa2,0x14,0x92,0x84,0x20,0x87,
        0x08,0xc3,0x74,0x84,0x8c,0x22,0x82,0x33,
        0xc2,0xb3,0x4f,0x33,0x2b,0xd2,0xe9,0xd3
    };

    MSTATUS status;

    if (OK > (status = FIPS_symKat(hwAccelCtx, "AES-OFB-128-ENC", &AESOFBSuite, 32, DO_ENCRYPT, expect_enc, sizeof(expect_enc), FIPS_FORCE_FAIL_AES_OFB_ENC_TEST)))
        goto exit;

    status = FIPS_symKat(hwAccelCtx, "AES-OFB-128-DEC", &AESOFBSuite, 32, DO_DECRYPT, expect_dec, sizeof(expect_dec), FIPS_FORCE_FAIL_AES_OFB_DEC_TEST);

exit:
	setFIPS_Status_Once(FIPS_ALGO_AES_OFB, status);
	return status;

} /* FIPS_aesOfbKat */



/*------------------------------------------------------------------*/

MOC_EXTERN MSTATUS
FIPS_aes256EcbKat(hwAccelDescr hwAccelCtx)
{
    ubyte expect_enc[] = {
        0xdc,0x95,0xc0,0x78,0xa2,0x40,0x89,0x89,
        0xad,0x48,0xa2,0x14,0x92,0x84,0x20,0x87,
        0xdc,0x95,0xc0,0x78,0xa2,0x40,0x89,0x89,
        0xad,0x48,0xa2,0x14,0x92,0x84,0x20,0x87
    };
    ubyte expect_dec[] = {
        0x67,0x67,0x1c,0xe1,0xfa,0x91,0xdd,0xeb,
        0x0f,0x8f,0xbb,0xb3,0x66,0xb5,0x31,0xb4,
        0x67,0x67,0x1c,0xe1,0xfa,0x91,0xdd,0xeb,
        0x0f,0x8f,0xbb,0xb3,0x66,0xb5,0x31,0xb4
    };

    MSTATUS status;

    if (OK > (status = FIPS_symKat(hwAccelCtx, "AES-ECB-256-ENC", &AESECBSuite, 32, DO_ENCRYPT, expect_enc, sizeof(expect_enc), FIPS_FORCE_FAIL_AES_ECB_ENC_TEST)))
        goto exit;

    status = FIPS_symKat(hwAccelCtx, "AES-ECB-256-DEC", &AESECBSuite, 32, DO_DECRYPT, expect_dec, sizeof(expect_dec), FIPS_FORCE_FAIL_AES_ECB_DEC_TEST);

exit:
	setFIPS_Status_Once(FIPS_ALGO_AES_ECB, status);
    return status;

} /* FIPS_aes256EcbKat */


/*------------------------------------------------------------------*/

MOC_EXTERN MSTATUS
FIPS_aes256CtrKat(hwAccelDescr hwAccelCtx)
{
    ubyte expect_enc[] = {
        0x66,0xe9,0x4b,0xd4,0xef,0x8a,0x2c,0x3b,
        0x88,0x4c,0xfa,0x59,0xca,0x34,0x2b,0x2e,
        0x58,0xe2,0xfc,0xce,0xfa,0x7e,0x30,0x61,
        0x36,0x7f,0x1d,0x57,0xa4,0xe7,0x45,0x5a
    };
    ubyte expect_dec[] = {
        0x66,0xe9,0x4b,0xd4,0xef,0x8a,0x2c,0x3b,
        0x88,0x4c,0xfa,0x59,0xca,0x34,0x2b,0x2e,
        0x58,0xe2,0xfc,0xce,0xfa,0x7e,0x30,0x61,
        0x36,0x7f,0x1d,0x57,0xa4,0xe7,0x45,0x5a
    };

    MSTATUS status;

    if (OK > (status = FIPS_symKat(hwAccelCtx, "AES-CTR-256-ENC", &AESCTRSuite, 32, DO_ENCRYPT, expect_enc, sizeof(expect_enc), FIPS_FORCE_FAIL_AES_CTR_ENC_TEST)))
        goto exit;

    status = FIPS_symKat(hwAccelCtx, "AES-CTR-256-DEC", &AESCTRSuite, 32, DO_DECRYPT, expect_dec, sizeof(expect_dec), FIPS_FORCE_FAIL_AES_CTR_DEC_TEST);

exit:
	setFIPS_Status_Once(FIPS_ALGO_AES_CTR, status);
	return status;

} /* FIPS_aes256CtrKat */


/*------------------------------------------------------------------*/

MOC_EXTERN MSTATUS
FIPS_tdesCbcKat(hwAccelDescr hwAccelCtx)
{
    ubyte expect_enc[] = {
        0x8c,0xa6,0x4d,0xe9,0xc1,0xb1,0x23,0xa7,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x8c,0xa6,0x4d,0xe9,0xc1,0xb1,0x23,0xa7,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
    };
    ubyte expect_dec[] = {
        0x8c,0xa6,0x4d,0xe9,0xc1,0xb1,0x23,0xa7,
        0x8c,0xa6,0x4d,0xe9,0xc1,0xb1,0x23,0xa7,
        0x8c,0xa6,0x4d,0xe9,0xc1,0xb1,0x23,0xa7,
        0x8c,0xa6,0x4d,0xe9,0xc1,0xb1,0x23,0xa7
    };

    MSTATUS status;

    if (OK > (status = FIPS_symKat(hwAccelCtx, "3DES-168-EDE-CBC-ENC", &TDESCBCSuite, 24, DO_ENCRYPT, expect_enc, sizeof(expect_enc), FIPS_FORCE_FAIL_3DES_CBC_ENC_TEST)))
        goto exit;

    status = FIPS_symKat(hwAccelCtx, "3DES-168-EDE-CBC-DEC", &TDESCBCSuite, 24, DO_DECRYPT, expect_dec, sizeof(expect_dec), FIPS_FORCE_FAIL_3DES_CBC_DEC_TEST);

exit:
	setFIPS_Status_Once(FIPS_ALGO_3DES, status);
	return status;

} /* FIPS_tdesCbcKat */


/*------------------------------------------------------------------*/

MOC_EXTERN MSTATUS
FIPS_aesCcmKat(hwAccelDescr hwAccelCtx)
{
    aesCcmTestPacketDescr   testPacket;
    aesCcmTestPacketDescr*  pRefPacket;
    ubyte                   M, L;
    ubyte                   output[16];
    sbyte4                  resCmp;
    MSTATUS                 status;

#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
    FIPS_startTestMsg("FIPS_aesCcmKat", "AES-CCM");
#endif

    /* make a copy because we are going to be modifying it */
    MOC_MEMCPY(&testPacket, (pRefPacket = &mAesCcmTestPackets), sizeof(aesCcmTestPacketDescr));

    M = testPacket.resultLen - testPacket.packetLen;
    L = 15 - testPacket.nonceLen;

    status = AESCCM_encrypt(MOC_SYM(hwAccelCtx) M, L, testPacket.key, testPacket.keyLen,
                            testPacket.nonce,
                            testPacket.packet + testPacket.packetHeaderLen,
                            testPacket.packetLen - testPacket.packetHeaderLen,
                            testPacket.packet, testPacket.packetHeaderLen, output);

    if (OK > status)
        goto exit;

    MOC_MEMCMP(testPacket.packet, testPacket.result, testPacket.packetLen, &resCmp);

    if (0 != resCmp)
    {
        status = ERR_FIPS_SYM_KAT_FAILED;
        goto exit;
    }

    MOC_MEMCMP(output, testPacket.result + testPacket.packetLen, M, &resCmp);

    if (0 != resCmp)
    {
        status = ERR_FIPS_SYM_KAT_FAILED;
        goto exit;
    }

    if (STARTUP_TEST_SHOULDFAIL(FIPS_FORCE_FAIL_AES_CCM_TESTNUM))
    {
        output[0] ^= 0x01;
    }

    /* decryption now --> decrypt what we just encrypted */
    status = AESCCM_decrypt(MOC_SYM(hwAccelCtx) M, L, testPacket.key, testPacket.keyLen,
                            testPacket.nonce,
                            testPacket.packet + testPacket.packetHeaderLen,
                            testPacket.packetLen - testPacket.packetHeaderLen,
                            testPacket.packet, testPacket.packetHeaderLen, output);

    if (OK > status)
        goto exit;

    MOC_MEMCMP(testPacket.packet, pRefPacket->packet, testPacket.packetLen, &resCmp);

    if (0 != resCmp)
    {
        status = ERR_FIPS_SYM_KAT_FAILED;
        goto exit;
    }

exit:
#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
    FIPS_endTestMsg("FIPS_aesCcmKat", "AES-CCM", status);
#endif

    setFIPS_Status_Once(FIPS_ALGO_AES_CCM, status);
    return status;

} /* FIPS_aesCcmKat */


/*------------------------------------------------------------------*/

#if defined(__ENABLE_MOCANA_GCM__)

MOC_EXTERN MSTATUS
FIPS_aesGcmKat(hwAccelDescr hwAccelCtx)
{
    ubyte expectedCipherText[16] = {
        0x03, 0x88, 0xda, 0xce, 0x60, 0xb6, 0xa3, 0x92,
        0xf3, 0x28, 0xc2, 0xb9, 0x71, 0xb2, 0xfe, 0x78
    };
    ubyte expectedTag[AES_BLOCK_SIZE] = {
        0xab, 0x6e, 0x47, 0xd4, 0x2c, 0xec, 0x13, 0xbd,
        0xf5, 0x3a, 0x67, 0xb2, 0x12, 0x57, 0xbd, 0xdf
    };

    ubyte           key[16];
    ubyte           nonce[12];
    ubyte           data[16];
    ubyte           plainText[16];
    ubyte           tag[AES_BLOCK_SIZE];
    sbyte4          result;
    MSTATUS         status = OK;
    gcm_ctx_256b*   pC = 0;


#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
    FIPS_startTestMsg("FIPS_aesGcmKat", "AES-GCM");
#endif

    MOC_MEMSET(key, 0x00, 16);
    MOC_MEMSET(nonce, 0x00, 12);
    MOC_MEMSET(plainText, 0x00, 16);

    MOC_MEMCPY(data, plainText, 16);

    /* Encrypt test */
    if (NULL == (pC = GCM_createCtx_256b(MOC_SYM(hwAccelCtx) key, 16, TRUE)))
    {
        status = ERR_FIPS_SYM_KAT_FAILED;
        goto exit;
    }

    if (OK > (status = GCM_init_256b(MOC_SYM(hwAccelCtx) pC, nonce, 12, NULL, 0)))
        goto exit;

    if (OK > (status = GCM_update_encrypt_256b(MOC_SYM(hwAccelCtx) pC, data, 16)))
        goto exit;

    if (OK > (status = GCM_final_256b(pC, tag)))
        goto exit;

    if (OK > (status = GCM_deleteCtx_256b(MOC_SYM(hwAccelCtx) (BulkCtx*)&pC)))
        goto exit;

    if (STARTUP_TEST_SHOULDFAIL(FIPS_FORCE_FAIL_AES_GCM_ENC_TESTNUM))
    {
        *data ^= 0x01;
    }

    if ((OK > MOC_MEMCMP(data, expectedCipherText, 16, &result)) || (0 != result))
    {
        status = ERR_FIPS_SYM_KAT_FAILED;
        goto exit;
    }

    if ((OK > MOC_MEMCMP(tag, expectedTag, AES_BLOCK_SIZE, &result)) || (0 != result))
    {
        status = ERR_FIPS_SYM_KAT_FAILED;
        goto exit;
    }

    /* Decrypt test */
    if (NULL == (pC = GCM_createCtx_256b(MOC_SYM(hwAccelCtx) key, 16, FALSE)))
    {
        status = ERR_FIPS_SYM_KAT_FAILED;
        goto exit;
    }

    if (OK > (status = GCM_init_256b(MOC_SYM(hwAccelCtx) pC, nonce, 12, NULL, 0)))
        goto exit;

    if (OK > (status = GCM_update_decrypt_256b(MOC_SYM(hwAccelCtx) pC, data, 16)))
        goto exit;

    if (OK > (status = GCM_final_256b(pC, tag)))
        goto exit;

    if (OK > (status = GCM_deleteCtx_256b(MOC_SYM(hwAccelCtx) (BulkCtx*)&pC)))
        goto exit;

    if (STARTUP_TEST_SHOULDFAIL(FIPS_FORCE_FAIL_AES_GCM_DEC_TESTNUM))
    {
        *data ^= 0x01;
    }

    if ((OK > MOC_MEMCMP(data, plainText, 16, &result)) || (0 != result))
    {
        status = ERR_FIPS_SYM_KAT_FAILED;
        goto exit;
    }

    if ((OK > MOC_MEMCMP(tag, expectedTag, AES_BLOCK_SIZE, &result)) || (0 != result))
    {
        status = ERR_FIPS_SYM_KAT_FAILED;
        goto exit;
    }

exit:
#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
    FIPS_endTestMsg("FIPS_aesGcmKat", "AES-GCM", status);
#endif

    setFIPS_Status_Once(FIPS_ALGO_AES_GCM, status);
    return status;

} /* FIPS_aesGcmKAT */

#endif /* defined(__ENABLE_MOCANA_GCM__) */

/*------------------------------------------------------------------*/

MOC_EXTERN MSTATUS
FIPS_aesXtsKat(hwAccelDescr hwAccelCtx)
{
    aesXtsTestPacketDescr   testPacket;
    sbyte4                  resCmp;
    MSTATUS                 status;
    BulkCtx                 ctx = 0;

#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
    FIPS_startTestMsg("FIPS_aesXtsKat", "AES-XTS");
#endif

    /* make a copy because we are going to be modifying it */
    MOC_MEMCPY(&testPacket, &AES_XTS_TESTCASE, sizeof(AES_XTS_TESTCASE));

    /* do encrypt first */
    ctx = CreateAESXTSCtx(MOC_SYM(hwAccelCtx) testPacket.key, sizeof(testPacket.key), 1);
    if(0 == ctx)
    {
        status = ERR_FIPS_SYM_KAT_FAILED;
        goto exit;
    }

    status = DoAESXTS(MOC_SYM(hwAccelCtx) ctx, testPacket.plainText, sizeof(testPacket.plainText), 1, testPacket.tweak); 
    if(status < OK)
    {
        goto exit;
    }
    DeleteAESXTSCtx(MOC_SYM(hwAccelCtx) &ctx);

    /* verify encryption */
    MOC_MEMCMP(testPacket.plainText, AES_XTS_TESTCASE.cipherText, sizeof(AES_XTS_TESTCASE.cipherText), &resCmp);

    if (0 != resCmp)
    {
        status = ERR_FIPS_SYM_KAT_FAILED;
        goto exit;
    }

    if (STARTUP_TEST_SHOULDFAIL(FIPS_FORCE_FAIL_AES_XTS_TESTNUM))
    {
        testPacket.plainText[0] ^= 0x01;
    }

    /* decryption now --> decrypt what we just encrypted */
    ctx = CreateAESXTSCtx(MOC_SYM(hwAccelCtx) testPacket.key, sizeof(testPacket.key), 0);
    if(0 == ctx)
    {
        status = ERR_FIPS_SYM_KAT_FAILED;
        goto exit;
    }

    status = DoAESXTS(MOC_SYM(hwAccelCtx) ctx, testPacket.plainText, sizeof(testPacket.plainText), 0, testPacket.tweak); 
    if(status < OK)
    {
        goto exit;
    }
    DeleteAESXTSCtx(MOC_SYM(hwAccelCtx) &ctx);

    /* verify decryption */
    MOC_MEMCMP(testPacket.plainText, AES_XTS_TESTCASE.plainText, sizeof(AES_XTS_TESTCASE.plainText), &resCmp);

    if (0 != resCmp)
    {
        status = ERR_FIPS_SYM_KAT_FAILED;
        goto exit;
    }


exit:
#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
    FIPS_endTestMsg("FIPS_aesXtsKat", "AES-XTS", status);
#endif

    setFIPS_Status_Once(FIPS_ALGO_AES_XTS, status);
    return status;

} /* FIPS_aesXtsKat */



/*------------------------------------------------------------------*/

#if (!defined(__ENABLE_MOCANA_CRYPTO_KERNEL_MODULE_FIPS__))

#if (defined(__ENABLE_MOCANA_DSA__))
static const
ubyte m_dsaKeyBlob[] =
{
  0x00, 0x00, 0x01, 0x01, 0x00, 0x82, 0xc4, 0xcb, 0x18, 0x3e, 0x37, 0x46,
  0x13, 0x30, 0x13, 0xd8, 0x90, 0x35, 0x51, 0xb4, 0xd2, 0x6b, 0x8b, 0x71,
  0x71, 0x0a, 0x32, 0xb5, 0x9f, 0x3c, 0x55, 0x49, 0x25, 0x90, 0x79, 0x11,
  0x92, 0xfb, 0xaf, 0x9e, 0x68, 0x51, 0x0d, 0x60, 0x99, 0x5d, 0x37, 0xc8,
  0x01, 0x78, 0x49, 0x13, 0xba, 0x66, 0x69, 0xb7, 0xfc, 0xfc, 0xaa, 0x02,
  0xa1, 0xa3, 0x8b, 0xe9, 0x65, 0x9f, 0x77, 0x9c, 0xad, 0x30, 0x97, 0x62,
  0x30, 0x8b, 0x65, 0x9b, 0xa2, 0xd6, 0x40, 0xda, 0xf2, 0x45, 0x49, 0x3f,
  0x1b, 0xa3, 0x92, 0xa8, 0xd6, 0xe8, 0x11, 0x18, 0x26, 0x94, 0x52, 0x16,
  0x11, 0xfa, 0x27, 0x62, 0x80, 0x8b, 0xf8, 0x6a, 0xca, 0x8d, 0x4a, 0x07,
  0x5c, 0x84, 0xb5, 0x3f, 0x66, 0x4e, 0xe3, 0x02, 0x56, 0x66, 0x32, 0x7a,
  0x76, 0x93, 0x7c, 0xd4, 0xad, 0xaa, 0xef, 0xf0, 0xae, 0x9c, 0x48, 0x0c,
  0xda, 0x63, 0x2a, 0xd5, 0x8a, 0x69, 0x07, 0x3e, 0x71, 0x5a, 0x92, 0x00,
  0xd3, 0x87, 0x5c, 0x67, 0x71, 0x33, 0x13, 0xff, 0xd4, 0x26, 0x12, 0xaf,
  0x85, 0x09, 0x21, 0x40, 0x8f, 0x0e, 0x72, 0xdc, 0x45, 0xe3, 0x4e, 0x03,
  0xf8, 0xa9, 0xc4, 0x82, 0x31, 0xe8, 0x36, 0xa2, 0x75, 0x29, 0xcf, 0x2c,
  0xe3, 0x40, 0x4d, 0x0d, 0x93, 0xb9, 0xd0, 0x27, 0xb6, 0x82, 0x32, 0x6a,
  0xc2, 0x11, 0x85, 0x57, 0xad, 0x83, 0xe5, 0x4c, 0xca, 0x3a, 0x9e, 0xe4,
  0x13, 0x61, 0x14, 0x26, 0x11, 0x0e, 0x75, 0x0a, 0x70, 0xed, 0xe3, 0x20,
  0x02, 0xce, 0x5f, 0xea, 0x71, 0xbd, 0xc1, 0x0c, 0xe2, 0xa2, 0xed, 0x63,
  0x3a, 0xc2, 0x07, 0xd6, 0x72, 0x82, 0x36, 0x65, 0x23, 0x8f, 0x91, 0x13,
  0x10, 0xfe, 0x42, 0x52, 0x19, 0xfc, 0xfe, 0x64, 0x67, 0xf6, 0xc1, 0xd5,
  0x46, 0x63, 0x20, 0x9b, 0xaa, 0xaa, 0x01, 0x22, 0x53, 0x00, 0x00, 0x00,
  0x21, 0x00, 0xc0, 0x8e, 0x7e, 0xcc, 0xed, 0x16, 0xfa, 0x6d, 0x00, 0x2e,
  0x3f, 0x03, 0x85, 0x46, 0x58, 0x46, 0xb5, 0xd9, 0xc3, 0xaa, 0x72, 0x99,
  0x4a, 0x80, 0x55, 0x9e, 0xbc, 0x62, 0xfc, 0xae, 0x2f, 0x7b, 0x00, 0x00,
  0x01, 0x00, 0x14, 0x0b, 0x51, 0x8c, 0x8b, 0x28, 0x00, 0xf2, 0x5a, 0x0d,
  0x9b, 0x5b, 0x84, 0xe1, 0x43, 0x60, 0x6a, 0xe9, 0xd4, 0x23, 0x4a, 0xd2,
  0xbb, 0x34, 0xab, 0xa9, 0xe7, 0xe4, 0xf1, 0xfe, 0x25, 0xd5, 0xfe, 0xf4,
  0x09, 0x2f, 0x1e, 0xb9, 0x11, 0x31, 0x5f, 0x2f, 0xdf, 0xbd, 0xe5, 0x73,
  0x3d, 0xaa, 0x62, 0x5f, 0xb5, 0x06, 0x6e, 0x03, 0xde, 0xe1, 0xae, 0x24,
  0x97, 0xb0, 0x30, 0x36, 0x14, 0x9f, 0x3f, 0x18, 0xd4, 0xf8, 0x0c, 0x06,
  0x5b, 0xfa, 0x84, 0x52, 0xfb, 0xa2, 0xc2, 0x1d, 0x36, 0x4e, 0xbf, 0x68,
  0xc2, 0x36, 0xba, 0xad, 0x79, 0xc3, 0xb2, 0x35, 0x00, 0xac, 0x00, 0x2f,
  0xc7, 0x5e, 0x3d, 0xd1, 0x41, 0x82, 0x85, 0x62, 0xf6, 0x3f, 0xeb, 0x43,
  0x6e, 0xa3, 0x73, 0x35, 0x99, 0x8e, 0xde, 0xb0, 0xc7, 0xde, 0x50, 0x10,
  0x58, 0xb5, 0x14, 0x50, 0x58, 0x51, 0xfe, 0x82, 0x63, 0x90, 0x07, 0x91,
  0x1f, 0xa7, 0xe2, 0xd3, 0x86, 0xf0, 0x7c, 0x1c, 0x2a, 0x8a, 0x47, 0xc6,
  0x35, 0x74, 0x12, 0xd1, 0x0a, 0xf0, 0x6f, 0x3b, 0x27, 0x91, 0x09, 0x20,
  0x5f, 0x05, 0xeb, 0x24, 0xd9, 0x73, 0x0d, 0xdd, 0xc8, 0x5a, 0xde, 0xe5,
  0x8e, 0xe9, 0xdf, 0x31, 0x28, 0x5c, 0xf4, 0x21, 0xc9, 0x60, 0x6e, 0x51,
  0x2e, 0x13, 0xef, 0x88, 0x38, 0xd1, 0xdc, 0x7a, 0x04, 0x45, 0x66, 0x06,
  0xd2, 0x53, 0xf0, 0x8c, 0x0d, 0x0d, 0xb4, 0xb6, 0xc2, 0x2d, 0x37, 0x31,
  0xa7, 0x9b, 0x4a, 0x44, 0x2a, 0x2a, 0x8c, 0xa8, 0x4d, 0x70, 0x59, 0xb6,
  0x72, 0x51, 0x99, 0x11, 0xe6, 0xc3, 0x4f, 0x83, 0xa5, 0xdf, 0x32, 0x3c,
  0x50, 0x3a, 0x5f, 0xe3, 0x51, 0x5f, 0xc4, 0xb8, 0xec, 0x5f, 0xbb, 0x6c,
  0x6f, 0x5f, 0x01, 0xb6, 0xfd, 0x00, 0x23, 0x59, 0xaa, 0x5d, 0xe6, 0x55,
  0x02, 0xa9, 0x5f, 0x92, 0xca, 0xd3, 0x00, 0x00, 0x01, 0x00, 0x08, 0x77,
  0xd0, 0x6c, 0x5b, 0xc3, 0x51, 0x67, 0x64, 0x6c, 0x19, 0xc2, 0x31, 0xe4,
  0x67, 0xb6, 0x17, 0x5c, 0x4c, 0xb8, 0xb4, 0x27, 0xde, 0x81, 0xca, 0x2f,
  0x17, 0x30, 0x92, 0xfd, 0x69, 0xd6, 0xf7, 0xd9, 0xa5, 0x11, 0x98, 0x41,
  0x6f, 0xc4, 0xe8, 0x98, 0x25, 0xfe, 0x71, 0x5e, 0xbd, 0x24, 0x68, 0x14,
  0xc6, 0xcd, 0x17, 0xbe, 0x2c, 0xd6, 0xf0, 0xef, 0xe6, 0x2d, 0xd4, 0xc0,
  0xad, 0x0c, 0x78, 0xfe, 0x7d, 0xe4, 0x8b, 0xde, 0x9f, 0xb0, 0xbc, 0x6d,
  0x03, 0x83, 0x3a, 0x14, 0xe4, 0xb8, 0x0d, 0xc6, 0xbc, 0x85, 0xbb, 0xe2,
  0xb3, 0xa3, 0x17, 0x20, 0xf9, 0xce, 0x83, 0xe7, 0x82, 0x9f, 0x57, 0xfb,
  0xc1, 0x13, 0x91, 0xb8, 0x55, 0xe2, 0x85, 0x52, 0x5b, 0x06, 0x24, 0xa0,
  0x6f, 0x2a, 0xc7, 0xad, 0x66, 0xc5, 0x66, 0xfe, 0xc1, 0x6c, 0x02, 0x38,
  0x00, 0x03, 0x2a, 0x73, 0x89, 0x3d, 0x8f, 0x36, 0x59, 0x3e, 0x1c, 0xf0,
  0x62, 0x27, 0x45, 0xa2, 0xa5, 0xf8, 0x51, 0x4b, 0x60, 0x21, 0xb5, 0xed,
  0xeb, 0xe0, 0x79, 0x54, 0x43, 0xe0, 0xb2, 0x91, 0xbf, 0xe9, 0x2b, 0x78,
  0xd3, 0xa2, 0xf8, 0x31, 0x7c, 0x06, 0xdb, 0x0c, 0xbc, 0x32, 0xe4, 0x71,
  0x9d, 0xcf, 0x2a, 0x79, 0xda, 0x47, 0x16, 0xba, 0x27, 0x73, 0x75, 0xd8,
  0xb4, 0xdc, 0x1d, 0x1e, 0xd2, 0xfb, 0xdd, 0xac, 0x8b, 0xa5, 0x76, 0xab,
  0x48, 0x92, 0x68, 0xc2, 0x59, 0xe2, 0x71, 0x03, 0x61, 0x4e, 0x5f, 0x85,
  0x19, 0x43, 0xa6, 0x0e, 0xf7, 0x28, 0x17, 0xc3, 0xec, 0xe4, 0x2b, 0xc6,
  0xb1, 0x3e, 0xcb, 0x9c, 0x91, 0xb8, 0xf2, 0x4e, 0xcd, 0xaf, 0xfb, 0x63,
  0x15, 0xe9, 0xcc, 0xc1, 0x2e, 0x4f, 0xb3, 0xf3, 0x5d, 0x6a, 0xb8, 0x01,
  0xc1, 0x97, 0xc0, 0x03, 0x29, 0xe3, 0x60, 0x28, 0x74, 0x0d, 0x63, 0xc5,
  0x91, 0x49, 0x00, 0x00, 0x00, 0x20, 0x64, 0x86, 0xce, 0x78, 0xc9, 0x9a,
  0x81, 0x58, 0x9e, 0xff, 0x46, 0xc9, 0x0a, 0x70, 0x7f, 0x1b, 0x1e, 0x8e,
  0x7c, 0x48, 0xf8, 0x29, 0x40, 0x01, 0x7e, 0x8b, 0x0b, 0x4b, 0x65, 0xa6,
  0xf1, 0x8e
};
#endif


/*------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_DSA__))
static MSTATUS
FIPS_pctDsaTest(hwAccelDescr hwAccelCtx, randomContext *pRandomContext)
{
    DSAKey*         pDSAKey = NULL;
    DSAKey*         pDerivedDSAKey = NULL;
    vlong*          pH  = NULL;
    char*           pData = "Attack at dawn";
    vlong*          pBuff = NULL;
    vlong*          pR = NULL;
    vlong*          pS = NULL;
    intBoolean      isGoodSig;
    MSTATUS         status = OK;

    if (OK > (status = DSA_extractKeyBlob(&pDerivedDSAKey, m_dsaKeyBlob, sizeof(m_dsaKeyBlob))))
        goto exit;

    /* Converting the message string to VLONG */
    if (OK > (status = VLONG_vlongFromByteString(pData, MOC_STRLEN(pData), &pBuff, NULL)))
        goto exit;

    /* Compute the Signature */
    if (OK > (status = DSA_computeSignature(MOC_DSA(hwAccelCtx)
                                            pRandomContext, pDerivedDSAKey,
                                            pBuff, &isGoodSig,
                                            &pR, &pS, NULL)))
    {
        goto exit;
    }

    if (FALSE == isGoodSig)
    {
        status = ERR_FIPS_DSA_FAIL;
        goto exit;
    }

    if (STARTUP_TEST_SHOULDFAIL(FIPS_FORCE_FAIL_DSA_TESTNUM))
    {
        *(pR->pUnits) ^= 0x783F;
    }

    /* Verify the signature */
    if (OK > (status = DSA_verifySignature(MOC_DSA(hwAccelCtx)
                                           pDerivedDSAKey, pBuff,
                                           pR, pS,
                                           &isGoodSig, NULL)))
    {
        goto exit;
    }

    if (FALSE == isGoodSig)
    {
        status = ERR_FIPS_DSA_FAIL;
        goto exit;
    }

    DSA_freeKey(&pDSAKey,NULL);
    DSA_freeKey(&pDerivedDSAKey,NULL);
    VLONG_freeVlong(&pH,NULL);
    VLONG_freeVlong(&pR,NULL);
    VLONG_freeVlong(&pS,NULL);
    VLONG_freeVlong(&pBuff,NULL);

exit:
    return status;
}
#endif /* (defined(__ENABLE_MOCANA_DSA__)) */


/*------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_DSA__))
MOC_EXTERN MSTATUS
FIPS_dsaPct(hwAccelDescr hwAccelCtx, randomContext *pRandomContext)
{
    MSTATUS status = OK;

#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
    FIPS_startTestMsg("FIPS_dsaPct", "DSA");
#endif

    if (OK > (status = FIPS_pctDsaTest(hwAccelCtx, pRandomContext)))
        goto exit;

exit:
#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
    FIPS_endTestMsg("FIPS_dsaPct", "DSA", status);
#endif

    setFIPS_Status_Once(FIPS_ALGO_DSA, status);
    return status;
}
#endif /* (defined(__ENABLE_MOCANA_DSA__)) */


/*---------------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_ECC__))
static MSTATUS
FIPS_pctEcdsaTest(hwAccelDescr hwAccelCtx, randomContext *pRandomContext)
{
    ubyte*              mesg    = (ubyte*)"Sign this!";
    PFEPtr              r       = NULL;
    PFEPtr              s       = NULL;
    PrimeFieldPtr       pPF     = NULL;
    ECCKey*             pNewKey = NULL;
    MSTATUS             status  = OK;


    if (OK > (status = EC_newKey(EC_P256, &pNewKey)))
        goto exit;

    pPF = EC_getUnderlyingField(pNewKey->pCurve);

    if (OK > (status = EC_generateKeyPair(pNewKey->pCurve,
                                          RANDOM_rngFun, pRandomContext,
                                          pNewKey->k, pNewKey->Qx, pNewKey->Qy)))
    {
        goto exit;
    }

    if (OK > (status = PRIMEFIELD_newElement(pPF, &r)))
        goto exit;

    if (OK > (status = PRIMEFIELD_newElement(pPF, &s)))
        goto exit;

    if (OK > (status = ECDSA_sign(pNewKey->pCurve, pNewKey->k, RANDOM_rngFun,
                                  pRandomContext, mesg, sizeof(mesg), r, s)))
    {
        goto exit;
    }

    if (STARTUP_TEST_SHOULDFAIL(FIPS_FORCE_FAIL_ECDSA_TESTNUM))
    {
        r->units[0] ^= 0x783F;
    }

    if (OK > (status = ECDSA_verifySignature(pNewKey->pCurve,
                                             pNewKey->Qx, pNewKey->Qy, mesg, sizeof(mesg),
                                             r, s)))
    {
        goto exit;
    }

    /* do a negative test */
    if (OK <= (status = ECDSA_verifySignature(pNewKey->pCurve,
                                              pNewKey->Qx, pNewKey->Qy, mesg, sizeof(mesg) - 1,
                                              r, s)))
    {
        status = ERR_FIPS_ECDSA_PCT_FAILED;
        goto exit;
    }

    status = OK;

exit:
    PRIMEFIELD_deleteElement(pPF, &r);
    PRIMEFIELD_deleteElement(pPF, &s);

    EC_deleteKey(&pNewKey);

    return status;
}

#endif /* (defined(__ENABLE_MOCANA_ECC__)) */


/*------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_ECC__))
MOC_EXTERN MSTATUS
FIPS_ecdsaPct(hwAccelDescr hwAccelCtx, randomContext *pRandomContext)
{
    MSTATUS status = OK;

#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
    FIPS_startTestMsg("FIPS_ecdsaPct", "ECDSA-P192");
#endif

    if (OK > (status = FIPS_pctEcdsaTest(hwAccelCtx, pRandomContext)))
        goto exit;

exit:
#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
    FIPS_endTestMsg("FIPS_ecdsaPct", "ECDSA-P192", status);
#endif

    setFIPS_Status_Once(FIPS_ALGO_ECC, status);  /* There is overlap. */
	setFIPS_Status_Once(FIPS_ALGO_ECDSA, status);

    return status;
}

#endif /* (defined(__ENABLE_MOCANA_ECC__)) */


/*---------------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_ECC__))
static MSTATUS
FIPS_doEcdhTest(hwAccelDescr hwAccelCtx, randomContext *pRandomContext, PEllipticCurvePtr pEC, intBoolean isForceFail)
{
    ECCKey* pKey1 = 0;
    ECCKey* pKey2 = 0;
    ubyte*  sharedSecret1 = 0;
    ubyte*  sharedSecret2 = 0;
    ubyte4  sharedSecret1Len;
    ubyte4  sharedSecret2Len;
    sbyte4  res;
    MSTATUS status;

    if (OK > (status = EC_newKey(pEC, &pKey1)))
    {
        goto exit;
    }

    if (OK > (status = EC_newKey(pEC, &pKey2)))
    {
        goto exit;
    }

    if (OK > (status = EC_generateKeyPair(pEC, RANDOM_rngFun, pRandomContext, pKey1->k, pKey1->Qx, pKey1->Qy)))
    {
        goto exit;
    }

    if (OK > (status = EC_generateKeyPair(pEC, RANDOM_rngFun, pRandomContext, pKey2->k, pKey2->Qx, pKey2->Qy)))
    {
        goto exit;
    }

    if (OK > (status = ECDH_generateSharedSecretAux(pEC,
                                                    pKey1->Qx,  pKey1->Qy, pKey2->k,
                                                    &sharedSecret2, &sharedSecret2Len, 1)))
    {
        goto exit;
    }

    if (OK > (status = ECDH_generateSharedSecretAux(pEC,
                                                    pKey2->Qx,  pKey2->Qy, pKey1->k,
                                                    &sharedSecret1, &sharedSecret1Len, 1)))
    {
        goto exit;
    }

    if (sharedSecret1Len != sharedSecret2Len)
    {
        status = ERR_FIPS_ECDH_PCT_FAILED;
        goto exit;
    }

    if (isForceFail)
    {
        *sharedSecret1 ^= 0x01;
    }

    MOC_MEMCMP(sharedSecret1, sharedSecret2, sharedSecret1Len, &res);

    if (res != 0)
    {
        status = ERR_FIPS_ECDH_PCT_FAILED;
        goto exit;
    }

exit:
    EC_deleteKey(&pKey1);
    EC_deleteKey(&pKey2);

    if (sharedSecret1)
    {
        FREE(sharedSecret1);
    }

    if (sharedSecret2)
    {
        FREE(sharedSecret2);
    }

    return status;

} /* FIPS_doEcdhTest */

#endif /* (defined(__ENABLE_MOCANA_ECC__)) */


/*---------------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_ECC__))
MOC_EXTERN MSTATUS
FIPS_ecdhPct(hwAccelDescr hwAccelCtx, randomContext *pRandomContext)
{
    MSTATUS         status;

#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
    FIPS_startTestMsg("FIPS_ecdhPct", "ECDH-p-curves");
#endif

    if (OK > (status = FIPS_doEcdhTest(hwAccelCtx, pRandomContext, EC_P521, FIPS_FORCE_FAIL_ECDH_TEST)))
        goto exit;

exit:
#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
    FIPS_endTestMsg("FIPS_ecdhPct", "ECDH-p-curves", status);
#endif

    setFIPS_Status_Once(FIPS_ALGO_ECC, status);  /* There is overlap. */
	setFIPS_Status_Once(FIPS_ALGO_ECDH, status);

    return status;
}
#endif /* (defined(__ENABLE_MOCANA_ECC__)) */


/*------------------------------------------------------------------*/

MOC_EXTERN MSTATUS
FIPS_dhPct(hwAccelDescr hwAccelCtx, randomContext *pRandomContext)
{
    diffieHellmanContext*   pDhServer = NULL;
    diffieHellmanContext*   pDhClient = NULL;
    vlong*                  pVlongQueue = NULL;
    vlong*                  pMpintF = NULL;
    sbyte4                  comparisonResult;
    MSTATUS                 status = OK;

#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
    FIPS_startTestMsg("FIPS_dhPct", "DH-group2");
#endif

    if (OK > (status = DH_allocateServer(MOC_DH(hwAccelCtx) pRandomContext, &pDhServer, DH_GROUP_2)))
        goto exit;

    if (OK > (status = DH_allocate(&pDhClient)))
        goto exit;

    /* clone server's p & g for client */
    if (OK > (status = DH_setPG(MOC_DH(hwAccelCtx) pRandomContext, 20, pDhClient, COMPUTED_VLONG_P(pDhServer), COMPUTED_VLONG_G(pDhServer))))
        goto exit;

    /* Get the client public key into server's DH context */
    if (OK > (status = VLONG_makeVlongFromVlong(COMPUTED_VLONG_F(pDhClient), &pMpintF, &pVlongQueue)))
        goto exit;

    COMPUTED_VLONG_E(pDhServer) = pMpintF; pMpintF = NULL;

    /* Get the server public key into client's DH context */
    if (OK > (status = VLONG_makeVlongFromVlong(COMPUTED_VLONG_F(pDhServer), &pMpintF, &pVlongQueue)))
        goto exit;

    if (STARTUP_TEST_SHOULDFAIL(FIPS_FORCE_FAIL_DH_TESTNUM))
    {
        pMpintF->pUnits[0] ^= 0x01;
    }

    COMPUTED_VLONG_E(pDhClient) = pMpintF; pMpintF = NULL;

    /* Compute shared secret for the server */
    if (OK > (status = DH_computeKeyExchange(MOC_DH(hwAccelCtx) pDhServer, &pVlongQueue)))
        goto exit;

    /* Compute shared secret for the client */
    if (OK > (status = DH_computeKeyExchange(MOC_DH(hwAccelCtx) pDhClient, &pVlongQueue)))
        goto exit;

    comparisonResult = VLONG_compareSignedVlongs(COMPUTED_VLONG_K(pDhClient), COMPUTED_VLONG_K(pDhServer));

    if (0 != comparisonResult)
    {
        status = ERR_FIPS_DH_PCT_FAILED;
        goto exit;
    }

exit:
#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
    FIPS_endTestMsg("FIPS_dhPct", "DH-group2", status);
#endif

    setFIPS_Status_Once(FIPS_ALGO_DH, status);

    if (NULL != pDhServer)
        DH_freeDhContext(&pDhServer, NULL);

    if (NULL != pDhClient)
        DH_freeDhContext(&pDhClient, NULL);

    if (NULL != pVlongQueue)
        VLONG_freeVlongQueue( &pVlongQueue);

    return status;

} /* FIPS_dhPct */

#endif /* !defined(__ENABLE_MOCANA_CRYPTO_KERNEL_MODULE_FIPS__) */


/*------------------------------------------------------------------*/

/* Sub FIPS Tests */
MOC_EXTERN MSTATUS
FIPS_knownAnswerTests(void)
{
    /* Run Known Answer Tests (KAT) */
    hwAccelDescr    hwAccelCtx;
    MSTATUS         status = OK;

#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
    DEBUG_PRINTNL(DEBUG_CRYPTO, (sbyte *) "FIPS_knownAnswerTests:\t\t\tStarted...");
#endif

    if (OK > (MSTATUS)(status = HARDWARE_ACCEL_OPEN_CHANNEL(MOCANA_MSS, &hwAccelCtx)))
    {
#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
        DEBUG_PRINTNL(DEBUG_CRYPTO, (sbyte *) "FIPS_knownAnswerTests: HARDWARE_ACCEL_OPEN_CHANNEL() failed.");
#endif
        return status;
    }

    if (ALGO_TESTSHOULDRUN(FIPS_ALGO_RNG_FIPS186))
    {
        if (OK > (status = FIPS_random186Kat()))
            goto exit;
    }

    if (OK > (status = FIPS_nistRngKat()))
        goto exit;

    /* Note: this doesn't use the RSA_SIMPLE implemenation */
    if (ALGO_TESTSHOULDRUN(FIPS_ALGO_RSA))
    {
		if (OK > (status = FIPS_rsaKat(hwAccelCtx)))
			goto exit;
    }

    /* Hash algorithms */
    if (ALGO_TESTSHOULDRUN(FIPS_ALGO_SHA1))
    {
		if (OK > (status = FIPS_sha1Kat(hwAccelCtx)))
			goto exit;
    }

#if (!defined(__DISABLE_MOCANA_SHA224__))
    if (ALGO_TESTSHOULDRUN(FIPS_ALGO_SHA256))
    {
		if (OK > (status = FIPS_sha224Kat(hwAccelCtx)))
			goto exit;
    }
#endif /* (!defined(__DISABLE_MOCANA_SHA224__)) */

#if (!defined(__DISABLE_MOCANA_SHA256__))
    if (ALGO_TESTSHOULDRUN(FIPS_ALGO_SHA256))
    {
    	if (OK > (status = FIPS_sha256Kat(hwAccelCtx)))
    		goto exit;
    }
#endif /* (!defined(__DISABLE_MOCANA_SHA256__)) */

#if (!defined(__DISABLE_MOCANA_SHA384__))
    if (ALGO_TESTSHOULDRUN(FIPS_ALGO_SHA512))
    {
    	if (OK > (status = FIPS_sha384Kat(hwAccelCtx)))
    		goto exit;
    }
#endif /* (!defined(__DISABLE_MOCANA_SHA384__)) */

#if (!defined(__DISABLE_MOCANA_SHA512__))
    if (ALGO_TESTSHOULDRUN(FIPS_ALGO_SHA512))
    {
    	if (OK > (status = FIPS_sha512Kat(hwAccelCtx)))
    		goto exit;
    }
#endif /* (!defined(__DISABLE_MOCANA_SHA512__)) */

    /* HMAC algorithms */
    if ( (ALGO_TESTSHOULDRUN(FIPS_ALGO_HMAC)) &&
         (ALGO_TESTSHOULDRUN(FIPS_ALGO_SHA1)) )
    {
    	if (OK > (status = FIPS_hmacSha1Kat(hwAccelCtx)))
    		goto exit;
    }

#if (!defined(__DISABLE_MOCANA_SHA224__))
    if ( (ALGO_TESTSHOULDRUN(FIPS_ALGO_HMAC)) &&
         (ALGO_TESTSHOULDRUN(FIPS_ALGO_SHA256)) )
    {
    	if (OK > (status = FIPS_hmacSha224Kat(hwAccelCtx)))
    		goto exit;
    }
#endif /* (!defined(__DISABLE_MOCANA_SHA224__)) */

#if (!defined(__DISABLE_MOCANA_SHA256__))
    if ( (ALGO_TESTSHOULDRUN(FIPS_ALGO_HMAC)) &&
         (ALGO_TESTSHOULDRUN(FIPS_ALGO_SHA256)) )
    {
    	if (OK > (status = FIPS_hmacSha256Kat(hwAccelCtx)))
    		goto exit;
    }
#endif /* (!defined(__DISABLE_MOCANA_SHA256__)) */

#if (!defined(__DISABLE_MOCANA_SHA384__))
    if ( (ALGO_TESTSHOULDRUN(FIPS_ALGO_HMAC)) &&
         (ALGO_TESTSHOULDRUN(FIPS_ALGO_SHA512)) )
    {
    	if (OK > (status = FIPS_hmacSha384Kat(hwAccelCtx)))
    		goto exit;
    }
#endif /* (!defined(__DISABLE_MOCANA_SHA384__)) */

#if (!defined(__DISABLE_MOCANA_SHA512__))
    if ( (ALGO_TESTSHOULDRUN(FIPS_ALGO_HMAC)) &&
         (ALGO_TESTSHOULDRUN(FIPS_ALGO_SHA512)) )
    {
    	if (OK > (status = FIPS_hmacSha512Kat(hwAccelCtx)))
    		goto exit;
    }
#endif /* (!defined(__DISABLE_MOCANA_SHA512__)) */

    /* Symmetrical algorithms */
    if (ALGO_TESTSHOULDRUN(FIPS_ALGO_3DES))
    {
		if (OK > (status = FIPS_tdesCbcKat(hwAccelCtx)))
			goto exit;
    }

    setFIPS_Status_Once(FIPS_ALGO_AES, OK); /* Mark that we are running all the AES tests. */

    if (ALGO_TESTSHOULDRUN(FIPS_ALGO_AES_CBC))
    {
    	if (OK > (status = FIPS_aes256CbcKat(hwAccelCtx)))
    		goto exit;
    }

    if (ALGO_TESTSHOULDRUN(FIPS_ALGO_AES_CTR))
    {
    	if (OK > (status = FIPS_aes256CtrKat(hwAccelCtx)))
    		goto exit;
    }

    if (ALGO_TESTSHOULDRUN(FIPS_ALGO_AES_ECB))
    {
    	if (OK > (status = FIPS_aes256EcbKat(hwAccelCtx)))
    		goto exit;
    }

    if (ALGO_TESTSHOULDRUN(FIPS_ALGO_AES_CCM))
    {
		if (OK > (status = FIPS_aesCcmKat(hwAccelCtx)))
			goto exit;
    }

    if (ALGO_TESTSHOULDRUN(FIPS_ALGO_AES_CMAC))
    {
		if (OK > (status = FIPS_aesCmacKat(hwAccelCtx)))
			goto exit;
    }

    if (ALGO_TESTSHOULDRUN(FIPS_ALGO_AES_XTS))
    {
		if (OK > (status = FIPS_aesXtsKat(hwAccelCtx)))
			goto exit;
    }

    if (ALGO_TESTSHOULDRUN(FIPS_ALGO_AES_CFB))
    {
		if (OK > (status = FIPS_aesCfbKat(hwAccelCtx)))
			goto exit;
    }

    if (ALGO_TESTSHOULDRUN(FIPS_ALGO_AES_OFB))
    {
		if (OK > (status = FIPS_aesOfbKat(hwAccelCtx)))
			goto exit;
    }


#if defined(__ENABLE_MOCANA_GCM__)
    if (ALGO_TESTSHOULDRUN(FIPS_ALGO_AES_GCM))
    {
		if (OK > (status = FIPS_aesGcmKat(hwAccelCtx)))
			goto exit;
    }
#endif

exit:
    HARDWARE_ACCEL_CLOSE_CHANNEL(MOCANA_MSS, &hwAccelCtx);

#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
    DEBUG_PRINTNL(DEBUG_CRYPTO, (sbyte *) "FIPS_knownAnswerTests:\t\t\tFinished");
#endif

    return status;

} /* FIPS_knownAnswerTests */


/*------------------------------------------------------------------*/

#if (!defined(__ENABLE_MOCANA_CRYPTO_KERNEL_MODULE_FIPS__))

MOC_EXTERN MSTATUS
FIPS_pairwiseConsistencyTests(randomContext* pRandomContext)
{
    /* Run Pair-wise Consistency Tests */
    hwAccelDescr    hwAccelCtx;                 /* FIX*/
    randomContext*  pLocalRandomContext;
    MSTATUS         status = OK;

#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
    DEBUG_PRINTNL(DEBUG_CRYPTO, (sbyte *) "FIPS_pairwiseConsistencyTests:\t\t\tStarted...");
#endif

    if (OK > (MSTATUS)(status = HARDWARE_ACCEL_OPEN_CHANNEL(MOCANA_MSS, &hwAccelCtx)))
    {
#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
        DEBUG_PRINTNL(DEBUG_CRYPTO, (sbyte *) "FIPS_pairwiseConsistencyTests: HARDWARE_ACCEL_OPEN_CHANNEL() failed.");
#endif
        return status;
    }

    if (NULL == pRandomContext)
    {
    	if (OK > (status = RANDOM_acquireContext(&pLocalRandomContext)))
    		goto exit;
    }
    else
    {
    	pLocalRandomContext = pRandomContext;
    }

#if (defined(__ENABLE_MOCANA_DSA__))
    if (ALGO_TESTSHOULDRUN(FIPS_ALGO_DSA))
    {
		if (OK > (status = FIPS_dsaPct(hwAccelCtx, pLocalRandomContext)))
		{
#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
			DEBUG_PRINTNL(DEBUG_CRYPTO, (sbyte *) "FIPS_pairwiseConsistencyTests: DSA FAILED!");
#endif
			goto exit;
		}
    }
#endif /* (defined(__ENABLE_MOCANA_DSA__)) */


#if (defined(__ENABLE_MOCANA_ECC__) && defined(__ENABLE_MOCANA_FIPS_ECDSA__))
    if (ALGO_TESTSHOULDRUN(FIPS_ALGO_ECDSA))
    {
    	if (OK > (status = FIPS_ecdsaPct(hwAccelCtx, pLocalRandomContext)))
    	{
#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
        	DEBUG_PRINTNL(DEBUG_CRYPTO, (sbyte *) "FIPS_pairwiseConsistencyTests: ECDSA FAILED!");
#endif
        	goto exit;
    	}
    }
#endif /* (defined(__ENABLE_MOCANA_ECC__) && defined(__ENABLE_MOCANA_FIPS_ECDSA__)) */

#if (defined(__ENABLE_MOCANA_ECC__) && defined(__ENABLE_MOCANA_FIPS_ECDH__))
    if (ALGO_TESTSHOULDRUN(FIPS_ALGO_ECDH))
    {
    	if (OK > (status = FIPS_ecdhPct(hwAccelCtx, pLocalRandomContext)))
    	{
#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
        	DEBUG_PRINTNL(DEBUG_CRYPTO, (sbyte *) "FIPS_pairwiseConsistencyTests: ECDH FAILED!");
#endif
        	goto exit;
    	}
    }
#endif /* (defined(__ENABLE_MOCANA_ECC__) && defined(__ENABLE_MOCANA_FIPS_ECDH__)) */

    if (ALGO_TESTSHOULDRUN(FIPS_ALGO_DH))
    {
    	if (OK > (status = FIPS_dhPct(hwAccelCtx, pLocalRandomContext)))
    	{
#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
        	DEBUG_PRINTNL(DEBUG_CRYPTO, (sbyte *) "FIPS_pairwiseConsistencyTests: DH FAILED!");
#endif
        	goto exit;
    	}
    }

exit:
    HARDWARE_ACCEL_CLOSE_CHANNEL(MOCANA_MSS, &hwAccelCtx);
    if (NULL == pRandomContext)
    {
    	RANDOM_releaseContext(&pLocalRandomContext);
    }

#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
    DEBUG_PRINTNL(DEBUG_CRYPTO, (sbyte *) "FIPS_pairwiseConsistencyTests:\t\t\tFinished");
#endif

    return status;
}

#endif /* !defined(__ENABLE_MOCANA_CRYPTO_KERNEL_MODULE_FIPS__) */


/*------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_FIPS_INTEG_TEST__))

#if (defined(__ENABLE_MOCANA_FIPS_STATIC_INTEG_TEST__))
MOC_EXTERN MSTATUS
FIPS_INTEG_TEST_hash_locate(ubyte* hashReturn)
{
    MSTATUS status = OK;
    int i;

    extern const unsigned char FIPS_MOCANA_SHA[];

    for (i=0; i<SHA1_RESULT_SIZE; ++i) {
    	hashReturn[i] = (ubyte)FIPS_MOCANA_SHA[i];
    }

    return status;
}

MOC_EXTERN MSTATUS
FIPS_INTEG_TEST_hash_inplace(ubyte* hashReturn)
{
    MSTATUS status = OK;
    int i;
    int sLen = SHA1_RESULT_SIZE;

    /* The edges of the static code regions in ".text" and ".rodata" */
    extern const void *FIPS_MOCANA_START(), *FIPS_MOCANA_STOP();
    extern const unsigned char FIPS_data0[], FIPS_data1[];

    const BulkHashAlgo SHA1Suite =
    {
    	SHA1_RESULT_SIZE, SHA1_BLOCK_SIZE, SHA1_allocDigest, SHA1_freeDigest,
    	(BulkCtxInitFunc)SHA1_initDigest, (BulkCtxUpdateFunc)SHA1_updateDigest,
    (BulkCtxFinalFunc)SHA1_finalDigest
    };

	ubyte4 keyLen = 117;
	ubyte key[] = {
		0xd6,0xc3,0x44,0xd5,0x0f,0x0d,0xc1,0x88,
		0xbb,0xf8,0x74,0x59,0xcc,0x7e,0xf8,0xc7,
		0xd2,0xbc,0x8b,0x6a,0x14,0xb0,0xc0,0xda,
		0xe0,0x41,0x74,0xcc,0x1f,0x7f,0x02,0x7e,
		0x2c,0x2d,0xbb,0x56,0xe7,0x7d,0x90,0xc6,
		0x05,0x1a,0x12,0x72,0xaa,0x6b,0x9d,0x95,
		0x39,0x17,0xcf,0xa0,0x6b,0xc4,0x3f,0x25,
		0x9e,0x25,0x6c,0xf4,0x70,0x33,0xf4,0x84,
		0x8d,0xba,0x07,0x94,0xc5,0x18,0x1a,0x11,
		0x62,0x41,0xe0,0x3b,0xb8,0x07,0x76,0x04,
		0xfd,0x99,0xde,0xb8,0x5b,0x49,0xae,0xe3,
		0x44,0x92,0x09,0x70,0x06,0x59,0xcf,0x0e,
		0x9f,0x73,0x11,0xd8,0xd2,0x68,0xc8,0x34,
		0x7a,0x76,0xc6,0xfb,0x1f,0xd9,0xec,0xdb,
		0xc7,0x4a,0x9e,0xfa,0x0e
	};

	/* HMAC context pointer */
	HMAC_CTX* pHMACCtx = NULL;

    if (OK > (status = HmacCreate( MOC_HASH(hwAccelCtx) &pHMACCtx, &SHA1Suite )))
      goto exit;

    if (OK > (status = HmacKey( MOC_HASH(hwAccelCtx) pHMACCtx, key, keyLen )))
      goto exit;

	/* Add FIPS bytes from ".text" */
	/* Create data for ".text" */
	if (OK > (status = HmacUpdate( MOC_HASH(hwAccelCtx) pHMACCtx, (char*)FIPS_MOCANA_START, ((size_t)FIPS_MOCANA_STOP-(size_t)FIPS_MOCANA_START) )))
	  goto exit;

	/* Add FIPS bytes from ".rodata" */
	/* Create data for ".rodata" */
	if (OK > (status = HmacUpdate( MOC_HASH(hwAccelCtx) pHMACCtx, FIPS_data0, ((size_t)FIPS_data1-(size_t)FIPS_data0) )))
	  goto exit;

	/* Finalize data streaming */
	if (OK > (status = HmacFinal( MOC_HASH(hwAccelCtx) pHMACCtx, hashReturn
)))
	  goto exit;

	exit:
	HmacDelete( MOC_HASH(sbyte4 hwAccelCtx) &pHMACCtx );

    return status;
}

#else   /* Not: __ENABLE_MOCANA_FIPS_STATIC_INTEG_TEST__ */

MOC_EXTERN MSTATUS
FIPS_INTEG_TEST_hash_load(ubyte* hashReturn, const char* optionalSigFileName)
{
    MSTATUS status = OK;
#ifndef __ENABLE_MOCANA_CRYPTO_KERNEL_MODULE_FIPS__
    FILE*   fhash = NULL;
#else
    sbyte4  fhash = 0;
#endif
    ubyte   buffer = 0;
    ubyte   indexWrite = 0;
    ubyte4  bytesRead;
    ubyte   indexBuf = 0;
    ubyte   hBuffer[SHA1_RESULT_SIZE*2];


#if defined( __RTOS_WIN32__ ) || defined(__RTOS_WINCE__)
    if (optionalSigFileName)
    {
#ifndef __ENABLE_MOCANA_CRYPTO_KERNEL_MODULE_FIPS__
    	fhash = fopen( optionalSigFileName, "rb");
#else
    	fhash = MOC_CRYPTO_fipsSelfTestInit((ubyte *)optionalSigFileName);
#endif
    }
    else
    {
#ifndef __ENABLE_MOCANA_CRYPTO_KERNEL_MODULE_FIPS__
    	TCHAR modulePath[_MAX_PATH+5]; /* add a little buffer to the buffer. 5 should be enough (.sig) */
    	HMODULE hModule;
    	TCHAR* lastBS;

    	hModule = GetModuleHandle(DLL_NAME);

    	if (!GetModuleFileName( hModule, modulePath, _MAX_PATH))
    	{
#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
    		DEBUG_PRINTNL( DEBUG_CRYPTO, (sbyte *) "FIPS_INTEG: GetModuleFileName failed");
#endif
    		status = ERR_FILE_OPEN_FAILED;
    		goto exit;
    	}

    	/* signature file is mss_dll.dll.sig */
    	/* don't bother with splitpath */
    	lastBS = _tcsrchr( modulePath, _T('\\'));
    	if (lastBS)
    	{
    		size_t signatureFileLen = _tcslen(SIGNATURE_FILE);
    		if (lastBS + 2 + signatureFileLen - modulePath > _MAX_PATH + 5)
    		{
#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
    			DEBUG_PRINTNL( DEBUG_CRYPTO, (sbyte *) "FIPS_INTEG: Internal Error -- Constant");
#endif
    			status = ERR_FILE_OPEN_FAILED;
    			goto exit;
    		}

    		memcpy(lastBS+1, SIGNATURE_FILE, (1+signatureFileLen)*sizeof(TCHAR));
    		fhash = _tfopen(modulePath, _T("rt"));
    	}
#else /* WIN32 Kernel Space */
    	fhash = MOC_CRYPTO_fipsSelfTestInit("\\SystemRoot\\system32\\drivers\\moc_crypto.sys.sig");
#endif /* !__ENABLE_MOCANA_CRYPTO_MODULE_FIPS__ */
    }
#else
    /* other OSes like linux uses an absolute file name */
    if (optionalSigFileName)
    {
#ifndef __ENABLE_MOCANA_CRYPTO_KERNEL_MODULE_FIPS__
    	fhash = fopen( optionalSigFileName, "rb");
#else
    	fhash = MOC_CRYPTO_fipsSelfTestInit((ubyte *)optionalSigFileName);
#endif
    }
    else
    {
#ifndef __ENABLE_MOCANA_CRYPTO_KERNEL_MODULE_FIPS__
    	fhash = fopen( FIPS_INTEG_TEST_HASH_FILENAME, "r");
#else
    	fhash = MOC_CRYPTO_fipsSelfTestInit( FIPS_INTEG_TEST_HASH_FILENAME);
#endif
    }
#endif

    if (0 == fhash)
    {
#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
        DEBUG_PRINTNL( DEBUG_CRYPTO, (sbyte *) "FIPS_INTEG: Hash file load error!" );
#endif
        status = ERR_FILE_OPEN_FAILED;
        goto exit;
    }

#ifndef __ENABLE_MOCANA_CRYPTO_KERNEL_MODULE_FIPS__
    bytesRead = (ubyte4)fread(hBuffer, 1, SHA1_RESULT_SIZE*2, fhash);
#else
    MOC_CRYPTO_fipsSelfTestUpdate( fhash, hBuffer, SHA1_RESULT_SIZE*2 );
#endif
    while( SHA1_RESULT_SIZE > indexWrite )
    {
        buffer = hBuffer[indexBuf];
        if( ('0' <= buffer) && ('9' >= buffer) )
            hashReturn[indexWrite] = (buffer - '0') * 16;
        if( ('A' <= buffer) && ('F' >= buffer) )
            hashReturn[indexWrite] = (buffer - ('A' - 10)) * 16;
        if( ('a' <= buffer) && ('f' >= buffer) )
            hashReturn[indexWrite] = (buffer - ('a' - 10)) * 16;

        indexBuf++;
        buffer = hBuffer[indexBuf];

        if( ('0' <= buffer) && ('9' >= buffer) )
            hashReturn[indexWrite] += buffer - '0';
        if( ('A' <= buffer) && ('F' >= buffer) )
            hashReturn[indexWrite] += buffer - ('A' - 10);
        if( ('a' <= buffer) && ('f' >= buffer) )
            hashReturn[indexWrite] += buffer - ('a' - 10);

        indexBuf++;
        indexWrite++;
    }

exit:
    if ( fhash)
    {
#ifndef __ENABLE_MOCANA_CRYPTO_KERNEL_MODULE_FIPS__
        fclose(fhash);
#else
        MOC_CRYPTO_fipsSelfTestFinal( fhash );
#endif
    }

    return status;
}


/*------------------------------------------------------------------*/

MOC_EXTERN MSTATUS
FIPS_INTEG_TEST_hash_bin( ubyte* hashReturn, const char* optionalBinFileName)
{
    MSTATUS status = OK;

    ubyte4 keyLen = 117;
    ubyte key[] = {
        0xd6,0xc3,0x44,0xd5,0x0f,0x0d,0xc1,0x88,
        0xbb,0xf8,0x74,0x59,0xcc,0x7e,0xf8,0xc7,
        0xd2,0xbc,0x8b,0x6a,0x14,0xb0,0xc0,0xda,
        0xe0,0x41,0x74,0xcc,0x1f,0x7f,0x02,0x7e,
        0x2c,0x2d,0xbb,0x56,0xe7,0x7d,0x90,0xc6,
        0x05,0x1a,0x12,0x72,0xaa,0x6b,0x9d,0x95,
        0x39,0x17,0xcf,0xa0,0x6b,0xc4,0x3f,0x25,
        0x9e,0x25,0x6c,0xf4,0x70,0x33,0xf4,0x84,
        0x8d,0xba,0x07,0x94,0xc5,0x18,0x1a,0x11,
        0x62,0x41,0xe0,0x3b,0xb8,0x07,0x76,0x04,
        0xfd,0x99,0xde,0xb8,0x5b,0x49,0xae,0xe3,
        0x44,0x92,0x09,0x70,0x06,0x59,0xcf,0x0e,
        0x9f,0x73,0x11,0xd8,0xd2,0x68,0xc8,0x34,
        0x7a,0x76,0xc6,0xfb,0x1f,0xd9,0xec,0xdb,
        0xc7,0x4a,0x9e,0xfa,0x0e
    };

    HMAC_CTX* pHMACCtx = NULL;
#ifndef __ENABLE_MOCANA_CRYPTO_KERNEL_MODULE_FIPS__
    FILE*  fbin = NULL;
#else
    sbyte4 fbin = NULL;
#endif
    ubyte  data = 0;
#define BDATA_READ_SIZE 64
    ubyte  bdata[BDATA_READ_SIZE];
    sbyte4 bytesRead;


#if defined (__RTOS_WIN32__) || defined(__RTOS_WINCE__)

    if (optionalBinFileName)
    {
#ifndef __ENABLE_MOCANA_CRYPTO_KERNEL_MODULE_FIPS__
        fbin = fopen( optionalBinFileName, "rb");
#else
        fbin = MOC_CRYPTO_fipsSelfTestInit((ubyte *)optionalBinFileName);
#endif
    }
    else
    {
#ifndef __ENABLE_MOCANA_CRYPTO_KERNEL_MODULE_FIPS__
        TCHAR modulePath[_MAX_PATH+5]; /* add a little buffer to the buffer. 5 should be enough (.sig) */
        HMODULE hModule;

        hModule = GetModuleHandle(DLL_NAME);

        if (!GetModuleFileName( hModule, modulePath, _MAX_PATH))
        {
            DEBUG_PRINTNL( DEBUG_CRYPTO, (sbyte *) "FIPS_INTEG: GetModuleFileName failed");
            status = ERR_FILE_OPEN_FAILED;
            goto exit;
        }

        fbin = _tfopen(modulePath, _T("rb"));
#else
        fbin = MOC_CRYPTO_fipsSelfTestInit("\\SystemRoot\\system32\\drivers\\moc_crypto.sys");
#endif
    }
#elif defined(__RTOS_WINCE__)

    TCHAR modulePath[_MAX_PATH+5]; /* add a little buffer to the buffer. 5 should be enough (.sig) */
    HMODULE hModule;

    hModule = GetModuleHandle(_T("mss_ce_dll"));

    if (!GetModuleFileName( hModule, modulePath, _MAX_PATH))
    {
#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
        DEBUG_PRINTNL( DEBUG_CRYPTO, (sbyte *) "FIPS_INTEG: GetModuleFileName failed");
#endif
        status = ERR_FILE_OPEN_FAILED;
        goto exit;
    }

    fbin = _tfopen(modulePath, _T("rb"));
#else 

    if (optionalBinFileName)
    {
#ifndef __ENABLE_MOCANA_CRYPTO_KERNEL_MODULE_FIPS__
        fbin = fopen( optionalBinFileName, "rb");
#else
        fbin = MOC_CRYPTO_fipsSelfTestInit((ubyte *)optionalBinFileName);
#endif
    }
    else
    {
#ifndef __ENABLE_MOCANA_CRYPTO_KERNEL_MODULE_FIPS__
        fbin = fopen( FIPS_INTEG_TEST_BINARY_FILENAME, "rb");
#else
        fbin = MOC_CRYPTO_fipsSelfTestInit( FIPS_INTEG_TEST_BINARY_FILENAME);
#endif
    }
#endif


    if (0 == fbin)
    {
#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
        DEBUG_PRINTNL( DEBUG_CRYPTO, (sbyte *) "FIPS_INTEG: Binary file load error!" );
#endif
        status = ERR_FILE_OPEN_FAILED;
        goto exit;
    }

    if (OK > (status = HmacCreate( MOC_HASH(hwAccelCtx) &pHMACCtx, &SHA1Suite )))
        goto exit;

    if (OK > (status = HmacKey( MOC_HASH(hwAccelCtx) pHMACCtx, key, keyLen )))
        goto exit;

    MOC_MEMSET(bdata, 0x00, BDATA_READ_SIZE);

#ifndef __ENABLE_MOCANA_CRYPTO_KERNEL_MODULE_FIPS__
    /* while( EOF != (intVal = fgetc(fbin)) )*/
    while (0 < (bytesRead = (ubyte4)fread(bdata, 1, BDATA_READ_SIZE, fbin)))
#else
    while (0 < (bytesRead = MOC_CRYPTO_fipsSelfTestUpdate(fbin, bdata, BDATA_READ_SIZE)))
#endif
    {
        if (OK > (status = HmacUpdate( MOC_HASH(hwAccelCtx) pHMACCtx, bdata, bytesRead )))
            goto exit;
    }

    if (OK > (status = HmacFinal( MOC_HASH(hwAccelCtx) pHMACCtx, hashReturn )))
        goto exit;

exit:
    HmacDelete( MOC_HASH(sbyte4 hwAccelCtx) &pHMACCtx );

    if ( fbin)
    {
#ifndef __ENABLE_MOCANA_CRYPTO_KERNEL_MODULE_FIPS__
        fclose(fbin);
#else
        MOC_CRYPTO_fipsSelfTestFinal( fbin );
#endif
    }

    return status;
}

#endif  /* __ENABLE_MOCANA_FIPS_STATIC_INTEG_TEST__ */


/*------------------------------------------------------------------*/

#if defined(__FIPS_OPS_TEST__) && \
	defined(__ENABLE_MOCANA_FIPS_INTEG_TEST__) && \
	defined(__ENABLE_MOCANA_FIPS_STATIC_INTEG_TEST__)
/* this section places ascii tags into section to support the tampering test */
const unsigned int FIPS_OPS_TEST_CONST[] =
		/* "FIPS_OPS_TEST_DATA"; */
{
		0x53504946, 0x53504f5f, 0x5345545f, 0x41445f54, 0x00004154
//		FIPS        _OPS        _TES        T_DA         TA
};

#ifndef __RTOS_INTEGRITY__
void fips_ops_test_code()
{
    asm volatile  (  ".ascii \"FIPS_OPS_TEST_TEXT\" " );
}
#endif
#endif /* defined(__FIPS_OPS_TEST__) ... */

/*------------------------------------------------------------------*/

MOC_EXTERN MSTATUS
FIPS_INTEG_TEST(void)
{
    MSTATUS status = OK;
    int i;

    ubyte fileHash[SHA1_RESULT_SIZE];
    ubyte binHash[SHA1_RESULT_SIZE];
    sbyte4 cmpRes = 0;

#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
#ifdef __ENABLE_MOCANA_FIPS_STATUS_TIMING_MESSAGES__
    static volatile ubyte4 teststarttime, testendtime;
    char timebuf[40];
#endif /* __ENABLE_MOCANA_FIPS_STATUS_TIMING_MESSAGES__ */
    DEBUG_PRINTNL( DEBUG_CRYPTO, (sbyte *) "FIPS_INTEG: Testing..." );
#ifdef __ENABLE_MOCANA_FIPS_STATUS_TIMING_MESSAGES__
    teststarttime = RTOS_getUpTimeInMS();
    sprintf(timebuf,"%d",teststarttime);
    DEBUG_PRINT(DEBUG_CRYPTO, (sbyte *) "S-Time= ");
    DEBUG_PRINTNL(DEBUG_CRYPTO, (sbyte *)timebuf);
#endif /* __ENABLE_MOCANA_FIPS_STATUS_TIMING_MESSAGES__ */
#endif

#ifdef __ENABLE_MOCANA_FIPS_STATIC_INTEG_TEST__
    if (OK > (status = FIPS_INTEG_TEST_hash_locate( fileHash )))
        goto exit;

    if (OK > (status = FIPS_INTEG_TEST_hash_inplace( binHash )))
        goto exit;

#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
	for (i=0; i<SHA1_RESULT_SIZE; ++i) {
		fprintf(stderr, "%02X", binHash[i]);
	}
	fprintf(stderr, "\n");
#endif
#else
    if (OK > (status = FIPS_INTEG_TEST_hash_load( fileHash, sCurrRuntimeConfig.sigPath )))
        goto exit;

    if (OK > (status = FIPS_INTEG_TEST_hash_bin( binHash, sCurrRuntimeConfig.libPath )))
        goto exit;
#endif

    if (OK != MOC_MEMCMP( fileHash, binHash, SHA1_RESULT_SIZE, &cmpRes ))
    {
#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
        DEBUG_PRINTNL( DEBUG_CRYPTO, (sbyte *) "FIPS_INTEG: FAILED!" );
#endif
        status = ERR_FIPS_INTEGRITY_FAILED;
        goto exit;
    }

    if (0 != cmpRes)
    {
#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
        DEBUG_PRINTNL( DEBUG_CRYPTO, (sbyte *) "FIPS_INTEG: FAILED!" );
#endif
        status = ERR_FIPS_INTEGRITY_FAILED;
        goto exit;
    }

#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
    DEBUG_PRINTNL( DEBUG_CRYPTO, (sbyte *) "FIPS_INTEG:\t\t\tPASS" );
#endif

exit:
	sCurrStatus.integrityTestStatus = status;

#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
    DEBUG_PRINTNL( DEBUG_CRYPTO, (sbyte *) "FIPS_INTEG:\t\t\tFinished" );
#ifdef __ENABLE_MOCANA_FIPS_STATUS_TIMING_MESSAGES__
    testendtime = RTOS_getUpTimeInMS();
    sprintf(timebuf,"%d",testendtime);
    DEBUG_PRINT(DEBUG_CRYPTO, (sbyte *) "E-Time = ");
    DEBUG_PRINT(DEBUG_CRYPTO, (sbyte *)timebuf);
    DEBUG_PRINT(DEBUG_CRYPTO, (sbyte *) "  Elapsed (mil) = ");
    sprintf(timebuf,"%d",testendtime-teststarttime);
    DEBUG_PRINTNL(DEBUG_CRYPTO, (sbyte *)timebuf);
#endif /* __ENABLE_MOCANA_FIPS_STATUS_TIMING_MESSAGES__ */
#endif

    return status;
}
#endif /* defined(__ENABLE_MOCANA_FIPS_INTEG_TEST__) */


/*------------------------------------------------------------------*/
MOC_EXTERN MSTATUS FIPS_powerupSelfTestEx(FIPSRuntimeConfig *pfips_config)
{
    MSTATUS status = OK;
    randomContext*   pRandomContext = NULL;

#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
    DEBUG_PRINTNL(DEBUG_CRYPTO, (sbyte *) "FIPS_powerupSelfTestEx: Started...");
#endif

#if defined(__FIPS_OPS_TEST__) &&  defined(__ENABLE_MOCANA_FIPS_INTEG_TEST__) && \
	defined(__ENABLE_MOCANA_FIPS_STATIC_INTEG_TEST__) &&  defined(__ENABLE_MOCANA_FIPS_STATUS_MESSAGES__)
    DEBUG_PRINTNL(DEBUG_CRYPTO, (sbyte *) FIPS_OPS_TEST_CONST);
#endif

    /* Nominally validate and copy the provided config */
    if (OK != (status = FIPS_validateAndCopyConfig(pfips_config)))
    {
#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
    	DEBUG_PRINTNL(DEBUG_CRYPTO, (sbyte *) "FIPS_powerupSelfTestEx: Bad configuration.");
#endif
    	FIPS_ZeroStartupStatus(&sCurrRuntimeConfig);
    	goto exit;
    }

	FIPS_ZeroStartupStatus(&sCurrRuntimeConfig);

    FIPS_MarkAllAlgosStartupTBD(&sCurrRuntimeConfig);
    sCurrStatus.startupState = FIPS_SS_INPROCESS;

    /* FORCE OK so we can run HMAC SHA1 to do integrity check */
    sCurrStatus.globalFIPS_powerupStatus = OK;

    /* Always do the integrity check */
#ifdef __ENABLE_MOCANA_FIPS_INTEG_TEST__
    if (OK > (status = FIPS_INTEG_TEST()))
        goto exit;

#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
    DEBUG_PRINTNL(DEBUG_CRYPTO, (sbyte *) "FIPS_IntegTest: Done...");
#endif
#endif

    FIPS_MarkAllAlgosStartupTBD(&sCurrRuntimeConfig);

    /* See if we need to do KnownAnswer and Pairwise tests */
    if (FALSE == FIPS_StartupTests_ShouldBeRun())
    {
#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
    	DEBUG_PRINTNL(DEBUG_CRYPTO, (sbyte *) "FIPS_powerupSelfTest: Has already completed.");
#endif
        goto exit;
    }

#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
    DEBUG_PRINTNL(DEBUG_CRYPTO, (sbyte *) "FIPS_powerupSelfTest(void): Calling FIPS_knownAnswerTests().");
#endif
    if (OK > (status = FIPS_knownAnswerTests()))
        goto exit;

#ifndef __ENABLE_MOCANA_CRYPTO_KERNEL_MODULE_FIPS__
#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
    DEBUG_PRINTNL(DEBUG_CRYPTO, (sbyte *) "FIPS_powerupSelfTest(void): Calling RANDOM_acquireContext().");
#endif
    /* Setup a random number generator for all tests to use */
    if (OK > (status = RANDOM_setEntropySource(ENTROPY_SRC_EXTERNAL)))
    	goto exit;

    if (OK > (status = RANDOM_acquireContext(&pRandomContext)))
        goto exit;

    if (sCurrRuntimeConfig.useInternalEntropy)
    {
#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
    DEBUG_PRINTNL(DEBUG_CRYPTO, (sbyte *) "FIPS_powerupSelfTest(void): Internal Entropy Set.");
#endif
    	RANDOM_setEntropySource(ENTROPY_SRC_INTERNAL);
    }
    else
    {
#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
    DEBUG_PRINTNL(DEBUG_CRYPTO, (sbyte *) "FIPS_powerupSelfTest(void): External Entropy Set.");
#endif
    	RANDOM_setEntropySource(ENTROPY_SRC_EXTERNAL);
    }

#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
    DEBUG_PRINTNL(DEBUG_CRYPTO, (sbyte *) "FIPS_powerupSelfTest(void): Calling FIPS_pairwiseConsistencyTests().");
#endif
    if (OK > (status = FIPS_pairwiseConsistencyTests(pRandomContext)))
        goto exit;
#endif

exit:

    /* Global status is set to powerup tests status */
#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
    DEBUG_PRINTNL(DEBUG_CRYPTO, (sbyte *) "FIPS_powerupSelfTest(void): Calling FIPS_StartupTests_SaveResults().");
#endif
	FIPS_StartupTests_SaveResults(status);

#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
    DEBUG_PRINTNL(DEBUG_CRYPTO, (sbyte *) "FIPS_powerupSelfTest(void): exit: Calling FIPS_DumpStartupStatusData().");
#endif
    FIPS_DumpStartupStatusData();

#ifdef __ENABLE_MOCANA_DEBUG_MEMORY__
    RTOS_sleepMS(5000);
    dbg_dump();
#endif

#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
    DEBUG_PRINTNL(DEBUG_CRYPTO, (sbyte *) "FIPS_powerupSelfTestEx: Finished");
#endif

	RANDOM_releaseContext(&pRandomContext);
    return status;
}


/* Main FIPS Tests */
MOC_EXTERN MSTATUS
FIPS_powerupSelfTest(void)
{
    MSTATUS status = OK;
    FIPSRuntimeConfig *pdef_fips_config = NULL;

#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
    DEBUG_PRINTNL(DEBUG_CRYPTO, (sbyte *) "FIPS_powerupSelfTest: Started...");
#endif

    pdef_fips_config = MALLOC(sizeof(FIPSRuntimeConfig));

    /* If no config was passed in, get the default (which enables all compiled in algos) */
    FIPS_getDefaultConfig(pdef_fips_config);

    status = FIPS_powerupSelfTestEx(pdef_fips_config);

    FREE(pdef_fips_config);

#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
    DEBUG_PRINTNL(DEBUG_CRYPTO, (sbyte *) "FIPS_powerupSelfTest: Finished");
#endif

    return status;
}

#ifdef __ENABLE_MOCANA_FIPS_LIB_CONSTRUCTOR__ 

static void FIPS_constructor() __attribute__((constructor));

void FIPS_constructor() 
{
#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
    DEBUG_PRINTNL(DEBUG_CRYPTO, (sbyte *) "FIPS Constructor called.");
#endif
#ifndef __DISABLE_MOCANA_FIPS_CONSTRUCTOR_SELFTEST__
    FIPS_powerupSelfTest();
#endif

}


static void FIPS_destructor() __attribute__((destructor));

void FIPS_destructor() 
{
#ifdef __ENABLE_MOCANA_FIPS_STATUS_MESSAGES__
    DEBUG_PRINTNL(DEBUG_CRYPTO, (sbyte *) "FIPS Destructor Finished.");
#endif
    // KRB:: Not sure whether to do this or not... Needs to be pondered...
    // KRB:: FIPS_ZeroStartupStatus(&sCurrRuntimeConfig);

}

#endif /* __ENABLE_MOCANA_FIPS_LIB_CONSTRUCTOR__ */


#endif /* __ENABLE_MOCANA_FIPS_MODULE__ */

