/* Version: mss_v6_3 */
/*
 * pkcs1.c
 *
 * PKCS#1 Version 2.1 Utilities
 *
 * Copyright Mocana Corp 2009. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

#include "../common/moptions.h"

#ifdef __ENABLE_MOCANA_PKCS1__

#include "../common/mtypes.h"
#include "../common/mocana.h"
#include "../crypto/hw_accel.h"

#include "../common/mdefs.h"
#include "../common/merrors.h"
#include "../crypto/secmod.h"
#include "../common/mstdlib.h"
#include "../common/mrtos.h"
#include "../common/tree.h"
#include "../common/absstream.h"
#include "../common/memfile.h"
#include "../common/vlong.h"
#include "../common/random.h"
#include "../common/memory_debug.h"
#include "../common/debug_console.h"
#include "../asn1/oiddefs.h"
#include "../crypto/crypto.h"
#include "../crypto/rsa.h"
#ifdef __ENABLE_MOCANA_ECC__
#include "../crypto/primefld.h"
#include "../crypto/primeec.h"
#endif
#include "../crypto/pubcrypto.h"
#include "../crypto/pkcs1.h"
#ifdef __ENABLE_MOCANA_FIPS_MODULE__
#include "../crypto/fips.h"
#endif
#include "../harness/harness.h"

/* #define __ENABLE_MOCANA_PKCS1_DEBUG__ */


/*--------------------------------------------------------------------------*/

extern MSTATUS
PKCS1_OS2IP(const ubyte *pMessage, ubyte4 mesgLen, vlong **ppRetM)
{
    MSTATUS status;

    if (OK > (status = VLONG_vlongFromByteString(pMessage, (sbyte4)mesgLen, ppRetM, NULL)))
        goto exit;

    DEBUG_RELABEL_MEMORY(*ppRetM);

exit:
    return status;
}


/*--------------------------------------------------------------------------*/

extern MSTATUS
PKCS1_I2OSP(vlong *pValue, ubyte4 fixedLength, ubyte **ppRetString)
{
    ubyte*  pString = NULL;
    MSTATUS status = ERR_MEM_ALLOC_FAIL;

    if (NULL != (pString = MALLOC(fixedLength)))
    {
        if (OK > (status = VLONG_fixedByteStringFromVlong(pValue, pString, (sbyte4)fixedLength)))
            goto exit;

        /* set results */
        *ppRetString = pString;
        pString = NULL;
    }

exit:
    if (NULL != pString)
        FREE(pString);

    return status;
}


/*--------------------------------------------------------------------------*/

extern MSTATUS
#ifdef OPENSSL_ENGINE
MOC_PKCS1_MGF1(hwAccelDescr hwAccelCtx, const ubyte *mgfSeed, ubyte4 mgfSeedLen, ubyte4 maskLen, BulkHashAlgo *H, ubyte **ppRetMask)
#else
PKCS1_MGF1(hwAccelDescr hwAccelCtx, const ubyte *mgfSeed, ubyte4 mgfSeedLen, ubyte4 maskLen, BulkHashAlgo *H, ubyte **ppRetMask)
#endif
{
    /* RFC 3447, section B.2.1: MGF1 is a Mask Generation Function based on a hash function */
    BulkCtx hashCtx = NULL;
    ubyte*  T    = NULL;
    ubyte*  C    = NULL;
    ubyte*  mask = NULL;
    ubyte4  Tlen;
    ubyte4  TbufLen = 0;
    MSTATUS status;

    if ((NULL == mgfSeed) || (NULL == H) || (NULL == ppRetMask))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    TbufLen = maskLen + H->digestSize;

    if (NULL == (T = MALLOC(TbufLen)))
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    if (NULL == (mask = MALLOC(maskLen)))
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    if (OK > (status = CRYPTO_ALLOC(hwAccelCtx, 4, TRUE, &C)))
        goto exit;

    DEBUG_RELABEL_MEMORY(C);

    /* C = 0 */
    C[0] = C[1] = C[2] = C[3] = 0;

    /* setup hash context */
    if (OK > (status = H->allocFunc(MOC_HASH(hwAccelCtx) &hashCtx)))
        goto exit;

    for (Tlen = 0; Tlen < maskLen; Tlen += H->digestSize)
    {
        /* T = T || Hash(mgfSeed || C) */
        if (OK > (status = H->initFunc(MOC_HASH(hwAccelCtx) hashCtx)))
            goto exit;

        if (OK > (status = H->updateFunc(MOC_HASH(hwAccelCtx) hashCtx, mgfSeed, mgfSeedLen)))
            goto exit;

        if (OK > (status = H->updateFunc(MOC_HASH(hwAccelCtx) hashCtx, C, 4)))
            goto exit;

        if (OK > (status = H->finalFunc(MOC_HASH(hwAccelCtx) hashCtx, Tlen + T)))
            goto exit;

        /* increment string counter */
        if (0 == ++C[3])
            if (0 == ++C[2])
                if (0 == ++C[1])
                    ++C[0];
    }

    /* copy out result */
    if (OK > (status = MOC_MEMCPY(mask, T, maskLen)))
        goto exit;

    *ppRetMask = mask;
    mask = NULL;

exit:
    if ((NULL != H) && (NULL != hashCtx))
        H->freeFunc(MOC_HASH(hwAccelCtx) &hashCtx);

    CRYPTO_FREE(hwAccelCtx, TRUE, &C);

    if (NULL != mask)
    {
        /* zeroize buffer, before releasing */
        MOC_MEMSET(mask, 0x00, maskLen);
        FREE(mask);
    }

    if (NULL != T)
    {
        /* zeroize buffer, before releasing */
        MOC_MEMSET(T, 0x00, TbufLen);
        FREE(T);
    }

    return status;
}


/*--------------------------------------------------------------------------*/

static MSTATUS
emeOaepEncode(hwAccelDescr hwAccelCtx, randomContext *pRandomContext,
              ubyte4 k, BulkHashAlgo *H, ubyte4 hLen,
              mgfFunc MGF, const ubyte *M, ubyte4 mLen, const ubyte *L, ubyte4 lLen,
              ubyte** ppRetEM)
{
    BulkCtx         hashCtx = NULL;
    ubyte*          EM = NULL;
    ubyte*          DB = NULL;
    ubyte*          seed = NULL;
    ubyte*          seedMask = NULL;
    ubyte*          maskedSeed = NULL;
    ubyte*          dbMask = NULL;
    ubyte*          maskedDB = NULL;
    ubyte4          i;
    MSTATUS         status;

    /* allocate memory for DB */
    if (OK > (status = CRYPTO_ALLOC(hwAccelCtx, k - hLen - 1, TRUE, &DB)))
        goto exit;

    DEBUG_RELABEL_MEMORY(DB);

    /* setup hash context */
    if (OK > (status = H->allocFunc(MOC_HASH(hwAccelCtx) &hashCtx)))
        goto exit;

    if (OK > (status = H->initFunc(MOC_HASH(hwAccelCtx) hashCtx)))
        goto exit;

    if ((0 != lLen) && (NULL != L))
    {
        /* make sure there is something to hash */
        if (OK > (status = H->updateFunc(MOC_HASH(hwAccelCtx) hashCtx, L, lLen)))
            goto exit;
    }

    /* DB = lHash = HASH(L) */
    /* lHash || PS || 0x01 || M */
    if (OK > (status = H->finalFunc(MOC_HASH(hwAccelCtx) hashCtx, DB)))
        goto exit;

    /* || PS */
    MOC_MEMSET(DB + hLen, 0x00, k - mLen - (2 * hLen) - 2);

    /* || 0x01 */
    DB[hLen + (k - mLen - (2 * hLen) - 2)] = 0x01;

    /* || M */
    MOC_MEMCPY(DB + hLen + (k - mLen - (2 * hLen) - 2) + 1, M, mLen);

    /* allocate memory for seed */
    if (OK > (status = CRYPTO_ALLOC(hwAccelCtx, hLen, TRUE, &seed)))
        goto exit;

    DEBUG_RELABEL_MEMORY(seed);

    /* generate a random octet string seed of length hLen */
    if (OK > (status = RANDOM_numberGenerator(pRandomContext, seed, hLen)))
        goto exit;

    /* dbMask = MGF(seed, k - hLen - 1) */
    if (OK > (status = MGF(hwAccelCtx, seed, hLen, k - hLen -1, H, &dbMask)))
        goto exit;

    /* allocate EM == 0x00 || maskedSeed || maskedDB */
    if (OK > (status = CRYPTO_ALLOC(hwAccelCtx, k, TRUE, &EM)))
        goto exit;

    DEBUG_RELABEL_MEMORY(EM);

    /* set maskedDB */
    maskedDB = EM + 1 + hLen;

    /* maskedDB = DB xor dbMask */
    for (i = 0; i < k - hLen - 1; i++)
        maskedDB[i] = DB[i] ^ dbMask[i];

    /* seedMask = MGF(maskedDB, hLen) */
    if (OK > (status = MGF(hwAccelCtx, maskedDB, k - hLen - 1, hLen, H, &seedMask)))
        goto exit;

    /* set maskedSeed */
    maskedSeed = EM + 1;

    /* maskedSeed = seed xor seedMask */
    for (i = 0; i < hLen; i++)
        maskedSeed[i] = seed[i] ^ seedMask[i];

    /* EM = 0x00 || maskedSeed || maskedDB */
    EM[0] = 0x00;

    /* return EM */
    *ppRetEM = EM;
    EM = NULL;

exit:
    CRYPTO_FREE(hwAccelCtx, TRUE, &seedMask);
    CRYPTO_FREE(hwAccelCtx, TRUE, &EM);
    CRYPTO_FREE(hwAccelCtx, TRUE, &dbMask);
    CRYPTO_FREE(hwAccelCtx, TRUE, &seed);

    if ((NULL != H) && (NULL != hashCtx))
        H->freeFunc(MOC_HASH(hwAccelCtx) &hashCtx);

    CRYPTO_FREE(hwAccelCtx, TRUE, &DB);

    return status;

} /* emeOaepEncode */


/*--------------------------------------------------------------------------*/

static MSTATUS
PKCS1_rsaEncryption(MOC_RSA(hwAccelDescr hwAccelCtx)
                    const RSAKey *pRSAKey, ubyte4 k, ubyte *EM,
                    ubyte **ppRetC)
{
    vlong*  m = NULL;
    vlong*  c = NULL;
    ubyte*  C = NULL;
    vlong*  pVlongQueue = NULL;
    MSTATUS status;

    /* RFC 3447, section 4.2 */
    if (OK > (status = PKCS1_OS2IP(EM, k, &m)))
        goto exit;

    if (OK > (status = RSA_RSAEP(MOC_RSA(hwAccelCtx) pRSAKey, m, &c, &pVlongQueue)))
        goto exit;

    DEBUG_RELABEL_MEMORY(c);

    /* RFC 3447, section 4.1 */
    if (OK > (status = PKCS1_I2OSP(c, k, &C)))
        goto exit;

    DEBUG_RELABEL_MEMORY(C);

    /* set results */
    *ppRetC = C;
    C = NULL;

exit:
    if (NULL != C)
        FREE(C);

    VLONG_freeVlong(&c, &pVlongQueue);
    VLONG_freeVlong(&m, &pVlongQueue);
    VLONG_freeVlongQueue(&pVlongQueue);

    return status;

} /* PKCS1_rsaEncryption */


/*--------------------------------------------------------------------------*/

extern MSTATUS
PKCS1_rsaesOaepEncrypt(hwAccelDescr hwAccelCtx, randomContext *pRandomContext,
                       const RSAKey *pRSAKey, ubyte H_rsaAlgoId,
                       mgfFunc MGF, const ubyte *M, ubyte4 mLen, const ubyte *L, ubyte4 lLen,
                       ubyte **ppRetEncrypt, ubyte4 *pRetEncryptLen)
{
    BulkHashAlgo*   H = NULL;
    ubyte*          EM = NULL;
    ubyte*          cloneL = NULL;
    ubyte4          hLen;
    ubyte4          k;
    MSTATUS         status;

    if ((NULL == pRSAKey) || (NULL == MGF) || (NULL == M) || (NULL == ppRetEncrypt))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if ((NULL != L) && (0 != lLen))
    {
        if (OK > (status = CRYPTO_ALLOC(hwAccelCtx, lLen, TRUE, &cloneL)))
            goto exit;

        DEBUG_RELABEL_MEMORY(cloneL);

        MOC_MEMCPY(cloneL, L, lLen);
    }

    /* RFC 3447, section 7.1.1 */
    if (OK > (status = CRYPTO_getRSAHashAlgo(H_rsaAlgoId, (const BulkHashAlgo**)&H)))
        goto exit;

    hLen = H->digestSize;
    k = (7 + VLONG_bitLength(RSA_N(pRSAKey))) / 8;

    if ((k - (2 * hLen) - 2) < mLen)
    {
        /* message too long */
        status = ERR_BAD_LENGTH;
        goto exit;
    }

    if (OK > (status = emeOaepEncode(hwAccelCtx, pRandomContext, k, H, hLen, MGF, M, mLen, cloneL, lLen, &EM)))
        goto exit;

    if (OK > (status = PKCS1_rsaEncryption(MOC_RSA(hwAccelCtx) pRSAKey, k, EM, ppRetEncrypt)))
        goto exit;

    DEBUG_RELABEL_MEMORY(*ppRetEncrypt);
    *pRetEncryptLen = k;

exit:
    CRYPTO_FREE(hwAccelCtx, TRUE, &EM);
    CRYPTO_FREE(hwAccelCtx, TRUE, &cloneL);

    return status;

} /* PKCS1_rsaesOaepEncrypt */


/*--------------------------------------------------------------------------*/

static MSTATUS
emeOaepDecode(hwAccelDescr hwAccelCtx, ubyte4 k, BulkHashAlgo *H, ubyte4 hLen,
              mgfFunc MGF, ubyte *EM, const ubyte *L, ubyte4 lLen,
              ubyte** ppRetM, ubyte4 *pRetMlen)
{
    BulkCtx         hashCtx = NULL;
    ubyte*          lHash = NULL;
    ubyte*          DB = NULL;
    ubyte*          seed = NULL;
    ubyte*          seedMask = NULL;
    ubyte*          maskedSeed = NULL;
    ubyte*          dbMask = NULL;
    ubyte*          maskedDB = NULL;
    ubyte*          M = NULL;
    sbyte4          mLen;
    ubyte4          i;
    ubyte           Y;
    MSTATUS         status;

    /* allocate memory for lHash */
    if (OK > (status = CRYPTO_ALLOC(hwAccelCtx, hLen, TRUE, &lHash)))
        goto exit;

    DEBUG_RELABEL_MEMORY(lHash);

    /* setup hash context */
    if (OK > (status = H->allocFunc(MOC_HASH(hwAccelCtx) &hashCtx)))
        goto exit;

    if (OK > (status = H->initFunc(MOC_HASH(hwAccelCtx) hashCtx)))
        goto exit;

    if ((0 != lLen) && (NULL != L))
    {
        /* make sure there is something to hash */
        if (OK > (status = H->updateFunc(MOC_HASH(hwAccelCtx) hashCtx, L, lLen)))
            goto exit;
    }

    /* lHash = HASH(L) */
    if (OK > (status = H->finalFunc(MOC_HASH(hwAccelCtx) hashCtx, lHash)))
        goto exit;

    /* separate EM == Y || maskedSeed || maskedDB */
    Y = *EM;

    /* set maskedSeed */
    maskedSeed = EM + 1;

    /* set maskedDB */
    maskedDB = EM + 1 + hLen;

    /* seedMask = MGF(maskedDB, hLen) */
    if (OK > (status = MGF(hwAccelCtx, maskedDB, k - hLen - 1, hLen, H, &seedMask)))
        goto exit;

    /* allocate memory for seed */
    if (OK > (status = CRYPTO_ALLOC(hwAccelCtx, hLen, TRUE, &seed)))
        goto exit;

    DEBUG_RELABEL_MEMORY(seed);

    /* maskedSeed = seed xor seedMask */
    for (i = 0; i < hLen; i++)
        seed[i] = maskedSeed[i] ^ seedMask[i];

    /* dbMask = MGF(seed, k - hLen - 1) */
    if (OK > (status = MGF(hwAccelCtx, seed, hLen, k - hLen -1, H, &dbMask)))
        goto exit;

    /* allocate DB = lHash' || PS || 0x01 || M */
    if (OK > (status = CRYPTO_ALLOC(hwAccelCtx, k - hLen - 1, TRUE, &DB)))
        goto exit;

    DEBUG_RELABEL_MEMORY(DB);

    /* maskedDB = DB xor dbMask */
    for (i = 0; i < k - hLen - 1; i++)
        DB[i] = maskedDB[i] ^ dbMask[i];

    i = hLen;
    while ((0x01 != DB[i]) && (i < k - hLen - 1))
    {
        /* find first 0x01 */
        i++;
    }

    mLen = ((k - hLen - 1) - i) - 1;

    if ((0 < mLen) && (NULL == (M = MALLOC(mLen))))
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    /* copy out M */
    if (0 < mLen)
    {
        MOC_MEMCPY(M, DB + i + 1, mLen);
    }

    /* now do tests to make sure cipher text is correct in form */
    if ((0 != Y) || (0 >= mLen))
    {
        status = ERR_CRYPTO_FAILURE;
        goto exit;
    }

    /* return M */
    *ppRetM = M;
    M = NULL;

    /* and length */
    *pRetMlen = mLen;

exit:
    CRYPTO_FREE(hwAccelCtx, TRUE, &seedMask);
    CRYPTO_FREE(hwAccelCtx, TRUE, &EM);
    CRYPTO_FREE(hwAccelCtx, TRUE, &dbMask);
    CRYPTO_FREE(hwAccelCtx, TRUE, &seed);

    if ((NULL != H) && (NULL != hashCtx))
        H->freeFunc(MOC_HASH(hwAccelCtx) &hashCtx);

    CRYPTO_FREE(hwAccelCtx, TRUE, &DB);
    CRYPTO_FREE(hwAccelCtx, TRUE, &lHash);

    return status;

} /* emeOaepDecode */


/*--------------------------------------------------------------------------*/

#if !defined(__DISABLE_MOCANA_RSA_DECRYPTION__)
static MSTATUS
PKCS1_rsaDecryption(MOC_RSA(hwAccelDescr hwAccelCtx)
                    const RSAKey *pRSAKey, ubyte4 k, const ubyte *C,
                    ubyte **ppRetEM)
{
    ubyte*  EM = NULL;
    vlong*  m  = NULL;
    vlong*  c  = NULL;
    vlong*  pVlongQueue = NULL;
    MSTATUS status;

    /* RFC 3447, section 4.2 */
    if (OK > (status = PKCS1_OS2IP(C, k, &c)))
        goto exit;

    if (OK > (status = RSA_RSADP(MOC_RSA(hwAccelCtx) pRSAKey, c, &m, &pVlongQueue)))
        goto exit;

    DEBUG_RELABEL_MEMORY(m);

    /* RFC 3447, section 4.1 */
    if (OK > (status = PKCS1_I2OSP(m, k, &EM)))
        goto exit;

    DEBUG_RELABEL_MEMORY(EM);

    /* set results */
    *ppRetEM = EM;
    EM = NULL;

exit:
    if (NULL != EM)
        FREE(EM);

    VLONG_freeVlong(&m, &pVlongQueue);
    VLONG_freeVlong(&c, &pVlongQueue);
    VLONG_freeVlongQueue(&pVlongQueue);

    return status;
}
#endif


/*--------------------------------------------------------------------------*/

#if !defined(__DISABLE_MOCANA_RSA_DECRYPTION__)
extern MSTATUS
PKCS1_rsaesOaepDecrypt(hwAccelDescr hwAccelCtx, const RSAKey *pRSAKey, ubyte H_rsaAlgoId,
                       mgfFunc MGF, const ubyte *C, ubyte4 cLen, const ubyte *L, ubyte4 lLen,
                       ubyte **ppRetDecrypt, ubyte4 *pRetDecryptLength)
{
    BulkHashAlgo*   H = NULL;
    ubyte*          EM = NULL;
    ubyte*          cloneL = NULL;
    ubyte4          hLen;
    ubyte4          k;
    MSTATUS         status;

    if ((NULL == pRSAKey) || (NULL == MGF) || (NULL == C) || (NULL == ppRetDecrypt))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if ((NULL != L) && (0 != lLen))
    {
        if (OK > (status = CRYPTO_ALLOC(hwAccelCtx, lLen, TRUE, &cloneL)))
            goto exit;

        DEBUG_RELABEL_MEMORY(cloneL);

        MOC_MEMCPY(cloneL, L, lLen);
    }

    /* RFC 3447, section 7.1.1 */
    if (OK > (status = CRYPTO_getRSAHashAlgo(H_rsaAlgoId, (const BulkHashAlgo**)&H)))
        goto exit;

    hLen = H->digestSize;
    k = (7 + VLONG_bitLength(RSA_N(pRSAKey))) / 8;

    if ((k != cLen) || (k < ((2 * hLen) + 2)))
    {
        /* message should be equal to key length */
        status = ERR_CRYPTO_FAILURE;
        goto exit;
    }

    if (OK > (status = PKCS1_rsaDecryption(MOC_RSA(hwAccelCtx) pRSAKey, k, C, &EM)))
        goto exit;

    if (OK > (status = emeOaepDecode(hwAccelCtx, k, H, hLen, MGF, EM, L, lLen, ppRetDecrypt, pRetDecryptLength)))
        goto exit;

    DEBUG_RELABEL_MEMORY(*ppRetDecrypt);

exit:
    CRYPTO_FREE(hwAccelCtx, TRUE, &cloneL);

    return status;

} /* PKCS1_rsaesOaepDecrypt */

#endif /* !defined(__DISABLE_MOCANA_RSA_DECRYPTION__) */


/*--------------------------------------------------------------------------*/

static MSTATUS
emsaPssEncode(hwAccelDescr hwAccelCtx, randomContext *pRandomContext,
              const ubyte *M, ubyte4 mLen, ubyte4 emBits, ubyte4 sLen,
              BulkHashAlgo *Halgo, ubyte4 hLen,
              mgfFunc MGF, ubyte** ppRetEM)
{
    ubyte4  emLen = ((emBits + 7) / 8);
    BulkCtx hashCtx = NULL;
    ubyte*  EM = NULL;
    ubyte*  Mprime = NULL;
    ubyte*  DB = NULL;
    ubyte*  salt = NULL;
    ubyte*  H = NULL;
    ubyte*  dbMask = NULL;
    ubyte*  maskedDB = NULL;
    ubyte4  i, leftBits;
    MSTATUS status;

#ifdef __ENABLE_MOCANA_PKCS1_DEBUG__
    DEBUG_PRINTNL(DEBUG_CRYPTO, "emsaPssEncode: got here.");
#endif

    *ppRetEM = NULL;

    if ((emLen < (hLen + sLen + 2)) ||
        (emBits < ((8 * hLen) + (8 * sLen) + 9)))
    {
        status = ERR_BAD_LENGTH;
        goto exit;
    }

    /* allocate memory for M' (Mprime) */
    if (OK > (status = CRYPTO_ALLOC(hwAccelCtx, 8 + hLen + sLen, TRUE, &Mprime)))
        goto exit;

    DEBUG_RELABEL_MEMORY(Mprime);

    /* setup hash context */
    if (OK > (status = Halgo->allocFunc(MOC_HASH(hwAccelCtx) &hashCtx)))
        goto exit;

    if (OK > (status = Halgo->initFunc(MOC_HASH(hwAccelCtx) hashCtx)))
        goto exit;

    if ((0 != mLen) && (NULL != M))
    {
        /* make sure there is something to hash */
        if (OK > (status = Halgo->updateFunc(MOC_HASH(hwAccelCtx) hashCtx, M, mLen)))
            goto exit;
    }

    /* M' = (0x)00 00 00 00 00 00 00 00 || mHash || salt */
    /* set first 8 octets to zero */
    MOC_MEMSET(Mprime, 0x00, 8);

    /* append mHash */
    if (OK > (status = Halgo->finalFunc(MOC_HASH(hwAccelCtx) hashCtx, 8 + Mprime)))
        goto exit;

    /* append random octet string salt of length sLen */
    salt = 8 + hLen + Mprime;

    if (OK > (status = RANDOM_numberGenerator(pRandomContext, salt, sLen)))
        goto exit;

#ifdef __ENABLE_MOCANA_PKCS1_DEBUG__
    DEBUG_PRINTNL(DEBUG_CRYPTO, "M'=");
    DEBUG_HEXDUMP(DEBUG_CRYPTO, Mprime, 8 + hLen + sLen);
#endif

#ifdef __ENABLE_MOCANA_PKCS1_DEBUG__
    DEBUG_PRINTNL(DEBUG_CRYPTO, "salt=");
    DEBUG_HEXDUMP(DEBUG_CRYPTO, salt, sLen);
#endif

    /* allocate memory for H */
    if (OK > (status = CRYPTO_ALLOC(hwAccelCtx, hLen, TRUE, &H)))
        goto exit;

    DEBUG_RELABEL_MEMORY(H);

    /* H = Hash(M') */
    if (OK > (status = Halgo->initFunc(MOC_HASH(hwAccelCtx) hashCtx)))
        goto exit;

    if (OK > (status = Halgo->updateFunc(MOC_HASH(hwAccelCtx) hashCtx, Mprime, 8 + hLen + sLen)))
        goto exit;

    if (OK > (status = Halgo->finalFunc(MOC_HASH(hwAccelCtx) hashCtx, H)))
        goto exit;

#ifdef __ENABLE_MOCANA_PKCS1_DEBUG__
    DEBUG_PRINTNL(DEBUG_CRYPTO, "H=");
    DEBUG_HEXDUMP(DEBUG_CRYPTO, H, hLen);
#endif

    /* allocate memory for DB */
    if (OK > (status = CRYPTO_ALLOC(hwAccelCtx, emLen - hLen - 1, TRUE, &DB)))
        goto exit;

    DEBUG_RELABEL_MEMORY(DB);

    /* DB = PS || 0x01 || salt */
    /* clear first PS octets */
    if (0 < emLen - sLen - hLen - 2)
        MOC_MEMSET(DB, 0x00, emLen - sLen - hLen - 2);

    *(DB + emLen - sLen - hLen - 2) = 0x01;

    MOC_MEMCPY(DB + emLen - sLen - hLen - 1, salt, sLen);

#ifdef __ENABLE_MOCANA_PKCS1_DEBUG__
    DEBUG_PRINTNL(DEBUG_CRYPTO, "DB=");
    DEBUG_HEXDUMP(DEBUG_CRYPTO, DB, emLen - hLen - 1);
#endif

    /* dbMask = MGF(H, emLen - hLen - 1) */
    if (OK > (status = MGF(hwAccelCtx, H, hLen, emLen - hLen - 1, Halgo, &dbMask)))
        goto exit;

#ifdef __ENABLE_MOCANA_PKCS1_DEBUG__
    DEBUG_PRINTNL(DEBUG_CRYPTO, "dbMask=");
    DEBUG_HEXDUMP(DEBUG_CRYPTO, dbMask, emLen - hLen - 1);
#endif

    /* allocate EM == maskedDB || H || 0xbc */
    if (OK > (status = CRYPTO_ALLOC(hwAccelCtx, emLen, TRUE, &EM)))
        goto exit;

    DEBUG_RELABEL_MEMORY(EM);

    /* set maskedDB */
    maskedDB = EM;

    /* EM == maskedDB || H || 0xbc */
    /* maskedDB = DB xor dbMask */
    for (i = 0; i < emLen - hLen - 1; i++)
        maskedDB[i] = DB[i] ^ dbMask[i];

    /* clear leftmost maskedDB bits ((8 * emLen) - emBits) */
    if (0 < (leftBits = ((8 * emLen) - emBits)))
    {
        ubyte mask = 0xff >> leftBits;

        *maskedDB = ((*maskedDB) & mask);
    }

#ifdef __ENABLE_MOCANA_PKCS1_DEBUG__
    DEBUG_PRINTNL(DEBUG_CRYPTO, "maskedDB=");
    DEBUG_HEXDUMP(DEBUG_CRYPTO, maskedDB, emLen - hLen - 1);
#endif

    /* append H to EM */
    MOC_MEMCPY(EM + emLen - hLen - 1, H, hLen);

    /* append 0xbc*/
    *(EM + emLen - 1) = 0xbc;

#ifdef __ENABLE_MOCANA_PKCS1_DEBUG__
    DEBUG_PRINTNL(DEBUG_CRYPTO, "EM=");
    DEBUG_HEXDUMP(DEBUG_CRYPTO, EM, emLen);
#endif

    /* return EM */
    *ppRetEM = EM;
    EM = NULL;

exit:
    CRYPTO_FREE(hwAccelCtx, TRUE, &EM);
    CRYPTO_FREE(hwAccelCtx, TRUE, &dbMask);
    CRYPTO_FREE(hwAccelCtx, TRUE, &DB);
    CRYPTO_FREE(hwAccelCtx, TRUE, &Mprime);

    if ((NULL != Halgo) && (NULL != hashCtx))
        Halgo->freeFunc(MOC_HASH(hwAccelCtx) &hashCtx);

    CRYPTO_FREE(hwAccelCtx, TRUE, &H);

    return status;

} /* emsaPssEncode */


/*--------------------------------------------------------------------------*/

static MSTATUS
emsaPssVerify(hwAccelDescr hwAccelCtx, const ubyte * const M, ubyte4 mLen,
              ubyte *EM, ubyte4 emBits, ubyte4 sLen,
              BulkHashAlgo *Halgo, ubyte4 hLen,
              mgfFunc MGF, intBoolean *pIsConsistent)
{
    ubyte4  emLen = ((emBits + 7) / 8);
    BulkCtx hashCtx = NULL;
    ubyte*  maskedDB;
    ubyte*  H;
    ubyte*  DB = NULL;
    ubyte*  salt = NULL;
    ubyte*  mHash = NULL;
    ubyte*  dbMask = NULL;
    ubyte*  Mprime = NULL;
    ubyte*  Hprime = NULL;
    ubyte4  i, leftBits;
    sbyte4  result;
    MSTATUS status = OK;

#ifdef __ENABLE_MOCANA_PKCS1_DEBUG__
    DEBUG_PRINTNL(DEBUG_CRYPTO, "emsaPssVerify: got here.");
#endif

#ifdef __ENABLE_MOCANA_PKCS1_DEBUG__
    DEBUG_PRINTNL(DEBUG_CRYPTO, "EM=");
    DEBUG_HEXDUMP(DEBUG_CRYPTO, EM, emLen);
#endif

    /* default to result being inconsistent */
    *pIsConsistent = FALSE;

    /* test lengths */
    if ((emLen < (hLen + sLen + 2)) ||
        (emBits < ((8 * hLen) + (8 * sLen) + 9)) ||
        (0xbc != EM[emLen - 1]))
    {
        /* inconsistent, return */
        goto exit;
    }

    /* set pointers */
    maskedDB = EM;
    H = EM + emLen - hLen - 1;

#ifdef __ENABLE_MOCANA_PKCS1_DEBUG__
    DEBUG_PRINTNL(DEBUG_CRYPTO, "maskedDB=");
    DEBUG_HEXDUMP(DEBUG_CRYPTO, maskedDB, emLen - hLen - 1);
#endif

#ifdef __ENABLE_MOCANA_PKCS1_DEBUG__
    DEBUG_PRINTNL(DEBUG_CRYPTO, "H=");
    DEBUG_HEXDUMP(DEBUG_CRYPTO, H, hLen);
#endif

    /* test left most bits for zero */
    if (0 < (leftBits = ((8 * emLen) - emBits)))
    {
        ubyte mask = 0xff >> leftBits;

        if (0x00 != ((*maskedDB) & (0xff ^ mask)))
        {
            /* inconsistent, return */
            goto exit;
        }
    }

    /* allocate memory for mHash */
    if (OK > (status = CRYPTO_ALLOC(hwAccelCtx, hLen, TRUE, &mHash)))
        goto exit;

    DEBUG_RELABEL_MEMORY(mHash);

    /* setup hash context */
    if (OK > (status = Halgo->allocFunc(MOC_HASH(hwAccelCtx) &hashCtx)))
        goto exit;

    if (OK > (status = Halgo->initFunc(MOC_HASH(hwAccelCtx) hashCtx)))
        goto exit;

    if ((0 != mLen) && (NULL != M))
    {
        /* make sure there is something to hash */
        if (OK > (status = Halgo->updateFunc(MOC_HASH(hwAccelCtx) hashCtx, M, mLen)))
            goto exit;
    }

    /* mHash = Hash(M) */
    if (OK > (status = Halgo->finalFunc(MOC_HASH(hwAccelCtx) hashCtx, mHash)))
        goto exit;

#ifdef __ENABLE_MOCANA_PKCS1_DEBUG__
    DEBUG_PRINTNL(DEBUG_CRYPTO, "mHash=");
    DEBUG_HEXDUMP(DEBUG_CRYPTO, mHash, hLen);
#endif

    /* dbMask = MGF(H, emLen - hLen - 1 */
    if (OK > (status = MGF(hwAccelCtx, H, hLen, emLen - hLen - 1, Halgo, &dbMask)))
        goto exit;

#ifdef __ENABLE_MOCANA_PKCS1_DEBUG__
    DEBUG_PRINTNL(DEBUG_CRYPTO, "dbMask=");
    DEBUG_HEXDUMP(DEBUG_CRYPTO, dbMask, emLen - hLen - 1);
#endif

    /* allocate memory for mHash */
    if (OK > (status = CRYPTO_ALLOC(hwAccelCtx, emLen - hLen - 1, TRUE, &DB)))
        goto exit;

    DEBUG_RELABEL_MEMORY(DB);

    /* DB = maskedDB xor dbMask */
    for (i = 0; i < emLen - hLen - 1; i++)
        DB[i] = maskedDB[i] ^ dbMask[i];

#ifdef __ENABLE_MOCANA_PKCS1_DEBUG__
    DEBUG_PRINTNL(DEBUG_CRYPTO, "DB=");
    DEBUG_HEXDUMP(DEBUG_CRYPTO, DB, emLen - hLen - 1);
#endif

    /* clear leftmost DB bits ((8 * emLen) - emBits) */
    if (0 < (leftBits = ((8 * emLen) - emBits)))
    {
        ubyte mask = 0xff >> leftBits;

        *DB = ((*DB) & mask);
    }

    /* make sure the leftmost emLen - hLen - sLen - 2 octets are zero */
    for (i = 0; i < (emLen - hLen - sLen - 2); i++)
    {
        if (0x00 != DB[i])
        {
            /* inconsistent */
            goto exit;
        }
    }

    if (0x01 != DB[i])
    {
        /* inconsistent */
        goto exit;
    }

    /* allocate memory for M' (Mprime) */
    if (OK > (status = CRYPTO_ALLOC(hwAccelCtx, 8 + hLen + sLen, TRUE, &Mprime)))
        goto exit;

    DEBUG_RELABEL_MEMORY(DB);

    /* setup hash context */
    if (OK > (status = Halgo->initFunc(MOC_HASH(hwAccelCtx) hashCtx)))
        goto exit;

    if ((0 != mLen) && (NULL != M))
    {
        /* make sure there is something to hash */
        if (OK > (status = Halgo->updateFunc(MOC_HASH(hwAccelCtx) hashCtx, M, mLen)))
            goto exit;
    }

    /* M' = (0x)00 00 00 00 00 00 00 00 || mHash || salt */
    /* set first 8 octets to zero */
    MOC_MEMSET(Mprime, 0x00, 8);

    /* mHash = HASH(M) */
    if (OK > (status = Halgo->finalFunc(MOC_HASH(hwAccelCtx) hashCtx, 8 + Mprime)))
        goto exit;

    /* append random octet string salt of length sLen */
    salt = emLen - sLen - hLen - 1 + DB;

#ifdef __ENABLE_MOCANA_PKCS1_DEBUG__
    DEBUG_PRINTNL(DEBUG_CRYPTO, "salt=");
    DEBUG_HEXDUMP(DEBUG_CRYPTO, salt, sLen);
#endif

    if (OK > (status = MOC_MEMCPY(8 + hLen + Mprime, salt, sLen)))
        goto exit;

#ifdef __ENABLE_MOCANA_PKCS1_DEBUG__
    DEBUG_PRINTNL(DEBUG_CRYPTO, "M'=");
    DEBUG_HEXDUMP(DEBUG_CRYPTO, Mprime, 8 + hLen + sLen);
#endif

    /* allocate memory for H' (Hprime) */
    if (OK > (status = CRYPTO_ALLOC(hwAccelCtx, hLen, TRUE, &Hprime)))
        goto exit;

    DEBUG_RELABEL_MEMORY(Hprime);

    /* setup hash context */
    if (OK > (status = Halgo->initFunc(MOC_HASH(hwAccelCtx) hashCtx)))
        goto exit;

    if (OK > (status = Halgo->updateFunc(MOC_HASH(hwAccelCtx) hashCtx, Mprime, 8 + hLen + sLen)))
        goto exit;

    /* append mHash */
    if (OK > (status = Halgo->finalFunc(MOC_HASH(hwAccelCtx) hashCtx, Hprime)))
        goto exit;

#ifdef __ENABLE_MOCANA_PKCS1_DEBUG__
    DEBUG_PRINTNL(DEBUG_CRYPTO, "H'=");
    DEBUG_HEXDUMP(DEBUG_CRYPTO, Hprime, hLen);
#endif

    if (OK > (status = MOC_MEMCMP(H, Hprime, hLen, &result)))
        goto exit;

    if (0 == result)
    {
        /* message verified! */
        *pIsConsistent = TRUE;
    }

#ifdef __ENABLE_MOCANA_PKCS1_DEBUG__
    DEBUG_PRINTNL(DEBUG_CRYPTO, "emsaPssVerify: exit.");
#endif

exit:
    CRYPTO_FREE(hwAccelCtx, TRUE, &Hprime);
    CRYPTO_FREE(hwAccelCtx, TRUE, &Mprime);
    CRYPTO_FREE(hwAccelCtx, TRUE, &DB);
    CRYPTO_FREE(hwAccelCtx, TRUE, &dbMask);

    if ((NULL != Halgo) && (NULL != hashCtx))
        Halgo->freeFunc(MOC_HASH(hwAccelCtx) &hashCtx);

    CRYPTO_FREE(hwAccelCtx, TRUE, &mHash);

    return status;

} /* emsaPssVerify */


/*--------------------------------------------------------------------------*/

#if (!defined(__DISABLE_MOCANA_RSA_DECRYPTION__))

MOC_EXTERN MSTATUS
PKCS1_rsassaPssSign(hwAccelDescr hwAccelCtx, randomContext *pRandomContext,
                    const RSAKey *pRSAKey, ubyte H_rsaAlgoId, mgfFunc MGF,
                    const ubyte *pMessage, ubyte4 mesgLen, ubyte4 saltLen,
                    ubyte **ppRetSignature, ubyte4 *pRetSignatureLen)
{
    vlong*          s = NULL;
    vlong*          m = NULL;
    ubyte*          pEM = NULL;
    BulkHashAlgo*   Halgo;
    ubyte4          emBits;
    vlong*          pVlongQueue = NULL;
    MSTATUS         status;

#ifdef __ENABLE_MOCANA_FIPS_MODULE__
    if (OK != getFIPS_powerupStatus(FIPS_ALGO_RSA))
        return getFIPS_powerupStatus(FIPS_ALGO_RSA);
#endif /* __ENABLE_MOCANA_FIPS_MODULE__ */

    emBits = VLONG_bitLength(RSA_N(pRSAKey));

    if (OK > (status = CRYPTO_getRSAHashAlgo(H_rsaAlgoId, (const BulkHashAlgo**)&Halgo)))
        goto exit;

    if (OK > (status = emsaPssEncode(hwAccelCtx, pRandomContext,
                                     pMessage, mesgLen, emBits - 1, saltLen,
                                     Halgo, Halgo->digestSize, MGF, &pEM)))
    {
        goto exit;
    }

    if (OK > (status = PKCS1_OS2IP(pEM, ((emBits + 7) / 8), &m)))
        goto exit;

    if (OK > (status = RSA_RSASP1(MOC_RSA(hwAccelCtx) pRSAKey, m, NULL, NULL, &s, &pVlongQueue)))
        goto exit;

    if (OK > (status = PKCS1_I2OSP(s, ((emBits + 7) / 8), ppRetSignature)))
        goto exit;

    DEBUG_RELABEL_MEMORY(*ppRetSignature);

    *pRetSignatureLen = ((emBits + 7) / 8);

exit:
    VLONG_freeVlong(&s, &pVlongQueue);
    VLONG_freeVlong(&m, &pVlongQueue);
    VLONG_freeVlongQueue(&pVlongQueue);
    CRYPTO_FREE(hwAccelCtx, TRUE, &pEM);

    return status;

} /* PKCS1_rsassaPssSign */

#endif


/*--------------------------------------------------------------------------*/

MOC_EXTERN MSTATUS
PKCS1_rsassaFreePssSign(hwAccelDescr hwAccelCtx, ubyte **ppSignature)
{
    return CRYPTO_FREE(hwAccelCtx, TRUE, ppSignature);
}


/*--------------------------------------------------------------------------*/

MOC_EXTERN MSTATUS
PKCS1_rsassaPssVerify(hwAccelDescr hwAccelCtx,
                      const RSAKey *pRSAKey, ubyte H_rsaAlgoId, mgfFunc MGF,
                      const ubyte * const pMessage, ubyte4 mesgLen,
                      const ubyte *pSignature, ubyte4 signatureLen, ubyte4 saltLen,
                      intBoolean *pRetIsSignatureValid)
{
    vlong*          s = NULL;
    vlong*          m = NULL;
    ubyte*          pEM = NULL;
    BulkHashAlgo*   Halgo;
    ubyte4          emBits;
    vlong*          pVlongQueue = NULL;
    MSTATUS         status;

#ifdef __ENABLE_MOCANA_FIPS_MODULE__
    if (OK != getFIPS_powerupStatus(FIPS_ALGO_RSA))
        return getFIPS_powerupStatus(FIPS_ALGO_RSA);
#endif /* __ENABLE_MOCANA_FIPS_MODULE__ */

    /* default */
    *pRetIsSignatureValid = FALSE;

    emBits = VLONG_bitLength(RSA_N(pRSAKey));

    if (OK > (status = CRYPTO_getRSAHashAlgo(H_rsaAlgoId, (const BulkHashAlgo**)&Halgo)))
        goto exit;

    if (OK > (status = PKCS1_OS2IP(pSignature, signatureLen, &s)))
        goto exit;

    if (OK > (status = RSA_RSAVP1(MOC_RSA(hwAccelCtx) pRSAKey, s, &m, &pVlongQueue)))
        goto exit;

    /* !!!! PKCS#1 v2.1: pg 29: add bit test here! */

    if (OK > (status = PKCS1_I2OSP(m, ((emBits + 7) / 8), &pEM)))
        goto exit;

    DEBUG_RELABEL_MEMORY(pEM);

    if (OK > (status = emsaPssVerify(hwAccelCtx, pMessage, mesgLen,
                                     pEM, emBits-1, saltLen,
                                     Halgo, Halgo->digestSize,
                                     MGF, pRetIsSignatureValid)))
    {
        goto exit;
    }

    /* return an error with flag --- double notify, if signature check fails */
    if (TRUE != *pRetIsSignatureValid)
        status = ERR_RSA_BAD_SIGNATURE;

exit:
    if (NULL != pEM)
        FREE(pEM);

    VLONG_freeVlong(&m, &pVlongQueue);
    VLONG_freeVlong(&s, &pVlongQueue);
    VLONG_freeVlongQueue(&pVlongQueue);

    return status;

} /* PKCS1_rsassaPssVerify */

#endif /* __ENABLE_MOCANA_PKCS1__ */


/*--------------------------------------------------------------------------*/

#if 0
#include "../crypto/ca_mgmt.h"

extern MSTATUS
PKCS1_testRsaesOaep(void)
{
    AsymmetricKey   key;
    hwAccelDescr    hwAccelCtx = 0;
    ubyte*          pKeyBlob = NULL;
    ubyte4          keyBlobLength;
    ubyte           M[] = "Eagle lands at midnight.";
    ubyte*          C = NULL;
    ubyte4          cLen;
    ubyte*          D = NULL;
    ubyte4          dLen;
    ubyte4          count = 0;
    ubyte4          test;
    ubyte4          test1;
    MSTATUS         status;

    do
    {
        if (OK > (status = CA_MGMT_generateNakedKey(akt_rsa, 1024, &pKeyBlob, &keyBlobLength)))
            goto exit;

        if (OK > (status = CA_MGMT_extractKeyBlobEx(pKeyBlob, keyBlobLength, &key)))
            goto exit;

        if (1024 != (test1 = VLONG_bitLength(RSA_N(key.key.pRSA))))
            test = 0;

        if (OK > (status = PKCS1_rsaesOaepEncrypt(hwAccelCtx, g_pRandomContext, key.key.pRSA, sha1withRSAEncryption,
                           PKCS1_MGF1, M, sizeof(M), NULL, 0,
                           &C, &cLen)))
        {
            goto exit;
        }

        if (OK > (status = PKCS1_rsaesOaepDecrypt(hwAccelCtx, key.key.pRSA, sha1withRSAEncryption,
                           PKCS1_MGF1, C, cLen, NULL, 0, &D, &dLen)))
        {
            test = VLONG_bitLength(RSA_N(key.key.pRSA));
            goto exit;
        }

        FREE(C);
        FREE(D);
        CRYPTO_uninitAsymmetricKey(&key, NULL);
        FREE(pKeyBlob);
    }
    while (100 > ++count);

exit:
    return status;
}
#endif /* 0 */


/*--------------------------------------------------------------------------*/

#if 0
#include "sha1.h"

static const BulkHashAlgo SHA1Suite =
    { SHA1_RESULT_SIZE, SHA1_BLOCK_SIZE, SHA1_allocDigest, SHA1_freeDigest,
        (BulkCtxInitFunc)SHA1_initDigest, (BulkCtxUpdateFunc)SHA1_updateDigest, (BulkCtxFinalFunc)SHA1_finalDigest };

extern MSTATUS FIPS_powerupSelfTest(void);

static ubyte mesg[] =
{
    0x85, 0x9e, 0xef, 0x2f, 0xd7, 0x8a, 0xca, 0x00, 0x30, 0x8b, 0xdc, 0x47, 0x11, 0x93, 0xbf, 0x55,
    0xbf, 0x9d, 0x78, 0xdb, 0x8f, 0x8a, 0x67, 0x2b, 0x48, 0x46, 0x34, 0xf3, 0xc9, 0xc2, 0x6e, 0x64,
    0x78, 0xae, 0x10, 0x26, 0x0f, 0xe0, 0xdd, 0x8c, 0x08, 0x2e, 0x53, 0xa5, 0x29, 0x3a, 0xf2, 0x17,
    0x3c, 0xd5, 0x0c, 0x6d, 0x5d, 0x35, 0x4f, 0xeb, 0xf7, 0x8b, 0x26, 0x02, 0x1c, 0x25, 0xc0, 0x27,
    0x12, 0xe7, 0x8c, 0xd4, 0x69, 0x4c, 0x9f, 0x46, 0x97, 0x77, 0xe4, 0x51, 0xe7, 0xf8, 0xe9, 0xe0,
    0x4c, 0xd3, 0x73, 0x9c, 0x6b, 0xbf, 0xed, 0xae, 0x48, 0x7f, 0xb5, 0x56, 0x44, 0xe9, 0xca, 0x74,
    0xff, 0x77, 0xa5, 0x3c, 0xb7, 0x29, 0x80, 0x2f, 0x6e, 0xd4, 0xa5, 0xff, 0xa8, 0xba, 0x15, 0x98,
    0x90, 0xfc
};

extern MSTATUS
PKCS1_testPssEncodeDecode(void)
{
    ubyte*          pEM = NULL;
    hwAccelDescr    hwAccelCtx = 0;
    intBoolean      isConsistent;
    sbyte4          count;
    MSTATUS         status;

    status = FIPS_powerupSelfTest();

    MOCANA_initMocana();

    for (count = 0; count < 100; count++)
    {
        status = emsaPssEncode(hwAccelCtx, g_pRandomContext,
                               mesg, sizeof(mesg), 1024-1, 20,
                               &SHA1Suite, SHA1Suite.digestSize,
                               PKCS1_MGF1, &pEM);

        status = emsaPssVerify(hwAccelCtx, mesg, sizeof(mesg),
                               pEM, 1024-1, 20,
                               &SHA1Suite, SHA1Suite.digestSize,
                               PKCS1_MGF1, &isConsistent);
    }

    MOCANA_freeMocana();

    return status;
}


/*--------------------------------------------------------------------------*/

#include "../crypto/ca_mgmt.h"

static ubyte rsa_n[] =
{
    0xa2, 0xba, 0x40, 0xee, 0x07, 0xe3, 0xb2, 0xbd, 0x2f, 0x02, 0xce, 0x22, 0x7f, 0x36, 0xa1, 0x95,
    0x02, 0x44, 0x86, 0xe4, 0x9c, 0x19, 0xcb, 0x41, 0xbb, 0xbd, 0xfb, 0xba, 0x98, 0xb2, 0x2b, 0x0e,
    0x57, 0x7c, 0x2e, 0xea, 0xff, 0xa2, 0x0d, 0x88, 0x3a, 0x76, 0xe6, 0x5e, 0x39, 0x4c, 0x69, 0xd4,
    0xb3, 0xc0, 0x5a, 0x1e, 0x8f, 0xad, 0xda, 0x27, 0xed, 0xb2, 0xa4, 0x2b, 0xc0, 0x00, 0xfe, 0x88,
    0x8b, 0x9b, 0x32, 0xc2, 0x2d, 0x15, 0xad, 0xd0, 0xcd, 0x76, 0xb3, 0xe7, 0x93, 0x6e, 0x19, 0x95,
    0x5b, 0x22, 0x0d, 0xd1, 0x7d, 0x4e, 0xa9, 0x04, 0xb1, 0xec, 0x10, 0x2b, 0x2e, 0x4d, 0xe7, 0x75,
    0x12, 0x22, 0xaa, 0x99, 0x15, 0x10, 0x24, 0xc7, 0xcb, 0x41, 0xcc, 0x5e, 0xa2, 0x1d, 0x00, 0xee,
    0xb4, 0x1f, 0x7c, 0x80, 0x08, 0x34, 0xd2, 0xc6, 0xe0, 0x6b, 0xce, 0x3b, 0xce, 0x7e, 0xa9, 0xa5
};

static ubyte rsa_p[] =
{
    0xd1, 0x7f, 0x65, 0x5b, 0xf2, 0x7c, 0x8b, 0x16, 0xd3, 0x54, 0x62, 0xc9, 0x05, 0xcc, 0x04, 0xa2,
    0x6f, 0x37, 0xe2, 0xa6, 0x7f, 0xa9, 0xc0, 0xce, 0x0d, 0xce, 0xd4, 0x72, 0x39, 0x4a, 0x0d, 0xf7,
    0x43, 0xfe, 0x7f, 0x92, 0x9e, 0x37, 0x8e, 0xfd, 0xb3, 0x68, 0xed, 0xdf, 0xf4, 0x53, 0xcf, 0x00,
    0x7a, 0xf6, 0xd9, 0x48, 0xe0, 0xad, 0xe7, 0x57, 0x37, 0x1f, 0x8a, 0x71, 0x1e, 0x27, 0x8f, 0x6b
};

static ubyte rsa_q[] =
{
    0xc6, 0xd9, 0x2b, 0x6f, 0xee, 0x74, 0x14, 0xd1, 0x35, 0x8c, 0xe1, 0x54, 0x6f, 0xb6, 0x29, 0x87,
    0x53, 0x0b, 0x90, 0xbd, 0x15, 0xe0, 0xf1, 0x49, 0x63, 0xa5, 0xe2, 0x63, 0x5a, 0xdb, 0x69, 0x34,
    0x7e, 0xc0, 0xc0, 0x1b, 0x2a, 0xb1, 0x76, 0x3f, 0xd8, 0xac, 0x1a, 0x59, 0x2f, 0xb2, 0x27, 0x57,
    0x46, 0x3a, 0x98, 0x24, 0x25, 0xbb, 0x97, 0xa3, 0xa4, 0x37, 0xc5, 0xbf, 0x86, 0xd0, 0x3f, 0x2f
};

static ubyte4 rsa_e = 65537;

extern void dbg_dump(void);

extern MSTATUS
PKCS1_testPssSignVerify(void)
{
    AsymmetricKey   key;
    ubyte*          pEM = NULL;
    hwAccelDescr    hwAccelCtx = 0;
    sbyte4          count;
    ubyte*          pSignature = NULL;
    ubyte4          signatureLen;
    intBoolean      isSignatureValid;
    ubyte4          test = 1;
    ubyte4          test1 = 1;
    ubyte*          pKeyBlob = NULL;
    ubyte4          keyBlobLength;
    MSTATUS         status;

    status = FIPS_powerupSelfTest();

    MOCANA_initMocana();

    CRYPTO_initAsymmetricKey(&key);

    for (count = 0; count < 20; count++)
    {
/*        if (OK > (status = CRYPTO_setRSAParameters(&key, rsa_e, rsa_n, sizeof(rsa_n), rsa_p, sizeof(rsa_p), rsa_q, sizeof(rsa_q), NULL)))*/
/*            break;*/

        if (OK > (status = CA_MGMT_generateNakedKey(akt_rsa, 1024, &pKeyBlob, &keyBlobLength)))
            break;

        if (OK > (status = CA_MGMT_extractKeyBlobEx(pKeyBlob, keyBlobLength, &key)))
            break;

        if (1024 != (test1 = VLONG_bitLength(RSA_N(key.key.pRSA))))
        {
            vlong_unit p_high;
            vlong_unit q_high;

            p_high = VLONG_getVlongUnit(RSA_P(key.key.pRSA), VLONG_bitLength(RSA_P(key.key.pRSA)) / (sizeof(vlong_unit) * 8) - 1);
            q_high = VLONG_getVlongUnit(RSA_Q(key.key.pRSA), VLONG_bitLength(RSA_Q(key.key.pRSA)) / (sizeof(vlong_unit) * 8) - 1);

            test = 0;
        }

        status = PKCS1_rsassaPssSign(hwAccelCtx, g_pRandomContext,
                                     key.key.pRSA, sha1withRSAEncryption, PKCS1_MGF1,
                                     mesg, sizeof(mesg), 20,
                                     &pSignature, &signatureLen);

        if (OK > status)
        {
            DEBUG_ERROR(DEBUG_CRYPTO, "error at line = ", __LINE__);
        }

#ifdef __ENABLE_MOCANA_PKCS1_DEBUG__
    DEBUG_PRINTNL(DEBUG_CRYPTO, "s=");
    DEBUG_HEXDUMP(DEBUG_CRYPTO, pSignature, signatureLen);
#endif

        status = PKCS1_rsassaPssVerify(hwAccelCtx,
                                       key.key.pRSA, sha1withRSAEncryption, PKCS1_MGF1,
                                       mesg, sizeof(mesg),
                                       pSignature, signatureLen, 20,
                                       &isSignatureValid);

        if (OK > status)
        {
            DEBUG_ERROR(DEBUG_CRYPTO, "error at line = ", __LINE__);
        }

        if (TRUE != isSignatureValid)
        {
            DEBUG_ERROR(DEBUG_CRYPTO, "signature verify failed at line = ", __LINE__);
        }

        CRYPTO_uninitAsymmetricKey(&key, NULL);
        PKCS1_rsassaFreePssSign(hwAccelCtx, &pSignature);
        FREE(pKeyBlob);
    }

    MOCANA_freeMocana();

    dbg_dump();

    return status;
}


/*--------------------------------------------------------------------------*/

main()
{
    PKCS1_testPssSignVerify();
}
#endif
