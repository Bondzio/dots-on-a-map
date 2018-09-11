/* Version: mss_v6_3 */
/* Version: mss_v5_4_18021 */
/*
 * mocana.c
 *
 * Mocana Initialization
 *
 * Copyright Mocana Corp 2003-2008. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

/*! \file mocana.c Mocana initialization.
This file contains functions used by all Mocana components.

\since 1.41
\version 3.06 and later

! Flags
Whether the following flags are defined determines which additional header files
are included:
- $__ENABLE_MOCANA_IKE_SERVER__$
- $__ENABLE_MOCANA_PEM_CONVERSION__$
- $__ENABLE_MOCANA_PKCS10__$
- $__ENABLE_MOCANA_RADIUS_CLIENT__$
- $__ENABLE_MOCANA_SSH_SERVER__$

! External Functions
This file contains the following public ($extern$) functions:
- MOCANA_addEntropy32Bits
- MOCANA_addEntropyBit
- MOCANA_freeMocana
- MOCANA_freeReadFile
- MOCANA_initLog
- MOCANA_initMocana
- MOCANA_readFile
- MOCANA_writeFile

*/

#define __IN_MOCANA_C__

#include "../common/moptions.h"
#include "../common/mdefs.h"
#include "../common/mtypes.h"
#include "../common/merrors.h"
#include "../common/mrtos.h"
#include "../common/mstdlib.h"
#if (!defined(__DISABLE_MOCANA_TCP_INTERFACE__))
#include "../common/mtcp.h"
#endif
#include "../common/utils.h"
#include "../common/mocana.h"
#include "../common/random.h"
#include "../common/debug_console.h"
#if (defined(__ENABLE_MOCANA_MEM_PART__))
#include "../common/mem_part.h"
#endif
#include "../crypto/hw_accel.h"
#if (defined(__ENABLE_MOCANA_HARNESS__))
#include "../harness/harness.h"
#endif

#if (defined(__ENABLE_MOCANA_RADIUS_CLIENT__) || defined(__ENABLE_MOCANA_IKE_SERVER__)) && \
    !(defined(__KERNEL__) || defined(_KERNEL) || defined(IPCOM_KERNEL))
#include "../common/mudp.h"
#endif

#if (defined(__ENABLE_MOCANA_SSH_SERVER__) || defined(__ENABLE_MOCANA_PKCS10__) || defined(__ENABLE_MOCANA_PEM_CONVERSION__))
#include "../crypto/base64.h"
#endif

/*------------------------------------------------------------------*/

/*!
\exclude
*/
logFn            g_logFn          = NULL;

/*!
\exclude
*/
volatile sbyte4 gMocanaAppsRunning = 0;
/*!
\exclude
*/

moctime_t gStartTime;


#ifndef __DISABLE_MOCANA_INIT__
/*------------------------------------------------------------------*/

/*! Initialize Mocana %common code base.
This function initializes the Mocana %common code base; it is typically the
first initialization step for any Mocana Security Suite product.

\since 1.41
\version 3.06 and later

! Flags
To enable this function, the following flag must #not# be defined:
- $__DISABLE_MOCANA_INIT__$

Additionally, whether or not the following flags are defined determines which
initialization functions are called:
- $__DISABLE_MOCANA_STARTUP_GUARD__$
- $__ENABLE_MOCANA_DTLS_CLIENT__$
- $__ENABLE_MOCANA_DTLS_SERVER__$
- $__ENABLE_MOCANA_IKE_SERVER__$
- $__ENABLE_MOCANA_PEM_CONVERSION__$
- $__ENABLE_MOCANA_PKCS10__$
- $__ENABLE_MOCANA_RADIUS_CLIENT__$
- $__ENABLE_MOCANA_SSH_SERVER__$
- $__KERNEL__$
- $UDP_init$

#Include %file:#&nbsp;&nbsp;mocana.h

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error status, use the
$DISPLAY_ERROR$ macro.

\example
sbyte4 status = 0;
status = MOCANA_initMocana();
\endexample
*/
extern sbyte4
MOCANA_initMocana(void)
{
#ifndef __DISABLE_MOCANA_STARTUP_GUARD__
    static  sbyte4  mocanaStarted = 0;
#endif
    MSTATUS         status = OK;

#ifndef __DISABLE_MOCANA_STARTUP_GUARD__
    if (1 != (++mocanaStarted))
        goto exit;
#endif

#if !(defined(__KERNEL__) || defined(_KERNEL) || defined(IPCOM_KERNEL))
    if (OK > (status = RTOS_rtosInit()))
        goto exit;
#if (!defined(__DISABLE_MOCANA_TCP_INTERFACE__))
    if (OK > (status = TCP_INIT()))
        goto exit;
#endif /*__DISABLE_MOCANA_TCP_INTERFACE__*/
#endif

    RTOS_deltaMS(NULL, &gStartTime);

#if (defined(UDP_init) && (defined(__ENABLE_MOCANA_RADIUS_CLIENT__) || defined(__ENABLE_MOCANA_IKE_SERVER__) ||  defined(__ENABLE_MOCANA_DTLS_CLIENT__) ||  defined(__ENABLE_MOCANA_DTLS_SERVER__) ))
#if !(defined(__KERNEL__) || defined(_KERNEL) || defined(IPCOM_KERNEL))
    if (OK > (status = UDP_init()))
        goto exit;
#endif
#endif

#ifdef __ENABLE_MOCANA_DEBUG_CONSOLE__
#if !(defined(__KERNEL__) || defined(_KERNEL) || defined(IPCOM_KERNEL))
    DEBUG_CONSOLE_init();
#endif
#ifndef __MOCANA_DUMP_CONSOLE_TO_STDOUT__
    DEBUG_CONSOLE_start(MOCANA_DEBUG_CONSOLE_PORT);
#endif
#endif

#if (defined(__ENABLE_MOCANA_SSH_SERVER__) || defined(__ENABLE_MOCANA_PKCS10__) || defined(__ENABLE_MOCANA_PEM_CONVERSION__))
    if (OK > (status = BASE64_initializeContext()))
        goto exit;
#endif

#ifdef __ENABLE_MOCANA_MEM_PART__
    if (OK > (status = MEM_PART_init()))
        goto exit;
#endif

    if (OK > (status = (MSTATUS) HARDWARE_ACCEL_INIT()))
        goto exit;

    if (NULL == g_pRandomContext)
    {
        if (OK > (status = RANDOM_acquireGlobalContext(&g_pRandomContext)))
            goto exit;
    }

exit:
    return (sbyte4)status;
}
#endif /* __DISABLE_MOCANA_INIT__ */


/*------------------------------------------------------------------*/

#ifndef __DISABLE_MOCANA_INIT__
/*! Release memory allocated by MOCANA_initMocana.
This function releases memory previously allocated by a call to MOCANA_initMocana.

\since 1.41
\version 3.06 and later

! Flags
To enable this function, the following flag must #not# be defined:
- $__DISABLE_MOCANA_INIT__$

#Include %file:#&nbsp;&nbsp;mocana.h

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error status, use the
$DISPLAY_ERROR$ macro.

\example
int status = 0;
status = MOCANA_freeMocana();
\endexample
*/
extern sbyte4
MOCANA_freeMocana(void)
{
    RANDOM_releaseGlobalContext(&g_pRandomContext);

#if (defined(__ENABLE_MOCANA_SSH_SERVER__) || defined(__ENABLE_MOCANA_PKCS10__) || defined(__ENABLE_MOCANA_PEM_CONVERSION__))
    BASE64_freeContext();
#endif

#if !(defined(__KERNEL__) || defined(_KERNEL) || defined(IPCOM_KERNEL))
#if (defined(UDP_shutdown) && (defined(__ENABLE_MOCANA_RADIUS_CLIENT__) || defined(__ENABLE_MOCANA_IKE_SERVER__) ||  defined(__ENABLE_MOCANA_DTLS_CLIENT__) ||  defined(__ENABLE_MOCANA_DTLS_SERVER__) ))
    UDP_shutdown();
#endif
#if (!defined(__DISABLE_MOCANA_TCP_INTERFACE__))
    TCP_SHUTDOWN();
#endif /*__DISABLE_MOCANA_TCP_INTERFACE__*/
    RTOS_rtosShutdown();
#endif

    (void)HARDWARE_ACCEL_UNINIT();

#ifdef __ENABLE_MOCANA_MEM_PART__
    MEM_PART_uninit();
#endif

#if !(defined(__KERNEL__) || defined(_KERNEL) || defined(IPCOM_KERNEL))
#ifdef __ENABLE_MOCANA_DEBUG_CONSOLE__
    DEBUG_CONSOLE_stop();
#endif
#endif

    return (sbyte4)OK;
}
#endif /* __DISABLE_MOCANA_INIT__ */


/*------------------------------------------------------------------*/

#ifndef __DISABLE_MOCANA_ADD_ENTROPY__
/*! Add a random bit to application's random number generator.
This function adds a random bit to your application's random number generator.
Before calling this function, your application should have already initialized
the Mocana %common code base by calling MOCANA_initMocana.

\since 1.41
\version 1.41 and later

! Flags
To enable this function, the following flag must #not# be defined:
- $__DISABLE_MOCANA_ADD_ENTROPY__$

#Include %file:#&nbsp;&nbsp;mocana.h

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error status, use the
$DISPLAY_ERROR$ macro.

\param entropyBit 1-bit $char$ used to add randomness to your application's cryptography.

\example
sbyte4 status = 0;
ubyte ebit;
status = MOCANA_addEntropyBit(ebit);
\endexample
*/
extern sbyte4
MOCANA_addEntropyBit(ubyte entropyBit)
{
    return (sbyte4)RANDOM_addEntropyBit(g_pRandomContext, (ubyte)entropyBit);
}
#endif /* __DISABLE_MOCANA_ADD_ENTROPY__ */


/*------------------------------------------------------------------*/

#ifndef __DISABLE_MOCANA_ADD_ENTROPY__
/*! Add 32 random bits to application's random number generator.
This function adds 32 random bits to your application's random number generator.
Before calling this function, your application should have already initialized
the Mocana %common code base by calling MOCANA_initMocana.

\since 1.41
\version 1.41 and later

! Flags
To enable this function, the following flag must #not# be defined:
- $__DISABLE_MOCANA_ADD_ENTROPY__$

#Include %file:#&nbsp;&nbsp;mocana.h

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error status, use the
$DISPLAY_ERROR$ macro.

\param entropyBits 32-bit $integer$ used to add randomness to your application's cryptography.

\example
sbyte4 status = 0;
ubyte4 ebit;
status = MOCANA_addEntropy32Bits(ebit);
\endexample
*/
extern sbyte4
MOCANA_addEntropy32Bits(ubyte4 entropyBits)
{
    ubyte4  count;
    MSTATUS status = OK;

    for (count = 32; count > 0; count--)
    {
        if (OK > (status = RANDOM_addEntropyBit(g_pRandomContext, (ubyte)(entropyBits & 1))))
            break;

        entropyBits >>= 1;
    }

    return (sbyte4)status;
}
#endif /* __DISABLE_MOCANA_ADD_ENTROPY__ */


/*------------------------------------------------------------------*/

#ifndef __DISABLE_MOCANA_FILE_SYSTEM_HELPER__
/*! Allocate a buffer and fill with data read from a file.
This function allocates a buffer and then fills it with data read from a file.

\since 1.41
\version 1.41 and later

! Flags
To enable this function, the following flag must #not# be defined:
- $__DISABLE_MOCANA_FILE_SYSTEM_HELPER__$

#Include %file:#&nbsp;&nbsp;mocana.h

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error status, use the
$DISPLAY_ERROR$ macro.

\param pFilename Name of the file from which to read.
\param ppRetBuffer Reference to the pointer to a data buffer containing the file.
\param pRetBufLength Reference to length of the data buffer in bytes.

\example
sbyte4 status;
ubyte *pCertificate = NULL;
ubyte4 retCertLength = 0;

if (0 > (status = MOCANA_readFile(CERTIFICATE_DER_FILE, &pCertificate, &retCertLength)))
    goto exit;
\endexample

\note To avoid memory leaks, be sure to make a subsequent call to MOCANA_freeReadFile
\remark This is a convenience function provided for your application's use; it is not used by Mocana internal code.
*/
extern sbyte4
MOCANA_readFile(const char* pFilename, ubyte **ppRetBuffer, ubyte4 *pRetBufLength)
{
    return (sbyte4)UTILS_readFile(pFilename, ppRetBuffer, pRetBufLength);
}
#endif /* __DISABLE_MOCANA_FILE_SYSTEM_HELPER__ */


/*------------------------------------------------------------------*/

#ifndef __DISABLE_MOCANA_FILE_SYSTEM_HELPER__
/*! Release memory allocated by MOCANA_readFile.
This function releases memory previously allocated by a call to MOCANA_readFile.

\since 1.41
\version 1.41 and later

! Flags
To enable this function, the following flag must #not# be defined:
- $__DISABLE_MOCANA_FILE_SYSTEM_HELPER__$

#Include %file:#&nbsp;&nbsp;mocana.h

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error status, use the
$DISPLAY_ERROR$ macro.

\param ppRetBuffer Reference to the data buffer to free.

\example
ubyte *pCertificate;
...
status = MOCANA_freeReadFile(&pCertificate);
\endexample

\remark This is a convenience function provided for your application's use; it is not used by Mocana internal code.
*/
extern sbyte4
MOCANA_freeReadFile(ubyte **ppRetBuffer)
{
    return (sbyte4)UTILS_freeReadFile(ppRetBuffer);
}
#endif /* __DISABLE_MOCANA_FILE_SYSTEM_HELPER__ */


/*------------------------------------------------------------------*/

#ifndef __DISABLE_MOCANA_FILE_SYSTEM_HELPER__
/*! Write a buffer's contents to a file.
This function writes a data buffer's contents to a file.

\since 1.41
\version 1.41 and later

! Flags
To enable this function, the following flag must #not# be defined:
- $__DISABLE_MOCANA_FILE_SYSTEM_HELPER__$

#Include %file:#&nbsp;&nbsp;mocana.h

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error status, use the
$DISPLAY_ERROR$ macro.

\param pFilename    Pointer to name of the file to write to.
\param pBuffer      Pointer to buffer containing data to write to the file.
\param bufLength    Number of bytes in $pBuffer$.

\example
sbyte4 status = 0;
status = MOCANA_writeFile(CERTIFICATE_DER_FILE, pCertificate, retCertLength);
\endexample

\remark This is a convenience function provided for your application's use; it is not used by Mocana internal code.
*/
extern sbyte4
MOCANA_writeFile(const char* pFilename, ubyte *pBuffer, ubyte4 bufLength)
{
    return (sbyte4)UTILS_writeFile(pFilename, pBuffer, bufLength);
}
#endif /* __DISABLE_MOCANA_FILE_SYSTEM_HELPER__ */


/*------------------------------------------------------------------*/
/* Return the version string as requested by the 'type' parameter.

\since 5.4.2
\version 5.4.2 and later

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error status, use the
$DISPLAY_ERROR$ macro.

\param type     The enum value describing the requested version type.

\param pBuffer Pointer to memory, that will be overwritten by this method. The
               character array stored in the memory is the version string.
               The string will be terminated by '\0'.

\param bufLength The number of characters that can be stored in the above area.
               This method return an error code if the buffer is not large enough.

 */

extern sbyte4
MOCANA_readVersion(sbyte4 type, ubyte* pRetBuffer, ubyte4 retBufLength)
{
	MSTATUS status = OK;
	int total = 0;
	char add = FALSE;

#ifndef __MOCANA_DSF_BUILD_STR__
#define __MOCANA_DSF_BUILD_STR__ "(unknown)"
#endif

#ifndef __MOCANA_DSF_BUILDTIME_STR__
#define __MOCANA_DSF_BUILDTIME_STR__ "(unknown)"
#endif


	if ((type & VT_MAIN) != 0)
	{
		total = MOC_STRLEN((sbyte*)__MOCANA_DSF_VERSION_STR__) + 1;
	}

	if ((type & VT_BUILD) != 0)
	{
		total += MOC_STRLEN((sbyte*)__MOCANA_DSF_BUILD_STR__) + 1;
	}

	if ((type & VT_TIMESTAMP) != 0)
	{
		total += MOC_STRLEN((sbyte*)__MOCANA_DSF_BUILDTIME_STR__) + 1;
	}

	if (total > 0)
	{
		if (total >= retBufLength)
		{
			return ERR_BUFFER_OVERFLOW;
		}


		if ((type & VT_MAIN) != 0)
		{
			MOC_STRCBCPY((sbyte*)pRetBuffer, total,(sbyte*)__MOCANA_DSF_VERSION_STR__);
			add = TRUE;
		}

		if ((type & VT_BUILD) != 0)
		{
			if (add)
			{
				MOC_STRCAT((sbyte*)pRetBuffer, (sbyte*)" ");
				MOC_STRCAT((sbyte*)pRetBuffer, (sbyte*)__MOCANA_DSF_BUILD_STR__);
			}
			else
			{
				MOC_STRCBCPY((sbyte*)pRetBuffer, total, (sbyte*)__MOCANA_DSF_BUILD_STR__);
			}
			add = TRUE;
		}

		if ((type & VT_TIMESTAMP) != 0)
		{
			if (add)
			{
				MOC_STRCAT((sbyte*)pRetBuffer, (sbyte*)" ");
				MOC_STRCAT((sbyte*)pRetBuffer, (sbyte*)__MOCANA_DSF_BUILDTIME_STR__);
			}
			else
			{
				MOC_STRCBCPY((sbyte*)pRetBuffer, total, (sbyte*)__MOCANA_DSF_BUILDTIME_STR__);
			}
			add = TRUE;
		}
	}
	else
	{
		/* No good type parameter */
		return ERR_INVALID_ARG;
	}

	return status;
}

/*------------------------------------------------------------------*/

/* Register a callback function for the Mocana logging system.
This function registers a callback function for the Mocana logging system.

\since 1.41
\version 1.41 and later

! Flags
To enable this function, the following flag must #not# be defined:
- $__DISABLE_MOCANA_FILE_SYSTEM_HELPER__$

#Include %file:#&nbsp;&nbsp;mocana.h

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error status, use the
$DISPLAY_ERROR$ macro.

\param lFn  Callback function that you want to receive notification of Mocana
logging events.

\example
sbyte4 status = 0;
status = MOCANA_initLog(myEventHandler);
\endexample

\remark This is a convenience function provided for your application's use; it is not used by Mocana internal code.

*/
extern sbyte4
MOCANA_initLog(logFn lFn)
{
    /* fine to set callback to null, if you wish to disable logging */
    g_logFn = lFn;

#ifdef __ENABLE_MOCANA_DEBUG_CONSOLE__
    MOCANA_log(MOCANA_MSS, LS_WARNING, (sbyte *)"NOT A PRODUCTION BUILD: MOCANA DEBUG CONSOLE HAS BEEN ENABLED.");
#endif

    return (sbyte4)OK;
}


/*------------------------------------------------------------------*/

/* Doc Note: This function is for Mocana internal code use only, and should not
be included in the API documentation.
*/
extern void
MOCANA_log(sbyte4 module, sbyte4 severity, sbyte *msg)
{
    if ((NULL != g_logFn) && (NULL != msg))
        (*g_logFn)(module, severity, msg);
}


