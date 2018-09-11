/* Version: mss_v6_3 */
/*
 * pkcs8.h
 *
 * PKCS #8 Header
 *
 * Copyright Mocana Corp 2006-2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

#ifndef __PKCS8_HEADER__
#define __PKCS8_HEADER__

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------------------------------------------------*/

#ifdef __ENABLE_MOCANA_PKCS8__
MOC_EXTERN MSTATUS PKCS8_decodePrivateKeyPEM(const ubyte* pFilePemPkcs8, ubyte4 fileSizePemPkcs8, ubyte** ppRsaKeyBlob, ubyte4 *pRsaKeyBlobLength);
MOC_EXTERN MSTATUS PKCS8_decodePrivateKeyDER(const ubyte* pFileDerPkcs8, ubyte4 fileSizeDerPkcs8, ubyte** ppRsaKeyBlob, ubyte4 *pRsaKeyBlobLength);
#endif

#ifdef __cplusplus
}
#endif
    
#endif /* __PKCS8_HEADER__*/

