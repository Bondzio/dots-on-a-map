/* Version: mss_v6_3 */
/*
 * math_386.h
 *
 * Inline assembly macros for 80386+ Processor
 *
 * Copyright Mocana Corp 2003-2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

/*------------------------------------------------------------------*/

#define MULT_ADDC(a,b,index0,index1,result0,result1,result2) __asm      \
{                                                                       \
__asm   mov     ecx, a                                                  \
__asm   mov     eax, [ecx + (index0 * 4)]                               \
__asm   mov     edx, b                                                  \
__asm   mul     dword ptr [edx + (index1 * 4)]                          \
__asm   add     dword ptr [result0], eax                                \
__asm   adc     dword ptr [result1], edx                                \
__asm   adc     dword ptr [result2], 0                                  \
}


/*------------------------------------------------------------------*/

#define MULT_ADDCX(a,b,index0,index1,result0,result1,result2) __asm     \
{                                                                       \
__asm   mov     ecx, a                                                  \
__asm   mov     ebx, index0                                             \
__asm   mov     eax, [ecx + (ebx * 4)]                                  \
__asm   mov     edx, b                                                  \
__asm   mov     ebx, index1                                             \
__asm   mul     dword ptr [edx + (ebx * 4)]                             \
__asm   add     dword ptr [result0], eax                                \
__asm   adc     dword ptr [result1], edx                                \
__asm   adc     dword ptr [result2], 0                                  \
}



/*------------------------------------------------------------------*/

#define MULT_ADDC1(a,b,index0,index1,result0,result1)                   \
{                                                                       \
    unsigned long _a, _b;                                               \
    _a = a[index0];                                                     \
    _b = b[index1];                                                     \
__asm   mov     eax, dword ptr [_a]                                     \
__asm   mul     dword ptr [_b]                                          \
__asm   add     dword ptr [result0], eax                                \
__asm   adc     dword ptr [result1], edx                                \
}

/*------------------------------------------------------------------*/

#define ADD_DOUBLE(result0, result1, result2, half0, half1, half2) __asm  \
{                                                                   \
__asm   mov eax, half0                                              \
__asm   mov ebx, half1                                              \
__asm   mov ecx, half2                                              \
__asm   add eax, eax                                                \
__asm   adc ebx, ebx                                                \
__asm   adc ecx, ecx                                                \
__asm   add dword ptr[result0], eax                                 \
__asm   adc dword ptr[result1], ebx                                 \
__asm   adc dword ptr[result2], ecx                                 \
}


/*------------------------------------------------------------------*/

#define ASM_BIT_LENGTH( w, bitlen) __asm                            \
{                                                                   \
__asm   bsr eax, w                                                  \
__asm   inc eax                                                     \
__asm   mov dword ptr[bitlen], eax                                  \
}

/*------------------------------------------------------------------*/

#define DIV64BY32(hi,lo,d,q) { \
   unsigned long long t1;    \
   t1 = (((unsigned long long) hi) << 32 | lo); \
   t1 /= ((unsigned long) d); \
   q = (unsigned long) (t1 & 0xFFFFFFFF); }

/*------------------------------------------------------------------*/


#ifdef __VLONG_HEADER__

#define ASM_SHIFT_RIGHT_DEFINED

static void
shrVlong(vlong *pThis)
{
__asm
{
        xor     edx, edx
        mov     eax, pThis
        mov     ecx, [eax].numUnitsUsed
        mov     ebx, [eax].pUnits
        shl     ecx, 2
shiftAgain:
        or      ecx, ecx
        jz      shiftDone
        sub     ecx, 4
        mov     eax, [ebx + ecx]
        shrd    dword ptr [ebx + ecx], edx, 1
        mov     edx, eax
        jmp     shiftAgain
shiftDone:
}

    /* remove leading zeros */
    while ((pThis->numUnitsUsed) && (((vlong_unit)0)== pThis->pUnits[pThis->numUnitsUsed-1]))
        pThis->numUnitsUsed--;
}

#endif
