/* Version: mss_v6_3 */
/*
 * primefld_priv.h
 *
 * Prime Field Arithmetic -- Private data types definitions
 *
 * Copyright Mocana Corp 2006-2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

#ifndef __PRIMEFLD_PRIV_HEADER__
#define __PRIMEFLD_PRIV_HEADER__

#if (defined(__ENABLE_MOCANA_ECC__) /*|| defined(__ENABLE_MOCANA_RSA_SIMPLE__)*/ )


/* we should typedef the "word" used to represent numbers in the prime field */
/* the code is not cleaned up yet (i.e. ubyte4 is used instead of pf_unit ) */
#ifdef __ENABLE_MOCANA_64_BIT__
typedef ubyte8 pf_unit;
#else
typedef ubyte4 pf_unit;
#endif


/* Macros for doing double precision multiply */
#define BPU     (8 * sizeof(pf_unit))        /* number of bits in pf_unit */
#define HALF_MASK         (((pf_unit)1) << (BPU-1))
#define LO_MASK           ((((pf_unit)1)<<(BPU/2))-1)
#define HI_MASK           (~(LO_MASK))
#define LO_HUNIT(x)       ((x) & LO_MASK)             /* lower half */
#define HI_HUNIT(x)       ((x) >> (BPU/2))            /* upper half */
#define MAKE_HI_HUNIT(x)  (((pf_unit)(x)) << (BPU/2))  /* make upper half */
#define MAKE_UNIT(h,l)    (MAKE_HI_HUNIT(h) | ((pf_unit)(l)))
#define ZERO_UNIT         ((pf_unit)0)
#define FULL_MASK         (~ZERO_UNIT)

#if defined(__ENABLE_MOCANA_RSA_SIMPLE__)
/* use with extreme caution. Buffers must have the proper size k
and k + 1 for pN and pMu */
MSTATUS BI_modExp( sbyte4 k, pf_unit* pResult, const pf_unit* pN,
           ubyte4 e, const pf_unit* pModulo, const pf_unit* pMu);
MSTATUS BI_modExpEx( sbyte4 k, pf_unit pResult[/*k*/], const pf_unit pN[/*2*k*/],
           sbyte4 eLen, const pf_unit pE[/*eLen*/],
           const pf_unit pModulo[/*k+1*/], const pf_unit pMu[/*k+1*/]);
MSTATUS BI_barrettMu( sbyte4 k, pf_unit mu[/*k+1*/],
                     const pf_unit modulus[/*k+1*/]);
sbyte4 BI_cmp(sbyte4 n, const pf_unit* a, const pf_unit* b);
pf_unit BI_add( sbyte4 n, pf_unit* a_s, const pf_unit* b);
pf_unit BI_sub( sbyte4 n, pf_unit* a_s, const pf_unit* b);
void BI_mul( sbyte4 n, pf_unit hilo[/*2*n*/],
             const pf_unit a[/*n*/], const pf_unit b[/*n*/],
             sbyte4 x_limit);
void BI_barrettReduction( sbyte4 n,
                    pf_unit c[/*2*n*/],
                    pf_unit r[/*n*/],
                    pf_unit mulBuffer[/*2*n+2*/],
                    const pf_unit mu[/*n+1*/],
                    const pf_unit m[/*n+1*/]);
MSTATUS BI_modularInverse(sbyte4 n, const pf_unit a[/*n*/],
                            const pf_unit m[/*n*/],
                            pf_unit inv[/*n*/]);



#endif

void BI_setUnitsToByteString( sbyte4 n, pf_unit* a,
                               const ubyte* b, sbyte4 bLen);
void BI_shiftREx( sbyte4 n, pf_unit* a_s, sbyte4 shift);



/*------------------------------------------------------------------------*/
/* Multiplication routines */
MSTATUS PRIMEFIELD_multiplyAux( PrimeFieldPtr pField, PFEPtr pProduct,
                        ConstPFEPtr pA, ConstPFEPtr pB, pf_unit* hilo);
MSTATUS PRIMEFIELD_squareAux( PrimeFieldPtr pField, PFEPtr pProduct,
                        ConstPFEPtr pA, pf_unit* hilo);

/*---------------------------------------------------------------------------*/

/* data types */

typedef void (*PRIMEFIELD_reduceFun)( const pf_unit* toReduce,
                                         pf_unit* reduce,
                                         PrimeFieldPtr pField);

/*---------------------------------------------------------------------------*/

struct PrimeField
{
    const pf_unit*          units;
    sbyte4                  n;
    ubyte4                  numBits;
    PRIMEFIELD_reduceFun    reduceFun;      /* to reduce a big number */
};

struct PFE
{
    pf_unit  units[1];
};


#endif
#endif

