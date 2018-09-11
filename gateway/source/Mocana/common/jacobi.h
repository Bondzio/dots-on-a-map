/* Version: mss_v6_3 */
/*
 * jacobi.h
 *
 * Jacobi Symbol Header
 *
 * Copyright Mocana Corp 2008. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

#ifndef __JACOBI_HEADER__
#define __JACOBI_HEADER__


/*------------------------------------------------------------------*/

MOC_EXTERN MSTATUS JACOBI_jacobiSymbol(MOC_MOD(hwAccelDescr hwAccelCtx) const vlong *a, const vlong *p, sbyte4 *pRetJacobiResult, vlong **ppVlongQueue);

#endif /* __JACOBI_HEADER__ */
