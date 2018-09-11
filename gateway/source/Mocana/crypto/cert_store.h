/* Version: mss_v6_3 */
/*
 * cert_store.h
 *
 * Certificate Store Header
 *
 * Copyright Mocana Corp 2006-2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

/*! \file cert_store.h Certificate store factory.
This header file contains structures, enumerations, and function declarations
used for certificate stores.

\since 1.41
\version 2.02 and later

! Flags
No flag definitions are required to use this file.

*/

#ifndef __CERT_STORE_HEADER__
#define __CERT_STORE_HEADER__

#ifdef __cplusplus
extern "C" {
#endif


/*------------------------------------------------------------------*/

#ifndef MAX_SIZE_CERT_STORE_TRUST_HASH_TABLE
#define MAX_SIZE_CERT_STORE_TRUST_HASH_TABLE        (0x1f)
#endif

/* left first and second octet are reserved */
#define CERT_STORE_ALGO_FLAG_RESERVED               (0xFF000000L)
/* left third and fourth octet are for EC curves */
#define CERT_STORE_ALGO_FLAG_ECCURVES               (0x00FF0000L)
/* left fifth and sixth octets are for sign keyType */
#define CERT_STORE_ALGO_FLAG_SIGNKEYTYPE            (0x0000FF00L)
/* right most two octets are for HASH algorithms */
#define CERT_STORE_ALGO_FLAG_HASHALGO               (0x000000FFL)

/* individual flags: ec curves */
#define CERT_STORE_ALGO_FLAG_EC192                  (0x00010000L)
#define CERT_STORE_ALGO_FLAG_EC224                  (0x00020000L)
#define CERT_STORE_ALGO_FLAG_EC256                  (0x00040000L)
#define CERT_STORE_ALGO_FLAG_EC384                  (0x00080000L)
#define CERT_STORE_ALGO_FLAG_EC521                  (0x00100000L)

/* individual flags: signing keyType */
#define CERT_STORE_ALGO_FLAG_RSA                    (0x00000100L)
#define CERT_STORE_ALGO_FLAG_ECDSA                  (0x00000200L)
#define CERT_STORE_ALGO_FLAG_DSA                    (0x00000400L)

/* individual flags: hash algos */
#define CERT_STORE_ALGO_FLAG_MD5                    (0x00000001L)
#define CERT_STORE_ALGO_FLAG_SHA1                   (0x00000002L)
#define CERT_STORE_ALGO_FLAG_SHA224                 (0x00000004L)
#define CERT_STORE_ALGO_FLAG_SHA256                 (0x00000008L)
#define CERT_STORE_ALGO_FLAG_SHA384                 (0x00000010L)
#define CERT_STORE_ALGO_FLAG_SHA512                 (0x00000020L)

/*------------------------------------------------------------------*/

enum authTypes
{
    CERT_STORE_AUTH_TYPE_RSA                        = 0,
    CERT_STORE_AUTH_TYPE_ECDSA                      = 1,
    CERT_STORE_AUTH_TYPE_DSA                        = 2,
    CERT_STORE_AUTH_TYPE_ARRAY_SIZE                 = 3     /* needs to be last */
};

enum identityTypes
{
    CERT_STORE_IDENTITY_TYPE_NAKED                  = 0,
    CERT_STORE_IDENTITY_TYPE_CERT_X509_V3           = 1,
    CERT_STORE_IDENTITY_TYPE_ARRAY_SIZE             = 2     /* needs to be last */
};

/*------------------------------------------------------------------*/

struct AsymmetricKey;
struct certStore;
typedef struct certStore *certStorePtr;


/*------------------------------------------------------------------*/

MOC_EXTERN MSTATUS CERT_STORE_createStore(certStorePtr *ppNewStore);
MOC_EXTERN MSTATUS CERT_STORE_releaseStore(certStorePtr *ppReleaseStore);

/* add identity */
MOC_EXTERN MSTATUS CERT_STORE_addIdentity(certStorePtr pCertStore, const ubyte *pDerCert, ubyte4 derCertLength, const ubyte *pKeyBlob, ubyte4 keyBlobLength);
MOC_EXTERN MSTATUS CERT_STORE_addIdentityWithCertificateChain(certStorePtr pCertStore, SizedBuffer *certificates, ubyte4 numCertificate, const ubyte *pKeyBlob, ubyte4 keyBlobLength);
MOC_EXTERN MSTATUS CERT_STORE_addIdentityNakedKey(certStorePtr pCertStore, const ubyte *pKeyBlob, ubyte4 keyBlobLength);
MOC_EXTERN MSTATUS CERT_STORE_addIdentityPSK(certStorePtr pCertStore, const ubyte *pPskIdentity, ubyte4 pskIdentityLength, const ubyte *pPskHint, ubyte4 pskHintLength, const ubyte *pPskSecret, ubyte4 pskSecretLength);

/* add trust point to hash table */
MOC_EXTERN MSTATUS CERT_STORE_addCertAuthority(certStorePtr pCertStore, const ubyte *pDerCert, ubyte4 derCertLength);
MOC_EXTERN MSTATUS CERT_STORE_addTrustPoint(certStorePtr pCertStore, const ubyte *pDerTrustPoint, ubyte4 derTrustPointLength);

/* lookup CA, RA, trustpoints in hash table */
MOC_EXTERN MSTATUS CERT_STORE_findCertBySubject(const certStorePtr pCertStore, ubyte *pSubject, ubyte4 subjectLength, ubyte **ppRetDerCert, ubyte4 *pRetDerCertLength);

/* find an identity certificate by type */
MOC_EXTERN MSTATUS CERT_STORE_findIdentityByTypeFirst(const certStorePtr pCertStore, enum authTypes authType, enum identityTypes identityType, struct AsymmetricKey** ppRetIdentityKey, ubyte **ppRetDerCert, ubyte4 *pRetDerCertLength, void** ppRetHint);
MOC_EXTERN MSTATUS CERT_STORE_findIdentityByTypeNext(const certStorePtr pCertStore, enum authTypes authType, enum identityTypes identityType, struct AsymmetricKey** ppRetIdentityKey, ubyte **ppRetDerCert, ubyte4 *pRetDerCertLength, void** ppRetHint);

/* find an identity certificate chain by keyType and supported algorithm flags: ecCurves, sign keyType, and hash defined above. */
MOC_EXTERN MSTATUS CERT_STORE_findIdentityCertChainFirst(const certStorePtr pCertStore, ubyte4 pubKeyType, ubyte4 supportedAlgoFlags, struct AsymmetricKey** ppRetIdentityKey, SizedBuffer** ppRetCertificates, ubyte4 *pRetNumberCertificate, void** ppRetHint);
MOC_EXTERN MSTATUS CERT_STORE_findIdentityCertChainNext(const certStorePtr pCertStore, ubyte4 pubKeyType, ubyte4 supportedKeyTypeAndAlgoFlags, struct AsymmetricKey** ppRetIdentityKey, SizedBuffer** ppRetCertificates, ubyte4 *pRetNumberCertificate, void** ppRetHint);

/* traverse/find a psk */
MOC_EXTERN MSTATUS CERT_STORE_traversePskListHead(const certStorePtr pCertStore, ubyte **ppRetPskIdentity, ubyte4 *pRetPskIdentityLength, ubyte **ppRetPskHint, ubyte4 *pRetPskHintLength, ubyte **ppRetPskSecret, ubyte4 *pRetPskSecretLength, void** ppRetHint);
MOC_EXTERN MSTATUS CERT_STORE_traversePskListNext(const certStorePtr pCertStore, ubyte **ppRetPskIdentity, ubyte4 *pRetPskIdentityLength, ubyte **ppRetPskHint, ubyte4 *pRetPskHintLength, ubyte **ppRetPskSecret, ubyte4 *pRetPskSecretLength, void** ppRetHint);
MOC_EXTERN MSTATUS CERT_STORE_findPskByIdentity(const certStorePtr pCertStore, ubyte *pPskIdentity, ubyte4 pskIdentityLength, ubyte **ppRetPskSecret, ubyte4 *pRetPskSecretLength);

/* certificate store reentrancy protection */
MOC_EXTERN MSTATUS CERT_STORE_lockStore(certStorePtr pCertStore);

#ifdef __cplusplus
}
#endif

#endif /* __CERT_STORE_HEADER__ */

