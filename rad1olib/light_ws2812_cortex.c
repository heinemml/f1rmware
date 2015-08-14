/*
 * light weight WS2812 lib - ARM Cortex M0/M0+ version
 *
 * Created: 07.07.2013
 *  Author: Tim (cpldcpu@gmail.com)
 */

#include "light_ws2812_cortex.h"
/*
* The total length of each bit is 1.25µs (25 cycles @ 20Mhz)
* At 0µs the dataline is pulled high.
* To send a zero the dataline is pulled low after 0.375µs
* To send a one the dataline is pulled low after 0.625µs
*/

#define ws2812_ctot	(((ws2812_cpuclk/1000)*1250)/1000000)
#define ws2812_t1	(((ws2812_cpuclk/1000)*375 )/1000000)		// floor
#define ws2812_t2	(((ws2812_cpuclk/1000)*625+500000)/1000000) // ceil

#define w1 (ws2812_t1-2)
#define w2 (ws2812_t2-ws2812_t1-2)
#define w3 (ws2812_ctot-ws2812_t2-5)

#define ws2812_DEL1 "	nop		\n\t"
#define ws2812_DEL2 "   nop \n\t nop \n\t"
#define ws2812_DEL4 ws2812_DEL2 ws2812_DEL2
#define ws2812_DEL8 ws2812_DEL4 ws2812_DEL4
#define ws2812_DEL16 ws2812_DEL8 ws2812_DEL8


void ws2812_sendarray(uint8_t *data,int datlen)
{
	uint32_t maskhi = ws2812_mask_set;
	uint32_t masklo = ws2812_mask_clr;
	volatile uint32_t *set = ws2812_port_set;
	volatile uint32_t *clr = ws2812_port_clr;
	uint32_t i = 0;
	uint32_t curbyte;
	
/* Workaround to match CPU speed to the ws2812 "baudrate" */	
	uint32_t old_cpu_speed = _cpu_speed;
	cpu_clock_set(51);
	
	while (datlen--) {
		curbyte=*data++/1;

	__asm__ volatile(

            "CPSID I \n\t"
			"		lsl %[dat],#24				\n\t"
			"		movs %[ctr],#8				\n\t"
			"ilop%=:							\n\t"
			"		movs   %[dat], %[dat], lsl #1 \n\t"
			"		str %[maskhi], [%[set]]		\n\t"
#if (w1&1)
			ws2812_DEL1
#endif
#if (w1&2)
			ws2812_DEL2
#endif
#if (w1&4)
			ws2812_DEL4
#endif
#if (w1&8)
			ws2812_DEL8
#endif
#if (w1&16)
			ws2812_DEL16
#endif
			"		bcs one%=					\n\t"
			"		str %[masklo], [%[clr]]		\n\t"
			"one%=:								\n\t"
#if (w2&1)
			ws2812_DEL1
#endif
#if (w2&2)
			ws2812_DEL2
#endif
#if (w2&4)
			ws2812_DEL4
#endif
#if (w2&8)
			ws2812_DEL8
#endif
#if (w2&16)
			ws2812_DEL16
#endif
			"		str %[masklo], [%[clr]]		\n\t"
			"		subs %[ctr], #1				\n\t"
			ws2812_DEL1
#if (w3&1)
			ws2812_DEL1
#endif
#if (w3&2)
			ws2812_DEL2
#endif
#if (w3&4)
			ws2812_DEL4
#endif
#if (w3&8)
			ws2812_DEL8
#endif
#if (w3&16)
			ws2812_DEL16
#endif
			"		bne 	ilop%=					\n\t"
			"end%=:								\n\t"
            "CPSIE I \n\t"
			:	[ctr] "+r" (i)
			:	[dat] "r" (curbyte), [set] "r" (set), [clr] "r" (clr), [masklo] "r" (masklo), [maskhi] "r" (maskhi)
			);
	}
	/* Reset CPU speed to previous */
	cpu_clock_set(old_cpu_speed);
}

