/* Version: mss_v6_3 */
/*
 * datetime.h
 *
 * Date Time routines
 *
 * Copyright Mocana Corp 2006-2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */


/*------------------------------------------------------------------*/

#ifndef __DATETIME_HEADER__
#define __DATETIME_HEADER__

#ifdef __cplusplus
extern "C" {
#endif

/* caculate the number of seconds diff between two TimeDates */
MOC_EXTERN MSTATUS
DATETIME_diffTime( TimeDate* pDT1, TimeDate* pDT2, sbyte4* pSecDiff);

/* caculate a new TimeDate by offseting the first TimeDate by a seconds diff */
MOC_EXTERN MSTATUS
DATETIME_getNewTime( TimeDate* pDT1, sbyte4 secDiff, TimeDate *pDT2);

/* Return a NULL terminated string representing the Validity in a X.509 certificate , as defined in RFC3280.
 * The return string format will be either UTCTIME or GENERALIZEDTIME depending on the time.
 */
MOC_EXTERN MSTATUS
DATETIME_convertToValidityString(TimeDate *pTime, sbyte *pTimeString);

/* Convert a NULL terminated string representing the Validity time, as defined in RFC3280,
 * to a TimeDate */
MOC_EXTERN MSTATUS
DATETIME_convertFromValidityString(sbyte *pTimeString, TimeDate *pTime);

MOC_EXTERN MSTATUS
DATETIME_convertFromValidityString2(ubyte *pTimeString, ubyte4 timeStrLen, TimeDate *pTime);


#ifdef __cplusplus
}
#endif

#endif /* __MTCP_HEADER__ */
