/* Version: mss_v6_3 */
/*
 * ocsp.c
 *
 * OCSP -- Online Certificate Security Protocol
 *
 * Copyright Mocana Corp 2005-2008. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

#include "../common/moptions.h"

#if (defined(__ENABLE_MOCANA_OCSP_CLIENT__))

#include "../common/mtypes.h"
#include "../common/mocana.h"
#include "../crypto/hw_accel.h"

#include "../common/mdefs.h"
#include "../common/merrors.h"
#include "../crypto/secmod.h"
#include "../common/mstdlib.h"
#include "../common/tree.h"
#include "../common/absstream.h"
#include "../common/memfile.h"
#include "../common/vlong.h"
#include "../common/random.h"
#include "../common/mrtos.h"
#include "../asn1/oiddefs.h"
#include "../crypto/crypto.h"
#include "../crypto/primefld.h"
#include "../crypto/primeec.h"
#include "../crypto/pubcrypto.h"
#include "../crypto/ca_mgmt.h"
#include "../asn1/parseasn1.h"
#include "../ocsp/ocsp.h"


/*------------------------------------------------------------------*/

static ocspSettings mOcspSettings;

/*------------------------------------------------------------------*/

extern ocspSettings*
OCSP_ocspSettings(void)
{
    return &mOcspSettings;
}

#endif /* #if (defined(__ENABLE_MOCANA_OCSP_CLIENT__))  */
