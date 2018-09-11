/*
*********************************************************************************************************
*                                             uC/FS V4
*                                     The Embedded File System
*
*                         (c) Copyright 2008-2014; Micrium, Inc.; Weston, FL
*
*                  All rights reserved.  Protected by international copyright laws.
*
*                  uC/FS is provided in source form to registered licensees ONLY.  It is
*                  illegal to distribute this source code to any third party unless you receive
*                  written permission by an authorized Micrium representative.  Knowledge of
*                  the source code may NOT be used to develop a similar product.
*
*                  Please help us continue to provide the Embedded community with the finest
*                  software available.  Your honesty is greatly appreciated.
*
*                  You can find our product's user manual, API reference, release notes and
*                  more information at: https://doc.micrium.com
*
*                  You can contact us at: http://www.micrium.com
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                  FILE SYSTEM SUITE UTILITY LIBRARY
*
* Filename      : fs_util.c
* Version       : v4.07.00
* Programmer(s) : EJ
*                 BAN
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#define    MICRIUM_SOURCE
#define    FS_UTIL_MODULE
#include  <cpu_core.h>
#include  <lib_def.h>
#include  <fs.h>
#include  <fs_util.h>


/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                           LOCAL CONSTANTS
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                          LOCAL DATA TYPES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                            LOCAL TABLES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                     LOCAL CONFIGURATION ERRORS
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                         INTERNAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                            FSUtil_Log2()
*
* Description : Calculate ceiling of base-2 logarithm of integer.
*
* Argument(s) : val         Integer value.
*
* Return(s)   : Logarithm of value, if val > 0;
*               0,                  otherwise.
*
* Caller(s)   : various.
*
*               This function is an INTERNAL file system suite function & MUST NOT be called by
*               application function(s).
*
* Note(s)     : none.
*********************************************************************************************************
*/

CPU_INT08U  FSUtil_Log2 (CPU_INT32U  val)
{
    CPU_SIZE_T  log_val;
    CPU_SIZE_T  val_cmp;


    val_cmp = 1u;
    log_val = 0u;
    while (val_cmp < val) {
        val_cmp *= 2u;
        log_val += 1u;
    }

    return (log_val);
}


/*
*********************************************************************************************************
*                                            FSUtil_ValPack32()
*
* Description : Packs a specified number of least significant bits of a 32-bit value to a specified octet
*               and bit position within an octet array, in little-endian order.
*
* Argument(s) : p_dest          Pointer to destination octet array.
*
*               p_offset_octet  Pointer to octet offset into 'p_dest'. This function adjusts the pointee
*                               to the new octet offset within octet array.
*
*               p_offset_bit    Pointer to bit offset into initial 'p_dest[*p_offset_octet]'. This function
*                               ajusts the pointee to the new bit offset within octet.
*
*               val             Value to pack into 'p_dest' array.
*
*               nbr_bits        Number of least-significants bits of 'val' to pack into 'p_dest'.
*
* Return(s)   : none.
*
* Caller(s)   : various.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  FSUtil_ValPack32 (CPU_INT08U  *p_dest,
                        CPU_SIZE_T  *p_offset_octet,
                        CPU_DATA    *p_offset_bit,
                        CPU_INT32U   val,
                        CPU_DATA     nbr_bits)
{
    CPU_INT32U  val_32_rem;
    CPU_DATA    nbr_bits_rem;
    CPU_DATA    nbr_bits_partial;
    CPU_INT08U  val_08;
    CPU_INT08U  val_08_mask;
    CPU_INT08U  dest_08_mask;
    CPU_INT08U  dest_08_mask_lsb;


#if (FS_CFG_ERR_ARG_CHK_DBG_EN == DEF_ENABLED)                  /* ------------------ VALIDATE ARGS ------------------- */
    if (p_dest == DEF_NULL) {                                   /* Validate dest  array  ptr.                           */
        CPU_SW_EXCEPTION(;);
    }

    if (p_offset_octet == DEF_NULL) {                           /* Validate octet offset ptr.                           */
        CPU_SW_EXCEPTION(;);
    }

    if (p_offset_bit == DEF_NULL) {                             /* Validate bit   offset ptr.                           */
        CPU_SW_EXCEPTION(;);
    }

    if (nbr_bits > (sizeof(val) * DEF_OCTET_NBR_BITS)) {        /* Validate bits nbr.                                   */
        CPU_SW_EXCEPTION(;);
    }

    if (*p_offset_bit >= DEF_OCTET_NBR_BITS) {                  /* Validate bit  offset.                                */
        CPU_SW_EXCEPTION(;);
    }
#endif  /* FS_CFG_ERR_ARG_CHK_EXT_EN */

    nbr_bits_rem = nbr_bits;
    val_32_rem   = val;


                                                                /* ------------ PACK LEADING PARTIAL OCTET ------------ */
    if (*p_offset_bit > 0) {
                                                                /* Calc nbr bits to pack in initial array octet.        */
        nbr_bits_partial = DEF_OCTET_NBR_BITS - *p_offset_bit;
        if (nbr_bits_partial > nbr_bits_rem) {
            nbr_bits_partial = nbr_bits_rem;
        }


        val_08_mask =  DEF_BIT_FIELD_08(nbr_bits_partial, 0u);  /* Calc  mask to apply on 'val_32_rem'.                 */
        val_08      =  val_32_rem & val_08_mask;                /* Apply mask.                                          */
        val_08    <<= *p_offset_bit;                            /* Shift according to bit offset in leading octet.      */


                                                                /* Calc mask for kept non-val bits from leading octet.  */
        dest_08_mask_lsb = *p_offset_bit + nbr_bits_partial;
        dest_08_mask     =  DEF_BIT_FIELD_08(DEF_OCTET_NBR_BITS - dest_08_mask_lsb, dest_08_mask_lsb);
        dest_08_mask    |=  DEF_BIT_FIELD_08(*p_offset_bit, 0u);

        p_dest[*p_offset_octet] &= dest_08_mask;                /* Keep      non-val bits from leading array octet.     */
        p_dest[*p_offset_octet] |= val_08;                      /* Merge leading val bits into leading array octet.     */

                                                                /* Update bit/octet offsets.                            */
       *p_offset_bit += nbr_bits_partial;
        if (*p_offset_bit >= DEF_OCTET_NBR_BITS) {              /* If bit offset > octet nbr bits, ...                  */
            *p_offset_bit  = 0u;                                /* ... zero bit offset (offset <= DEF_OCTET_NBR_BITS)   */
           (*p_offset_octet)++;                                 /* ... and inc octet offset.                            */
        }

                                                                /* Update rem'ing val/nbr bits.                         */
        val_32_rem   >>= nbr_bits_partial;
        nbr_bits_rem  -= nbr_bits_partial;
    }


                                                                /* ---------------- PACK FULL OCTET(S) ---------------- */
    while (nbr_bits_rem >= DEF_OCTET_NBR_BITS) {
        val_08                  = (CPU_INT08U)val_32_rem & DEF_OCTET_MASK;
        p_dest[*p_offset_octet] =  val_08;                      /* Merge full-octet val bits into array octet.          */
      (*p_offset_octet)++;                                      /* Update octet offset.                                 */

                                                                /* Update rem'ing val/nbr bits.                         */
        val_32_rem   >>= DEF_OCTET_NBR_BITS;
        nbr_bits_rem  -= DEF_OCTET_NBR_BITS;
    }


                                                                /* ----------- PACK TRAILING PARTIAL OCTET ------------ */
    if (nbr_bits_rem > 0) {
        val_08_mask  =  DEF_BIT_FIELD_08(nbr_bits_rem, 0u);
        val_08       = (CPU_INT08U)val_32_rem & val_08_mask;    /* Mask trailing val bits for merge.                    */

        dest_08_mask =  DEF_BIT_FIELD_08(DEF_OCTET_NBR_BITS - nbr_bits_rem,
                                         nbr_bits_rem);

        p_dest[*p_offset_octet] &= dest_08_mask;                /* Keep non-val bits of         trailing array octet.   */
        p_dest[*p_offset_octet] |= val_08;                      /* Merge trailing val bits into trailing array octet.   */

       *p_offset_bit += nbr_bits_rem;                           /* Update/rtn final bit offset.                         */
    }
}


/*
*********************************************************************************************************
*                                          FSUtil_ValUnpack32()
*
* Description : Unpacks a specified number of least-significant bits from a specified octet and bit
*               position within an octet array, in little-endian order to a 32-bit value.
*
* Argument(s) : p_src           Pointer to source octet array.
*
*               p_offset_octet  Pointer to octet offset into 'p_src'. This function adjusts the pointee
*                               to the new position within octet array.
*
*               p_offset_bit    Pointer to bit offset into initial 'p_src[*p_offset_octet]'. This function
*                               ajusts the pointee to the new position within octet array.
*
*               nbr_bits        Number of least-significants bits to unpack from 'p_src' into value.
*
* Return(s)   : Unpacked 32-bit value, if no errors;
*               DEF_INT_32U_MAX_VAL,   otherwise.
*
* Caller(s)   : various.
*
* Note(s)     : none.
*********************************************************************************************************
*/

CPU_INT32U  FSUtil_ValUnpack32 (CPU_INT08U  *p_src,
                                CPU_SIZE_T  *p_offset_octet,
                                CPU_DATA    *p_offset_bit,
                                CPU_DATA     nbr_bits)
{
    CPU_INT32U  val_32;
    CPU_DATA    nbr_bits_partial;
    CPU_DATA    nbr_bits_rem;
    CPU_INT08U  val_08;
    CPU_INT08U  val_08_mask;


#if (FS_CFG_ERR_ARG_CHK_DBG_EN == DEF_ENABLED)                  /* ------------------ VALIDATE ARGS ------------------- */
    if (p_src == DEF_NULL) {                                    /* Validate src ptr.                                    */
        CPU_SW_EXCEPTION(DEF_INT_32U_MAX_VAL);
    }

    if (p_offset_octet == DEF_NULL) {                           /* Validate octet offset ptr.                           */
        CPU_SW_EXCEPTION(DEF_INT_32U_MAX_VAL);
    }

    if (p_offset_bit == DEF_NULL) {                             /* Validate bit   offset ptr.                           */
        CPU_SW_EXCEPTION(DEF_INT_32U_MAX_VAL);
    }

    if (nbr_bits > (sizeof(val_32) * DEF_OCTET_NBR_BITS)) {     /* Validate bits nbr.                                   */
        CPU_SW_EXCEPTION(DEF_INT_32U_MAX_VAL);
    }

    if (*p_offset_bit >= DEF_OCTET_NBR_BITS) {                  /* Validate bit  offset.                                */
        CPU_SW_EXCEPTION(DEF_INT_32U_MAX_VAL);
    }
#endif  /* FS_CFG_ERR_ARG_CHK_EXT_EN */


    nbr_bits_rem = nbr_bits;
    val_32       = 0u;
                                                                /* ----------- UNPACK LEADING PARTIAL OCTET ----------- */
    if (*p_offset_bit > 0) {
                                                                /* Calc nbr of bits to unpack from first initial octet. */
        nbr_bits_partial = DEF_OCTET_NBR_BITS - *p_offset_bit;
        if (nbr_bits_partial > nbr_bits) {
            nbr_bits_partial = nbr_bits;
        }

        val_08_mask  =  DEF_BIT_FIELD_08(nbr_bits_partial, *p_offset_bit);
        val_08       =  p_src[*p_offset_octet];
        val_08      &=  val_08_mask;                            /* Keep val leading bits.                               */
        val_08     >>= *p_offset_bit;                           /* Shift bit offset to least sig of val.                */

        val_32      |= (CPU_INT32U)val_08;                      /* Merge leading val bits from leading array octet.     */

                                                                /* Update bit/octet offsets.                            */
       *p_offset_bit += nbr_bits_partial;
        if (*p_offset_bit >= DEF_OCTET_NBR_BITS) {              /* If bit offset > octet nbr bits, ...                  */
            *p_offset_bit  = 0u;                                /* ... zero bit offset (offset <= DEF_OCTET_NBR_BITS)   */
           (*p_offset_octet)++;                                 /* ... and inc octet offset.                            */
        }


        nbr_bits_rem -= nbr_bits_partial;                       /* Update rem'ing nbr bits.                             */
    }


                                                                /* -------------- UNPACK FULL OCTET(S) ---------------- */
    while (nbr_bits_rem >= DEF_OCTET_NBR_BITS) {
        val_08   =  p_src[*p_offset_octet];
                                                                /* Merge full-octet val bits into array octet.          */
        val_32  |=  (CPU_INT08U)(val_08 << (nbr_bits - nbr_bits_rem));

      (*p_offset_octet)++;                                      /* Update octet offset.                                 */

        nbr_bits_rem -= DEF_OCTET_NBR_BITS;                     /* Update rem'ing nbr bits.                             */
    }


                                                                /* ----------- UNPACK FINAL TRAILING OCTET ------------ */
    if (nbr_bits_rem  >  0) {
        val_08_mask   =  DEF_BIT_FIELD_08(nbr_bits_rem, 0u);
        val_08        =  p_src[*p_offset_octet];
        val_08       &=  val_08_mask;                           /* Keep val trailing bits.                              */

                                                                /* Merge trailing val bits from trailing array octet.   */
        val_32       |=  (CPU_INT08U)(val_08 << (nbr_bits - nbr_bits_rem));

       *p_offset_bit +=  nbr_bits_rem;                          /* Update bit offset.                                   */
    }

    return (val_32);
}


/*
*********************************************************************************************************
*                                          FSUtil_MapBitIsSet()
*
* Description : Determines if specified bit of bitmap is set.
*
* Argument(s) : p_bitmap        Pointer to bitmap.
*               --------        Argument validated by caller.
*
*               offset_bit      Offset of bit in bitmap to test.
*
* Return(s)   : DEF_YES, if bit is set;
*               DEF_NO , otherwise.
*
* Caller(s)   : various.
*
* Note(s)     : none.
*********************************************************************************************************
*/

CPU_BOOLEAN  FSUtil_MapBitIsSet (CPU_INT08U  *p_bitmap,
                                 CPU_SIZE_T   offset_bit)
{
    CPU_SIZE_T   offset_octet;
    CPU_DATA     offset_bit_in_octet;
    CPU_INT08U   bit_mask;
    CPU_BOOLEAN  bit_set;


    offset_octet        = offset_bit >> DEF_OCTET_TO_BIT_SHIFT;
    offset_bit_in_octet = offset_bit &  DEF_OCTET_TO_BIT_MASK;

    bit_mask = DEF_BIT(offset_bit_in_octet);
    bit_set  = DEF_BIT_IS_SET(p_bitmap[offset_octet], bit_mask);

    return (bit_set);
}


/*
*********************************************************************************************************
*                                           FSUtil_MapBitSet()
*
* Description : Set specified bit in bitmap.
*
* Argument(s) : p_bitmap        Pointer to bitmap.
*               --------        Argument validated by caller.
*
*               offset_bit      Offset of bit in bitmap to test.
*
* Return(s)   : none.
*
* Caller(s)   : various.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  FSUtil_MapBitSet (CPU_INT08U  *p_bitmap,
                        CPU_SIZE_T   offset_bit)
{
    CPU_SIZE_T  offset_octet;
    CPU_DATA    offset_bit_in_octet;
    CPU_INT08U  bit_mask;


    offset_octet        = offset_bit >> DEF_OCTET_TO_BIT_SHIFT;
    offset_bit_in_octet = offset_bit &  DEF_OCTET_TO_BIT_MASK;

    bit_mask = DEF_BIT(offset_bit_in_octet);
    DEF_BIT_SET_08(p_bitmap[offset_octet], bit_mask);
}


/*
*********************************************************************************************************
*                                           FSUtil_MapBitClr()
*
* Description : Clear specified bit in bitmap.
*
* Argument(s) : p_bitmap        Pointer to bitmap.
*               --------        Argument validated by caller.
*
*               offset_bit      Offset of bit in bitmap to test.
*
* Return(s)   : none.
*
* Caller(s)   : various.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  FSUtil_MapBitClr (CPU_INT08U  *p_bitmap,
                        CPU_SIZE_T   offset_bit)
{
    CPU_SIZE_T  offset_octet;
    CPU_DATA    offset_bit_in_octet;
    CPU_INT08U  bit_mask;


    offset_octet        = offset_bit >> DEF_OCTET_TO_BIT_SHIFT;
    offset_bit_in_octet = offset_bit &  DEF_OCTET_TO_BIT_MASK;

    bit_mask = DEF_BIT(offset_bit_in_octet);
    DEF_BIT_CLR_08(p_bitmap[offset_octet], bit_mask);
}
