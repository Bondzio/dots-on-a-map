/* Version: mss_v6_3 */
/*
 * asm_math.h
 *
 * Math Definitions
 *
 * Copyright Mocana Corp 2003-2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */


/*------------------------------------------------------------------*/

#if defined (__ASM_COLDFIRE_DIABDATA__)
#include "math_coldfire.h"

#elif defined(__ASM_386__)
#include "math_386.h"

#elif defined(__ASM_ARM__)
#include "math_arm.h"

#elif defined(__ASM_386_GCC__)
#include "math_386_gcc.h"

#elif defined(__ASM_PPC__)
#include "math_ppc.h"

#elif defined(__ASM_M68K_CROSSCODE__)
#include "math_m68k.h"

#elif defined(__ASM_MC68360_GCC__)
#include "math_mc68360.h"

#elif defined(__ASM_MIPS__)
#include "math_mips.h"

#elif defined(__ASM_H8S__)
#include "math_h8s.h"

#elif defined(__ASM_ULTRASPARC__)
#include "math_ultrasparc.h"

#elif defined(__ASM_CAVIUM__)
#include "math_cavm64.h"

#elif defined(__ASM_COLDFIRE_GHS__)
#include "math_coldfire_ghs.h"

#endif
