/* Version: mss_v6_3 */
/*
 * dh.h
 *
 * Diffie-Hellman Key Exchange
 *
 * Copyright Mocana Corp 2003-2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

/* \file dh.h Diffie-Hellman Key Exchange developer API header.
This header file contains definitions, enumerations, structures, and function
declarations used for Diffie-Hellman Key Exchange operations.

\since 1.41
\version 3.06 and later

! Flags
There are no flag dependencies to enable the functions in this header file.

! External Functions
This file contains the following public ($extern$) functions:
- DH_getG
- DH_getP
- DH_allocate
- DH_allocateServer
- DH_allocateClient
- DH_setPG
- DH_freeDhContext
- DH_computeKeyExchange

*/

/*------------------------------------------------------------------*/

#ifndef __KEYEX_DH_HEADER__
#define __KEYEX_DH_HEADER__

#ifdef __cplusplus
extern "C" {
#endif

#define DH_GROUP_TBD                    0
#define DH_GROUP_1                      1
#define DH_GROUP_2                      2
#define DH_GROUP_5                      5
#define DH_GROUP_14                     14
#define DH_GROUP_15                     15
#define DH_GROUP_16                     16
#define DH_GROUP_17                     17
#define DH_GROUP_18                     18
#define DH_GROUP_24                     24

#define COMPUTED_VLONG_G(X)             (X)->dh_g
#define COMPUTED_VLONG_Y(X)             (X)->dh_y
#define COMPUTED_VLONG_F(X)             (X)->dh_f
#define COMPUTED_VLONG_E(X)             (X)->dh_e
#define COMPUTED_VLONG_K(X)             (X)->dh_k
#define COMPUTED_VLONG_P(X)             (X)->dh_p

/* for SSH context */
#define DIFFIEHELLMAN_CONTEXT(X)        (X)->p_dhContext


/*------------------------------------------------------------------*/

typedef struct diffieHellmanContext
{
    vlong*  dh_g;           /* generator */
    vlong*  dh_p;           /* big prime */
    vlong*  dh_y;           /* random number - private key */
    vlong*  dh_f;           /* sent by the server - public key */
    vlong*  dh_e;           /* sent by the client - public key */
    vlong*  dh_k;           /* shared secret */
    ubyte*  pHashH;         /* hashed result */

} diffieHellmanContext;


/*------------------------------------------------------------------*/

MOC_EXTERN MSTATUS DH_getG(ubyte4 groupNum, vlong **ppRetG, vlong **ppVlongQueue);
MOC_EXTERN MSTATUS DH_getP(ubyte4 groupNum, vlong **ppRetP);
MOC_EXTERN MSTATUS DH_allocate(diffieHellmanContext **pp_dhContext);
MOC_EXTERN MSTATUS DH_allocateServer(MOC_DH(hwAccelDescr hwAccelCtx) randomContext *pRandomContext, diffieHellmanContext **pp_dhContext, ubyte4 groupNum);
MOC_EXTERN MSTATUS DH_allocateClient(MOC_DH(hwAccelDescr hwAccelCtx) randomContext *pRandomContext, diffieHellmanContext **pp_dhContext, ubyte4 groupNum);
MOC_EXTERN MSTATUS DH_setPG(MOC_DH(hwAccelDescr hwAccelCtx) randomContext *pRandomContext, ubyte4 lengthY, diffieHellmanContext *p_dhContext, const vlong *P, const vlong *G);
MOC_EXTERN MSTATUS DH_freeDhContext(diffieHellmanContext **pp_dhContext, vlong **ppVlongQueue);
MOC_EXTERN MSTATUS DH_computeKeyExchange(MOC_DH(hwAccelDescr hwAccelCtx) diffieHellmanContext *p_dhContext, vlong** ppVlongQueue);

#ifdef __cplusplus
}
#endif

#endif /* __KEYEX_DH_HEADER__ */
