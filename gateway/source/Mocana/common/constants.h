/*
 * constants.h
 *
 * Definitions
 *
 * Copyright Mocana Corp 2009. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

#ifndef __CONSTANTS_H__
#define __CONSTANTS_H__

/* RELOCATIONS */

/* 386 */

#define R_386_32     (1)            /* Direct 32 bit  */
#define R_386_PC32   (2)            /* PC relative 32 bit */
#define R_386_GOT32  (3)            /* 32 bit GOT entry */
#define R_386_PLT32  (4)            /* 32 bit PLT address */
#define R_386_GOTOFF (9)            /* 32 bit offset to GOT */
#define R_386_GOTPC  (10)           /* 32 bit PC relative offset to GOT */


/* powerpc */

#define R_PPC_ADDR32        (1)
#define R_PPC_ADDR16_LO     (4)
#define R_PPC_ADDR16_HA     (6)
#define R_PPC_REL24         (10)
#define R_PPC_PLTREL24      (18)
#define R_PPC_REL32         (26)
#define R_PPC_REL16_LO      (250)
#define R_PPC_REL16_HA      (252) 


/* ARM specific ELF definitions  cf ARM IHI 0044C document 
    http://infocenter.arm.com/help/topic/com.arm.doc.ihi0044c/IHI0044C_aaelf.pdf */
#define R_ARM_PC24      (1)     /* deprecated */
#define R_ARM_ABS32     (2)
#define R_ARM_GOTPC		(25)	/* 32 bit PC relative offset to GOT */
#define R_ARM_GOT32		(26)	/* 32 bit GOT entry */
#define R_ARM_PLT32		(27)	/* 32 bit PLT address */
#define R_ARM_CALL      (28)    /* new ABI -- but identical to R_ARM_PC24 */
#define R_ARM_JUMP24    (29)
#define R_ARM_PREL31    (42)

#define SHT_ARM_EXIDX   (0x70000001)

#define EF_ARM_EABIMASK (0xFF000000) 

/* length prefixed string constants */
#define ARM_EX_SEC_PREFIX_LEN   (10)
const sbyte* kArmExtabSectionPrefix = ".ARM.extab";
const sbyte* kArmExidxSectionPrefix = ".ARM.exidx";

/* other string constants */
const sbyte* kPersonalityRefName = "DW.ref.__gxx_personality_v0";
const sbyte* kPersonalityName = "__gxx_personality_v0";

#endif
