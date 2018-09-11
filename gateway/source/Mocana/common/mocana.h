/* Version: mss_v6_3 */
/*
 * mocana.h
 *
 * Mocana Initialization Header
 *
 * Copyright Mocana Corp 2003-2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

/*! \file mocana.h Mocana initialization header.
This header file contains enumerations and function declarations used by all
Mocana components.

\since 1.41
\version 2.02 and later

! Flags
Whether the following flags are defined determines which function declarations are enabled:
- $__DISABLE_MOCANA_ADD_ENTROPY__$
- $__DISABLE_MOCANA_FILE_SYSTEM_HELPER__$
- $__DISABLE_MOCANA_INIT__$

*/


/*------------------------------------------------------------------*/

#ifndef __MOCANA_HEADER__
#define __MOCANA_HEADER__

#ifdef __cplusplus
extern "C" {
#endif

enum moduleNames
{
    MOCANA_MSS,
    MOCANA_SSH,
    MOCANA_SSL,
    MOCANA_SCEP,
    MOCANA_IPSEC,
    MOCANA_IKE,
    MOCANA_RADIUS,
    MOCANA_EAP,
    MOCANA_HTTP,
    MOCANA_TEST,
    MOCANA_FIREWALL,
    MOCANA_OCSP,
    MOCANA_SRTP,
    MOCANA_SYSLOG,
    MOCANA_END
};

enum logSeverity
{
    LS_CRITICAL,
    LS_MAJOR,
    LS_MINOR,
    LS_WARNING,
    LS_INFO
};

enum versionType
{
	VT_MAIN = 0x01,
	VT_BUILD = 0x02,
	VT_TIMESTAMP = 0x04,
	VT_ALL = 0xFF
};
/*
\exclude
*/
typedef void (*logFn)(sbyte4 module, sbyte4 severity, sbyte *msg);

/*!
\exclude
*/
MOC_EXTERN volatile sbyte4 gMocanaAppsRunning;


/*------------------------------------------------------------------*/

#ifndef __DISABLE_MOCANA_INIT__
MOC_EXTERN sbyte4  MOCANA_initMocana(void);
MOC_EXTERN sbyte4  MOCANA_freeMocana(void);
#endif

MOC_EXTERN sbyte4 MOCANA_readVersion(sbyte4 type, ubyte *pRetBuffer, ubyte4 retBufLength);

MOC_EXTERN sbyte4  MOCANA_initLog(logFn lf);
MOC_EXTERN void MOCANA_log(sbyte4 module, sbyte4 severity, sbyte *msg);

#ifndef __DISABLE_MOCANA_ADD_ENTROPY__
MOC_EXTERN sbyte4  MOCANA_addEntropyBit(ubyte entropyBit);
MOC_EXTERN sbyte4  MOCANA_addEntropy32Bits(ubyte4 entropyBits);
#endif

#ifndef __DISABLE_MOCANA_FILE_SYSTEM_HELPER__
MOC_EXTERN sbyte4 MOCANA_readFile(const char* pFilename, ubyte **ppRetBuffer, ubyte4 *pRetBufLength);
MOC_EXTERN sbyte4 MOCANA_freeReadFile(ubyte **ppRetBuffer);
MOC_EXTERN sbyte4 MOCANA_writeFile(const char* pFilename, ubyte *pBuffer, ubyte4 bufLength);
#endif

#ifdef __cplusplus
}
#endif

#endif /* __MOCANA_HEADER__ */
