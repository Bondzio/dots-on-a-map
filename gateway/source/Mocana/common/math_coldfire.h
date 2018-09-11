/* Version: mss_v6_3 */
/*
 * math_coldfire.h
 *
 * Inline assembly macros for Diab Compiler & Coldfire Processor
 *
 * Copyright Mocana Corp 2003-2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */


/*------------------------------------------------------------------*/

/* If compilation fails, due to this inline assembly macro */
/* verify that the caller has not changed the argument */
/* list.  If the argument list has changed, the first '%' line */
/* will need to be modified,to reflect this change. JAB */

asm void shiftRightByOne (unsigned int *pArray, unsigned int numUnits)
{
% mem pArray; mem numUnits; lab shiftAgain; lab shiftClear; lab shiftDone;
! "a0", "a1", "d0", "d1"
                move.l  pArray, a0
                move.l  numUnits, d0
                eor.l   d1, d1
                lsl.l   #2, d0      /* multiply by 4 */
shiftAgain:
                or.l    d0, d0
                beq.b   shiftDone
                subq.l  #4, d0

                move.l  d1, a1
                move.l  (a0,d0), d1
                lsr.l   #1, d1
                bcc.b   shiftClear

                move.l  d1, (a0,d0)
                move.l  a1, d1
                add.l   d1, (a0,d0)
                move.l  #0x80000000, d1
                bra.b   shiftAgain
shiftClear:
                move.l  d1, (a0,d0)
                move.l  a1, d1
                add.l   d1, (a0,d0)
                eor.l   d1, d1
                bra.b   shiftAgain
shiftDone:
}

#define MACRO_SHIFT_RIGHT(P,N)  shiftRightByOne(P,N)


/*------------------------------------------------------------------*/

#define I_LIMIT         8(a7)
#define J_LIMIT         4(a7)
#define X_LIMIT         (a7)
#define X               a0
#define I               a1
#define J               a2
#define FACTOR_I        a4
#define FACTOR_J        a5
#define R0              d0
#define R1              d1
#define R2              d2

asm void multLoop(ubyte4 *pResult, ubyte4 *pFactorI, ubyte4 *pFactorJ, ubyte4 i_limit, ubyte4 j_limit, ubyte4 x_limit)
{
% mem pResult; mem pFactorI; mem pFactorJ; mem i_limit; mem j_limit; mem x_limit;
! "a0", "a1", "d0", "d1"
                lea     -40(a7), a7
                movem.l d2-d7/a2-a5,(a7)

                move.l  i_limit, d0
                move.l  d0, -(a7)
                move.l  j_limit, d0
                move.l  d0, -(a7)
                move.l  x_limit, d0
                move.l  d0, -(a7)

                move.l  pFactorI, d0
                move.l  pFactorJ, d1
                move.l  d0, FACTOR_I
                move.l  d1, FACTOR_J
                move.l  #16, d7

                eor.l   R0, R0              /* result0 = 0 */
                eor.l   R1, R1              /* result1 = 0 */
                eor.l   R2, R2              /* result2 = 0 */

                /* note:  */
                /* a0 = x */
                /* a1 = i */
                /* a2 = j */
                /* a4 = pFactorI */
                /* a5 = pFactorJ */

                move.l  R0, X               /* x = 0 */

                /*!!!! note: unraveling this loop should improve performance (~5%) */
multLoopTop:
                move.l  X_LIMIT, a3
                cmp.l   X, a3               /* compare x, x_limit */
                ble.w   multLoopExit        /* quit, if x_limit <= x */

                move.l  X, I                /* i = x */
                move.l  I_LIMIT, a3
                cmp.l   X, a3               /* compare x, i_limit */
                bge.b   multLoopJ           /* branch, if i_limit >= x */
                move.l  a3, I               /* i = i_limit */

multLoopJ:
                move.l  X, J                /* j = x */
                sub.l   I, J                /* j = x - i */

                move.l  J_LIMIT, a3
                cmp.l   X, a3               /* compare x, j_limit */
                blt.b   multLoopWhileTop    /* branch, if j_limit < x */
                move.l  X, a3

multLoopWhileTop:
                cmp.l   a3, J               /* compare ((x <= j_limit) ? x : j_limit), j */
                bgt.b   multLoopWhileEnd

multLoopWhile1:
                /* result2:result1:result0 += pFactorI->pArrayUnits[i] * pFactorJ->pArrayUnits[j]; */
                /* ---------------------------------------- */
                /* --- INNER LOOP MULTIPLICATION START ---- */
                move.l  I, d5
                move.l  (a4,d5*4), d3
                move.l  J, d5
                move.l  (a5,d5*4), d4
                move.l  d3, d5              /* d5 = x */
                move.l  d3, d6              /* d6 = low(x)  = x0 */
                lsr.l   d7, d5              /* d5 = high(x) = x1 */
                mulu.w  d4, d3              /* d3 = (x0 * y0) */
                add.l   d3, R0
                moveq.l #0, d3
                addx.l  d3, R1
                addx.l  d3, R2

                move.l  d5, d3              /* d3 = high(x) = x1 */
                mulu.w  d4, d5              /* d5 = (x1 * y0) */
                lsr.l   d7, d4              /* d4 = high(y) = y1 */
                mulu.w  d4, d6              /* d6 = (x0 * y1) */
                add.l   d5, d6              /* d6 = (x0 * y1) + (x1 * y0) */
                bcc.s   skipMultCarry
                add.l   #0x10000, R1        /* add "carry bit" to R1 */
                moveq.l #0, d5
                addx.l  d5, R2

skipMultCarry:
                move.l  d6, d5              /* d5 = (x0 * y1) + (x1 * y0) */
                mulu.w  d4, d3              /* d3 = (x1 * y1) */
                add.l   d3, R1
                moveq.l #0, d3
                addx.l  d3, R2

                lsr.l   d7, d5              /* d5 = high((x0 * y1) + (x1 * y0)) */
                lsl.l   d7, d6              /* d6 = low((x0 * y1) + (x1 * y0)) */
                add.l   d5, R1
                addx.l  d3, R2

                add.l   d6, R0
                addx.l  d3, R1
                addx.l  d3, R2
                /* --- INNER LOOP MULTIPLICATION END ---- */
                /* -------------------------------------- */

                subq.l  #1, I              /* i-- */
                addq.l  #1, J              /* j++ */

                cmp.l   a3, J              /* compare ((x <= j_limit) ? x : j_limit), j */
                ble.b   multLoopWhile1

multLoopWhileEnd:
                move.l  pResult, a3
                move.l  X, d6
                move.l  R0, (a3,d6*4)       /* pResult = result0; pResult++; */

                move.l  R1, R0              /* result0 = result1 */
                move.l  R2, R1              /* result1 = result2 */
                eor.l   R2, R2              /* result2 = 0 */

                addq.l  #1, X               /* x++ */
                bra.w   multLoopTop
multLoopExit:
                lea     12(a7), a7
                movem.l (a7),d2-d7/a2-a5
                lea     40(a7), a7
}

#define MACRO_MULTIPLICATION_LOOP(P,A,B,I,J,X)      multLoop(P,A,B,I,J,X)

#undef I_LIMIT
#undef J_LIMIT
#undef X_LIMIT
#undef X
#undef I
#undef J
#undef FACTOR_I
#undef FACTOR_J
#undef R0
#undef R1
#undef R2

