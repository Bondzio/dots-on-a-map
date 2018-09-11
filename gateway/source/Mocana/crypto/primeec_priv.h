/* Version: mss_v6_3 */
/*
 * primeec_priv.h
 *
 * Prime Field Elliptic Curve Cryptography -- Private data types definitions
 *
 * Copyright Mocana Corp 2009. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

#ifndef __PRIMEEC_PRIV_HEADER__
#define __PRIMEEC_PRIV_HEADER__

#if defined(__ENABLE_MOCANA_ECC__)

MSTATUS EC_modOrder( PEllipticCurvePtr pEC, PFEPtr x);

/* NIST curves */
struct PrimeEllipticCurve
{
    PrimeFieldPtr       pPF;
    ConstPFEPtr         pPx;  /* point */
    ConstPFEPtr         pPy;
    ConstPFEPtr         b;    /* b parameter, a = -3 */
    ConstPFEPtr         n;    /* order of point */
    ConstPFEPtr         mu;   /* special barrett constant */
};


#endif
#endif

