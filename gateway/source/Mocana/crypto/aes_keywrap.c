/* Version: mss_v6_3 */
/*
 * aes_keywrap.c
 *
 * AES-Key Wrap Implementation (RFC 3394)
 *
 * Copyright Mocana Corp 2006. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */


/*------------------------------------------------------------------*/

#include "../common/moptions.h"
#include "../common/mtypes.h"
#include "../common/mocana.h"
#include "../crypto/hw_accel.h"

#if (!defined(__DISABLE_AES_CIPHERS__))

#include "../common/mdefs.h"
#include "../common/merrors.h"
#include "../common/mrtos.h"
#include "../common/mstdlib.h"
#include "../common/debug_console.h"
#include "../crypto/aesalgo.h"
#include "../crypto/aes.h"
#include "../crypto/aes_keywrap.h"


/*------------------------------------------------------------------*/
static ubyte IV[] = {0xA6,0xA6,0xA6,0xA6,0xA6,0xA6,0xA6,0xA6};

/*------------------------------------------------------------------*/

extern MSTATUS
AESKWRAP_encrypt( MOC_SYM(hwAccelDescr hwAccelCtx) ubyte* keyMaterial,sbyte4 keyLength,
                  const ubyte* data, ubyte4 dataLen, ubyte * retData/* SHould be dataLen + 8 */)
{
    ubyte4 i,j;
    ubyte A[8];
    ubyte4 t = 0;
    ubyte4 n = dataLen / 8;
    ubyte* R = NULL;
    ubyte* P = (ubyte *)data;
    ubyte* C = NULL;
    ubyte  AR[16] ;
    BulkCtx ctx = NULL;
    MSTATUS status = OK;
    ubyte   aes_iv[16];

    if (!retData)
    {
        status = ERR_AES_BAD_ARG;
        goto exit;
    }

    if (dataLen % 8)
    {
        status = ERR_AES_BAD_LENGTH;
        goto exit;
    }

    ctx = CreateAESCtx(MOC_SYM(hwAccelCtx)(ubyte *) keyMaterial,keyLength, 1);
    if (!ctx)
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    C = retData;
    R = retData;

    /*Set A = IV, an initial value (see 2.2.3) */
    MOC_MEMCPY(A,IV,8);

 /*      For i = 1 to n
           R[i] = P[i]  */

    MOC_MEMCPY(R+8,P,n * 8);

  /*
   2) Calculate intermediate values.

       For j = 0 to 5
           For i=1 to n
               B = AES(K, A | R[i])
               A = MSB(64, B) ^ t where t = (n*j)+i
               R[i] = LSB(64, B)
   */
    for (j = 0; j < 6 ; j++)
    {
        for (i = 1; i < n + 1; i++)
        {
            MOC_MEMCPY(AR, A, 8);
            MOC_MEMCPY(AR+8, &R[i * 8], 8);
            MOC_MEMSET(aes_iv, 0, 16);
            DoAES(MOC_SYM(hwAccelCtx)ctx, AR, 16 , 1, aes_iv);
            t = (n * j) + i;
            MOC_MEMCPY(A, AR, 8);
            A[7] = A[7] ^ t;
            MOC_MEMCPY(&R[i * 8], AR + 8, 8);
        }

    }

   /*
   3) Output the results.

       Set C[0] = A
       For i = 1 to n
           C[i] = R[i]
    */
    MOC_MEMCPY(C, A, 8);
   /* R already Points to C  */

exit:
    if (ctx)
        DeleteAESCtx(MOC_SYM(hwAccelCtx)&ctx);

    return status;

}


/*------------------------------------------------------------------*/

extern MSTATUS
AESKWRAP_decrypt(MOC_SYM(hwAccelDescr hwAccelCtx) ubyte* keyMaterial,sbyte4 keyLength,
                 const ubyte* data, ubyte4 dataLen, ubyte * retData /* datalen - 8 */ )
{
    sbyte4 i,j;
    ubyte A[8];
    sbyte4 cmp;
    ubyte4 t = 0;
    ubyte4 n = dataLen / 8;
    ubyte* R = retData;
    ubyte* C = (ubyte *)data;
    ubyte  AR[16];
    BulkCtx ctx = NULL;
    MSTATUS status = OK;
    ubyte   aes_iv[16];

    n--;

    if (dataLen < 16)
    {
        status = ERR_AES_BAD_LENGTH;
        goto exit;
    }

    if (!retData)
    {
        status = ERR_AES_BAD_ARG;
        goto exit;
    }

    if (dataLen % 8)
    {
        status = ERR_AES_BAD_LENGTH;
        goto exit;
    }

    ctx = CreateAESCtx(MOC_SYM(hwAccelCtx)(ubyte *) keyMaterial,keyLength, 0);
    if (!ctx)
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }
  /*
    1) Initialize variables.

       Set A = C[0]
       For i = 1 to n
           R[i] = C[i] */

    MOC_MEMCPY(A, C, 8);
    MOC_MEMCPY(R, C + 8, (n* 8));


   /*
   2) Compute intermediate values.

       For j = 5 to 0
           For i = n to 1
               B = AES-1(K, (A ^ t) | R[i]) where t = n*j+i
               A = MSB(64, B)
               R[i] = LSB(64, B) */

    for (j = 5; j >=0 ; j--)
    {
        for (i = n; i>= 1; i--)
        {
            t = (n * j) + i;
            A[7] = A[7] ^ t;

            MOC_MEMCPY(AR, A, 8);
            MOC_MEMCPY(AR+8, &R[(i - 1) * 8], 8);
            MOC_MEMSET(aes_iv, 0, 16);
            DoAES(MOC_SYM(hwAccelCtx)ctx, AR, 16 , 0, aes_iv);
            MOC_MEMCPY(A, AR, 8);
            MOC_MEMCPY(&R[(i -1 ) * 8], AR + 8, 8);
        }

    }

   /*

   3) Output results.

   If A is an appropriate initial value (see 2.2.3),

   Then
       For i = 1 to n
           P[i] = R[i]
   Else
       Return an error
  */

    MOC_MEMCMP(A,IV,8,&cmp);
    if (cmp)
    {
        status = ERR_AES_BAD_KEY_MATERIAL;
        goto exit;
    }

exit:

    if (ctx)
        DeleteAESCtx(MOC_SYM(hwAccelCtx)&ctx);

    return status;
}



#endif /* (!defined(__DISABLE_AES_CIPHERS__) ) */
