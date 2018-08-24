#ifndef WV_EXPGOLOBM_H
#define WV_EXPGOLOBM_H

#include <stdint.h>
#include <stdlib.h>
#include <cstdbool>

typedef struct WV_H264RBSP
{
	uint8_t * buf;
	uint32_t bytePosition;
	uint32_t bitPosition;
	uint32_t dataLengthInBits;
}WV_H264Rbsp;


int32_t wv_getExpGolobm(WV_H264Rbsp * rbspData);
int32_t wv_getBit(WV_H264Rbsp * rbspData, int32_t nBit);
int32_t expGolobmTest(void);

#endif