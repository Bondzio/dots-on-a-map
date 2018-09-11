/* Version: mss_v6_3 */
/*
 * dsa2.c
 *
 * DSA
 *
 * Copyright Mocana Corp 2010. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

#include "../common/moptions.h"

#if (defined(__ENABLE_MOCANA_DSA__))
#include "../common/mtypes.h"
#include "../common/mocana.h"
#include "../crypto/hw_accel.h"
#include "../common/mdefs.h"
#include "../common/merrors.h"
#include "../common/mrtos.h"
#include "../common/mstdlib.h"
#include "../common/vlong.h"
#include "../common/random.h"
#include "../crypto/dsa.h"
#include "../crypto/dsa2.h"
#ifdef __ENABLE_MOCANA_FIPS_MODULE__
#include "../crypto/fips.h"
#endif


/*--------------------------------------------------------------------------*/

extern MSTATUS
DSA_computeSignature2(MOC_DSA(hwAccelDescr hwAccelCtx)
                      RNGFun rngFun, void* rngArg,
                      const DSAKey *p_dsaDescr,
                      const ubyte* msg, ubyte4 msgLen,
                      vlong **ppR, vlong **ppS,
                      vlong **ppVlongQueue)
{
    MSTATUS status;
    ubyte4 qLen;
    vlong* m = 0;

    if (!rngFun || !p_dsaDescr || !msg || !ppR || !ppS)
        return ERR_NULL_POINTER;

    /* get the length of Q */
    qLen = (VLONG_bitLength( DSA_Q( p_dsaDescr)) + 7) / 8;

    /* truncate the message to qLen if necessary */
    if (qLen < msgLen)
    {
        msgLen = qLen;
    }

    if (OK > ( status = VLONG_vlongFromByteString( msg, msgLen, &m, ppVlongQueue)))
        goto exit;

    if (OK > ( status = DSA_computeSignatureEx( MOC_DSA(hwAccelCtx)
                                                rngFun, rngArg, p_dsaDescr, m,
                                                NULL, ppR, ppS, ppVlongQueue)))
    {
        goto exit;
    }

exit:

    VLONG_freeVlong( &m, ppVlongQueue);

    return status;
}


/*--------------------------------------------------------------------------*/

extern MSTATUS
DSA_verifySignature2(MOC_DSA(hwAccelDescr hwAccelCtx)
                       const DSAKey *p_dsaDescr,
                       const ubyte *msg, ubyte4 msgLen,
                       vlong *pR, vlong *pS,
                       intBoolean *isGoodSignature,
                       vlong **ppVlongQueue)
{
    MSTATUS status;
    ubyte4 qLen;
    vlong* m = 0;

    if (!p_dsaDescr || !msg || !pR || !pS || !isGoodSignature )
        return ERR_NULL_POINTER;

    /* get the length of Q */
    qLen = (VLONG_bitLength( DSA_Q( p_dsaDescr)) + 7) / 8;

    /* truncate the message to qLen if necessary */
    if (qLen < msgLen)
    {
        msgLen = qLen;
    }

    if (OK > ( status = VLONG_vlongFromByteString( msg, msgLen, &m, ppVlongQueue)))
        goto exit;

    if ( OK > (status = DSA_verifySignature( p_dsaDescr, m, pR, pS,
                                            isGoodSignature, ppVlongQueue)))
    {
        goto exit;
    }

exit:

    VLONG_freeVlong( &m, ppVlongQueue);

    return status;
}


#endif /* (defined(__ENABLE_MOCANA_DSA__)) */
