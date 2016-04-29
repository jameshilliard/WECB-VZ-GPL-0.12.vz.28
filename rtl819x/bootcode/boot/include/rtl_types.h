/*
* Copyright c                  Realtek Semiconductor Corporation, 2002  
* All rights reserved.                                                    
* 
* Abstract : realtek type definition
 *
 * $Author: jasonwang $
 *
 *
*/



#ifndef _RTL_TYPES_H
#define _RTL_TYPES_H

/*
 * Internal names for basic integral types.  Omit the typedef if
 * not possible for a machine/compiler combination.
 */

typedef unsigned long long	uint64;
typedef long long		int64;
typedef unsigned int	uint32;
typedef int			int32;
typedef unsigned short	uint16;
typedef short			int16;
typedef unsigned char	uint8;
typedef char			int8;

#define UINT32_MAX	UINT_MAX
#define INT32_MIN	INT_MIN
#define INT32_MAX	INT_MAX
#define UINT16_MAX	USHRT_MAX
#define INT16_MIN	SHRT_MIN
#define INT16_MAX	SHRT_MAX
#define UINT8_MAX	UCHAR_MAX
#define INT8_MIN		SCHAR_MIN
#define INT8_MAX	SCHAR_MAX

typedef uint32		memaddr;	
typedef uint32          ipaddr_t;

typedef struct {
    uint16      mac47_32;
    uint16      mac31_16;
    uint16      mac15_0;
    uint16		align;
} macaddr_t;


typedef int8*			calladdr_t;

typedef struct ether_addr_s {
	uint8 octet[6];
} ether_addr_t;

#include "rtl_depend.h"

#ifndef NULL
#define NULL 0
#endif
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#ifndef SUCCESS
#define SUCCESS 	0
#endif
#ifndef FAILED
#define FAILED -1
#endif

#define CLEARBITS(a,b)	((a) &= ~(b))
#define SETBITS(a,b)		((a) |= (b))
#define ISSET(a,b)		(((a) & (b))!=0)
#define ISCLEARED(a,b)	(((a) & (b))==0)

#ifndef max
#define max(a,b)  (((a) > (b)) ? (a) : (b))
#endif			   /* max */

#ifndef min
#define min(a,b)  (((a) < (b)) ? (a) : (b))
#endif			   /* min */


extern int dprintf(char *fmt, ...);
//#define ASSERT_CSP(x) if (!(x)) {dprintf("\nAssertion fail at file %s, function %s, line number %d: (%s).\n", __FILE__, __FUNCTION__, __LINE__, #x); while(1);}
//#define ASSERT_ISR(x) if (!(x)) {printfByPolling("\nAssertion fail at file %s, function %s, line number %d: (%s).\n", __FILE__, __FUNCTION__, __LINE__, #x); while(1);}

//wei add, because we only use polling mode uart-print
#define ASSERT_CSP(x) if (!(x)) {dprintf("\nAssertion fail at file"); while(1);}
#define ASSERT_ISR(x) if (!(x)) {dprintf("\nAssertion fail at file");   while(1);}


#endif
