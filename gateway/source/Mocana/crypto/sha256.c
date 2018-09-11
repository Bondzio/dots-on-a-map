/* Version: mss_v6_3 */
/*
 * sha256.c
 *
 * SHA - Secure Hash Algorithm
 *
 * Copyright Mocana Corp 2005-2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

#include "../common/moptions.h"

#if !defined( __DISABLE_MOCANA_SHA256__) || !defined(__DISABLE_MOCANA_SHA224__)

#include "../common/mtypes.h"
#include "../common/mocana.h"
#include "../crypto/hw_accel.h"

#ifndef __SHA256_HARDWARE_HASH__

#include "../common/mdefs.h"
#include "../common/merrors.h"
#include "../common/mstdlib.h"
#include "../common/mrtos.h"
#include "../common/int64.h"
#include "../crypto/sha256.h"
#ifdef __ENABLE_MOCANA_FIPS_MODULE__
#include "../crypto/fips.h"
#endif
#include "../harness/harness.h"

#ifdef TEST
#include <stdio.h>
#endif


/*------------------------------------------------------------------*/

/* SHA256 constants */
static ubyte4 K[64] = {
0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};


/* SHA256 functions */
//#define CH(X,Y,Z)               (((X) & (Y)) ^ ((~X) & (Z)))
#define CH(X,Y,Z)               (Z ^ (X & (Y ^ Z)))
//#define MAJ(X,Y,Z)              (((X) & (Y)) ^ ((X) & (Z)) ^ ((Y) & (Z)))
#define MAJ(X,Y,Z)              ((X & Y) | (Z & (X ^ Y)))

#define SHR(X, n)               ( ((ubyte4)(X)) >> (n))
#define SHL(X, n)               ( ((ubyte4)(X)) << (n))
#define ROTR(X, n)              (  (SHR(X,n)) | (SHL(X, (32-n))) )

#define BSIG0(X)                 ROTR((ROTR((ROTR(X,9))^X,11)^X),2)
#define BSIG1(X)                 ROTR((ROTR((ROTR(X,14))^X,5)^X),6)

#ifdef __ASM_386_GCC__
#define HOST_c2l(c,l)	({ ubyte4 r=*((ubyte4 *)(c));	\
				   asm ("bswapl %0":"=r"(r):"0"(r));	\
				   (l)=r;			})
#endif
//#define BSIG0(X)                 ((ROTR(X,2)) ^ (ROTR(X,13)) ^ (ROTR(X,22)) )
//#define BSIG1(X)                 ((ROTR(X,6)) ^ (ROTR(X,11)) ^ (ROTR(X,25)) )
#define LSIG0(X)                 ((ROTR(X,7)) ^ (ROTR(X,18)) ^ (SHR(X,3))   )
#define LSIG1(X)                 ((ROTR(X,17)) ^ (ROTR(X,19)) ^ (SHR(X,10)) )


/*------------------------------------------------------------------*/

extern MSTATUS
SHA256_allocDigest(MOC_HASH(hwAccelDescr hwAccelCtx) BulkCtx *pp_context)
{
#if( defined(__ENABLE_MOCANA_FIPS_MODULE__) )
   if (OK != getFIPS_powerupStatus(FIPS_ALGO_SHA256))
        return getFIPS_powerupStatus(FIPS_ALGO_SHA256);
#endif /* ( defined(__ENABLE_MOCANA_FIPS_MODULE__) ) */

    return CRYPTO_ALLOC(hwAccelCtx, sizeof(SHA256_CTX), TRUE, pp_context);
}


/*------------------------------------------------------------------*/

extern MSTATUS
SHA256_freeDigest(MOC_HASH(hwAccelDescr hwAccelCtx) BulkCtx *pp_context)
{
#ifdef __ZEROIZE_TEST__
    int counter = 0;
#endif

#if( defined(__ENABLE_MOCANA_FIPS_MODULE__) )
   if (OK != getFIPS_powerupStatus(FIPS_ALGO_SHA256))
        return getFIPS_powerupStatus(FIPS_ALGO_SHA256);
#endif /* ( defined(__ENABLE_MOCANA_FIPS_MODULE__) ) */

#ifdef __ZEROIZE_TEST__
        FIPS_PRINT("\nSHA256 - Before Zeroization\n");
        for( counter = 0; counter < sizeof(SHA256_CTX); counter++)
        {
            FIPS_PRINT("%02x",*((ubyte*)*pp_context+counter));
        }
        FIPS_PRINT("\n");
#endif

    /* Zeroize the sensitive information before deleting the memory */
    MOC_MEMSET(*pp_context,0x00,sizeof(SHA256_CTX));

#ifdef __ZEROIZE_TEST__
        FIPS_PRINT("\nSHA256 - After Zeroization\n");
        for( counter = 0; counter < sizeof(SHA256_CTX); counter++)
        {
            FIPS_PRINT("%02x",*((ubyte*)*pp_context+counter));
        }
        FIPS_PRINT("\n");
#endif

    return CRYPTO_FREE(hwAccelCtx, TRUE, pp_context);
}

#ifndef __DISABLE_MOCANA_SHA256__
/*------------------------------------------------------------------*/

extern MSTATUS
SHA256_initDigest(MOC_HASH(hwAccelDescr hwAccelCtx) SHA256_CTX *pContext)
{
    MSTATUS status;

#if( defined(__ENABLE_MOCANA_FIPS_MODULE__) )
   if (OK != getFIPS_powerupStatus(FIPS_ALGO_SHA256))
        return getFIPS_powerupStatus(FIPS_ALGO_SHA256);
#endif /* ( defined(__ENABLE_MOCANA_FIPS_MODULE__) ) */

    if (NULL == pContext)
    {
        status = ERR_NULL_POINTER;
    }
    else
    {
        pContext->hashBlocks[0] = 0x6a09e667;
        pContext->hashBlocks[1] = 0xbb67ae85;
        pContext->hashBlocks[2] = 0x3c6ef372;
        pContext->hashBlocks[3] = 0xa54ff53a;
        pContext->hashBlocks[4] = 0x510e527f;
        pContext->hashBlocks[5] = 0x9b05688c;
        pContext->hashBlocks[6] = 0x1f83d9ab;
        pContext->hashBlocks[7] = 0x5be0cd19;

        ZERO_U8(pContext->mesgLength);

        pContext->hashBufferIndex = 0;

        status = OK;
    }

    return status;
}

#endif


#ifndef __DISABLE_MOCANA_SHA224__
/*------------------------------------------------------------------*/

extern MSTATUS
SHA224_initDigest(MOC_HASH(hwAccelDescr hwAccelCtx) SHA224_CTX *pContext)
{
    MSTATUS status;

    if (NULL == pContext)
    {
        status = ERR_NULL_POINTER;
    }
    else
    {
        pContext->hashBlocks[0] = 0xc1059ed8;
        pContext->hashBlocks[1] = 0x367cd507;
        pContext->hashBlocks[2] = 0x3070dd17;
        pContext->hashBlocks[3] = 0xf70e5939;
        pContext->hashBlocks[4] = 0xffc00b31;
        pContext->hashBlocks[5] = 0x68581511;
        pContext->hashBlocks[6] = 0x64f98fa7;
        pContext->hashBlocks[7] = 0xbefa4fa4;

        ZERO_U8(pContext->mesgLength);

        pContext->hashBufferIndex = 0;

        status = OK;
    }

    return status;
}
#endif


/*------------------------------------------------------------------*/

static void
sha256_transform(SHA256_CTX *pContext, const ubyte M[SHA256_BLOCK_SIZE])
{
#ifdef __ENABLE_MOCANA_MINIMUM_STACK__
    ubyte4 *W = pContext->W;
#else
    ubyte4 W[64];
#endif
    sbyte4 t;
    ubyte4 a,b,c,d,e,f,g,h;
    ubyte4 T1,T2;

    /* message schedule */
    for (t = 0; t < 16; t+=4)
    {
#ifdef __ASM_386_GCC__
		HOST_c2l((M),W[t]);
		M+=4;
		HOST_c2l((M),W[t+1]);
		M+=4;
		HOST_c2l((M),W[t+2]);
		M+=4;
		HOST_c2l((M),W[t+3]);
		M+=4;
#else
        W[t]  = ((ubyte4)(*M++) << 24);
        W[t] |= ((ubyte4)(*M++) << 16);
        W[t] |= ((ubyte4)(*M++) << 8);
        W[t] |=  (ubyte4)(*M++);

		W[t+1]  = ((ubyte4)(*M++) << 24);
        W[t+1] |= ((ubyte4)(*M++) << 16);
        W[t+1] |= ((ubyte4)(*M++) << 8);
        W[t+1] |=  (ubyte4)(*M++);

		W[t+2]  = ((ubyte4)(*M++) << 24);
        W[t+2] |= ((ubyte4)(*M++) << 16);
        W[t+2] |= ((ubyte4)(*M++) << 8);
        W[t+2] |=  (ubyte4)(*M++);

		W[t+3]  = ((ubyte4)(*M++) << 24);
        W[t+3] |= ((ubyte4)(*M++) << 16);
        W[t+3] |= ((ubyte4)(*M++) << 8);
        W[t+3] |=  (ubyte4)(*M++);
#endif
#ifdef VERBOSE
        printf("%08x ", W[t]);
#endif
    }

    for (; t < 64; ++t)
    {
        W[t] = LSIG1( W[t-2]) + W[t-7] + LSIG0( W[t-15]) + W[t-16];
    }

    a = pContext->hashBlocks[0];
    b = pContext->hashBlocks[1];
    c = pContext->hashBlocks[2];
    d = pContext->hashBlocks[3];
    e = pContext->hashBlocks[4];
    f = pContext->hashBlocks[5];
    g = pContext->hashBlocks[6];
    h = pContext->hashBlocks[7];

    for (t = 0; t < 64; ++t)
    {
        T1 = h + BSIG1(e) + CH(e,f,g) + K[t] + W[t];
        T2 = BSIG0(a) + MAJ(a,b,c);
        h = g;
        g = f;
        f = e;
        e = d + T1;
        d = c;
        c = b;
        b = a;
        a = T1 + T2;
#ifdef VERBOSE
        printf("\n%d %08x %08x %08x %08x %08x %08x %08x %08x",
            t, a,b,c,d,e,f,g,h);
#endif
    }

    pContext->hashBlocks[0] += a;
    pContext->hashBlocks[1] += b;
    pContext->hashBlocks[2] += c;
    pContext->hashBlocks[3] += d;
    pContext->hashBlocks[4] += e;
    pContext->hashBlocks[5] += f;
    pContext->hashBlocks[6] += g;
    pContext->hashBlocks[7] += h;

#ifdef VERBOSE
    printf("\n%08x %08x %08x %08x %08x %08x %08x %08x\n",
            pContext->hashBlocks[0],
            pContext->hashBlocks[1],
            pContext->hashBlocks[2],
            pContext->hashBlocks[3],
            pContext->hashBlocks[4],
            pContext->hashBlocks[5],
            pContext->hashBlocks[6],
            pContext->hashBlocks[7]);

#endif

}


/*------------------------------------------------------------------*/

extern MSTATUS
SHA256_updateDigest(MOC_HASH(hwAccelDescr hwAccelCtx) SHA256_CTX *pContext,
                    const ubyte *pData, ubyte4 dataLen)
{
    MSTATUS status = OK;

#if( defined(__ENABLE_MOCANA_FIPS_MODULE__) )
   if (OK != getFIPS_powerupStatus(FIPS_ALGO_SHA256))
        return getFIPS_powerupStatus(FIPS_ALGO_SHA256);
#endif /* ( defined(__ENABLE_MOCANA_FIPS_MODULE__) ) */

    if ((NULL == pContext) || (NULL == pData))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    u8_Incr32(&pContext->mesgLength, dataLen);

    /* some remaining from last time ?*/
    if (pContext->hashBufferIndex > 0)
    {
        sbyte4 numToCopy = SHA256_BLOCK_SIZE - pContext->hashBufferIndex;
        if ( (sbyte4)dataLen < numToCopy)
        {
            numToCopy = dataLen;
        }

        MOC_MEMCPY( pContext->hashBuffer + pContext->hashBufferIndex, pData, numToCopy);
        pData += numToCopy;
        dataLen -= numToCopy;
        pContext->hashBufferIndex += numToCopy;
        if (SHA256_BLOCK_SIZE == pContext->hashBufferIndex)
        {
            sha256_transform( pContext, pContext->hashBuffer);
            pContext->hashBufferIndex = 0;
        }
    }

    /* process as much as possible right now */
    while ( SHA256_BLOCK_SIZE <= dataLen)
    {
        sha256_transform( pContext, pData);

        dataLen -= SHA256_BLOCK_SIZE;
        pData += SHA256_BLOCK_SIZE;
    }

    /* store the rest in the buffer */
    if (dataLen > 0)
    {
        MOC_MEMCPY(pContext->hashBuffer + pContext->hashBufferIndex, pData, dataLen);
        pContext->hashBufferIndex += dataLen;
    }

exit:
    return status;
}


/*------------------------------------------------------------------*/

static MSTATUS
SHA256_finalDigestAux(MOC_HASH(hwAccelDescr hwAccelCtx) SHA256_CTX *pContext,
                      ubyte *pOutput, sbyte4 outputSize)
{
    MSTATUS status = OK;
    sbyte4 i;

    if ((NULL == pContext) || (NULL == pOutput))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    /* we should have room to append one byte onto the message */
    pContext->hashBuffer[pContext->hashBufferIndex++] = 0x80;

    /* less than 8 bytes available -> extra round */
    if ( pContext->hashBufferIndex > SHA256_BLOCK_SIZE - 8)
    {
        while ( pContext->hashBufferIndex < SHA256_BLOCK_SIZE)
        {
            pContext->hashBuffer[pContext->hashBufferIndex++] = 0x00;
        }
        sha256_transform( pContext, pContext->hashBuffer);
        pContext->hashBufferIndex = 0;
    }

    /*last round */
    while ( pContext->hashBufferIndex < SHA256_BLOCK_SIZE - 8)
    {
        pContext->hashBuffer[pContext->hashBufferIndex++] = 0x00;
    }

    /* fill in message bit length */
    /* bytes to bits */
    pContext->mesgLength = u8_Shl( pContext->mesgLength, 3);
    BIGEND32(pContext->hashBuffer+SHA256_BLOCK_SIZE-8, HI_U8(pContext->mesgLength));
    BIGEND32(pContext->hashBuffer+SHA256_BLOCK_SIZE-4, LOW_U8(pContext->mesgLength));

    sha256_transform( pContext, pContext->hashBuffer);

    /* return the output */
    for (i = 0; i < outputSize/4; ++i)
    {
        BIGEND32( pOutput, pContext->hashBlocks[i]);
        pOutput += 4;
    }

exit:
#ifdef __ZEROIZE_TEST__
    {
        int counter;

        FIPS_PRINT("\nSHA256_finalDigestAux - Before Zeroization\n");
        for( counter = 0; counter < sizeof(SHA256_CTX); counter++)
        {
            FIPS_PRINT("%02x",*((ubyte*)pContext + counter));
        }
        FIPS_PRINT("\n");
    }
#endif

    /* Zeroize the sensitive information before deleting the memory */
    MOC_MEMSET((ubyte *)pContext, 0x00, sizeof(SHA256_CTX));

#ifdef __ZEROIZE_TEST__
    {
        int counter;

        FIPS_PRINT("\nSHA256_finalDigestAux - After Zeroization\n");
        for( counter = 0; counter < sizeof(SHA256_CTX); counter++)
        {
            FIPS_PRINT("%02x",*((ubyte*)pContext + counter));
        }
        FIPS_PRINT("\n");
    }
#endif

    return status;

} /* SHA256_finalDigestAux */


#ifndef __DISABLE_MOCANA_SHA256__
/*------------------------------------------------------------------*/

extern MSTATUS
SHA256_finalDigest(MOC_HASH(hwAccelDescr hwAccelCtx) SHA256_CTX *pContext, ubyte *pOutput)
{
#if( defined(__ENABLE_MOCANA_FIPS_MODULE__) )
   if (OK != getFIPS_powerupStatus(FIPS_ALGO_SHA256))
        return getFIPS_powerupStatus(FIPS_ALGO_SHA256);
#endif /* ( defined(__ENABLE_MOCANA_FIPS_MODULE__) ) */

    return SHA256_finalDigestAux( MOC_HASH(hwAccelCtx) pContext, pOutput,
                                    SHA256_RESULT_SIZE);
} /* SHA256_finalDigest */
#endif


#ifndef __DISABLE_MOCANA_SHA224__
/*------------------------------------------------------------------*/

extern MSTATUS
SHA224_finalDigest(MOC_HASH(hwAccelDescr hwAccelCtx) SHA224_CTX *pContext, ubyte *pOutput)
{
    return SHA256_finalDigestAux( MOC_HASH(hwAccelCtx) pContext, pOutput,
                                    SHA224_RESULT_SIZE);
} /* SHA224_finalDigest */
#endif

/*------------------------------------------------------------------*/

#if !defined( __SHA256_ONE_STEP_HARDWARE_HASH__) && !defined(__DISABLE_MOCANA_SHA256__)

extern MSTATUS
SHA256_completeDigest(MOC_HASH(hwAccelDescr hwAccelCtx) const ubyte *pData, ubyte4 dataLen, ubyte *pShaOutput)
{
    SHA256_CTX context;
    MSTATUS  status;
#ifdef __ZEROIZE_TEST__
    int counter = 0;
#endif

#if( defined(__ENABLE_MOCANA_FIPS_MODULE__) )
   if (OK != getFIPS_powerupStatus(FIPS_ALGO_SHA256))
        return getFIPS_powerupStatus(FIPS_ALGO_SHA256);
#endif /* ( defined(__ENABLE_MOCANA_FIPS_MODULE__) ) */

    if (OK > (status = SHA256_initDigest(MOC_HASH(hwAccelCtx) &context)))
        goto exit;

    if (OK > (status = SHA256_updateDigest(MOC_HASH(hwAccelCtx) &context, pData, dataLen)))
        goto exit;

    status = SHA256_finalDigest(MOC_HASH(hwAccelCtx) &context, pShaOutput);

exit:
    return status;
}

#endif /* __SHA256_ONE_STEP_HARDWARE_HASH__ */

/*------------------------------------------------------------------*/

#if !defined (__SHA224_ONE_STEP_HARDWARE_HASH__) && !defined(__DISABLE_MOCANA_SHA224__)

extern MSTATUS
SHA224_completeDigest(MOC_HASH(hwAccelDescr hwAccelCtx) const ubyte *pData, ubyte4 dataLen, ubyte *pShaOutput)
{
    SHA224_CTX context;
    MSTATUS  status;

    if (OK > (status = SHA224_initDigest(MOC_HASH(hwAccelCtx) &context)))
        goto exit;

    if (OK > (status = SHA224_updateDigest(MOC_HASH(hwAccelCtx) &context, pData, dataLen)))
        goto exit;

    status = SHA224_finalDigest(MOC_HASH(hwAccelCtx) &context, pShaOutput);

exit:
    return status;
}

#endif /* __SHA224_ONE_STEP_HARDWARE_HASH__ */

#endif /* __SHA256_HARDWARE_HASH__ */

#ifdef TEST

#include <stdlib.h>
#include <stdio.h>

/*
 * those are the standard FIPS-180-2 test vectors
 */

static char *msg[] =
{
    "abc",
    "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
    NULL
};

static char *val256[] =
{
    "ba7816bf8f01cfea414140de5dae2223" \
    "b00361a396177a9cb410ff61f20015ad",
    "248d6a61d20638b8e5c026930c3e6039" \
    "a33ce45964ff2167f6ecedd419db06c1",
    "cdc76e5c9914fb9281a1c7e284d73e67" \
    "f1809a48a497200e046d39ccc7112cd0"
};

static char *val224[] =
{
    "23097d223405d8228642a477bda255b32aadbce4bda0b3f7e36c9da7",
    "75388b16512776cc5dba5da1fd890150b0c6455cb4f58b1952522525",
    "20794655980c91d8bbb4c1ea97618a4bf03f42581948b2ee4ee7ad67"
};

#include <string.h>

static int Compare( ubyte* res, int resSize, char* expectedVal)
{
    int i;
    char output[65];

    for( i = 0; i < resSize; i++ )
    {
        sprintf( output + i * 2, "%02x", res[i] );
    }

    return  strcmp(expectedVal, output) ? 1 : 0;
}

int TestSHA224()
{
    SHA224_CTX ctx;
    int i, retVal = 0;
    ubyte result[SHA224_RESULT_SIZE];
    char buf[1000];

    for (i = 0; i < 2; ++i)
    {
         SHA224_completeDigest( msg[i], MOC_STRLEN( msg[i]), result);
         retVal += Compare(result, sizeof(result), val224[i]);
    }

    SHA224_initDigest( &ctx);
    memset( buf, 'a', 1000 );

    for( i = 0; i < 1000; i++ )
    {
        SHA224_updateDigest( &ctx, buf, 1000 );
    }
    SHA224_finalDigest( &ctx, result);

    retVal += Compare(result, sizeof(result), val224[2]);

    return( retVal );
}


int TestSHA256()
{
    SHA256_CTX ctx;
    int i, retVal = 0;
    ubyte result[SHA256_RESULT_SIZE];
    char buf[1000];

    for (i = 0; i < 2; ++i)
    {
         SHA256_completeDigest( msg[i], MOC_STRLEN( msg[i]), result);
         retVal += Compare(result, sizeof(result), val256[i]);
    }

    SHA256_initDigest( &ctx);
    memset( buf, 'a', 1000 );

    for( i = 0; i < 1000; i++ )
    {
        SHA256_updateDigest( &ctx, buf, 1000 );
    }
    SHA256_finalDigest( &ctx, result);

    retVal += Compare(result, sizeof(result), val256[2]);

    return( retVal );
}

#endif

#endif /* __DISABLE_MOCANA_SHA256__ */
