/* Version: mss_v6_3 */
/*
 * base64.h
 *
 * Base64 Encoder & Decoder Header
 *
 * Copyright Mocana Corp 2003-2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */


/*------------------------------------------------------------------*/

#ifndef __MOCANA_BASE64_HEADER__
#define __MOCANA_BASE64_HEADER__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __IN_MOCANA_C__
/* these two functions should only be called from mocana.c */
MOC_EXTERN MSTATUS BASE64_initializeContext(void);
MOC_EXTERN MSTATUS BASE64_freeContext(void);
#endif

MOC_EXTERN MSTATUS BASE64_encodeMessage(const ubyte *pOrigMesg, ubyte4 origLen, ubyte **ppRetMesg, ubyte4 *pRetMesgLen);
MOC_EXTERN MSTATUS BASE64_decodeMessage(const ubyte *pOrigMesg, ubyte4 origLen, ubyte **ppRetMesg, ubyte4 *pRetMesgLen);
MOC_EXTERN MSTATUS BASE64_freeMessage(ubyte **ppMessage);

    
#ifdef __cplusplus
}
#endif
    
#endif /* __MOCANA_BASE64_HEADER__ */
