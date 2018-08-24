#include "wv_expGolobm.h"

static int32_t wv_get_bit_at_position(uint8_t  *buf, uint32_t * bytePotion, uint32_t * bitPosition)
{
	uint8_t mask = 0, val = 0;

	if ((NULL == buf) || (NULL == bytePotion) || (NULL == bitPosition))
	{
		return  - 1;
	}

	mask = 1 << (7 - *bitPosition);
	val = ((buf[*bytePotion] & mask) != 0);
	*bitPosition += 1;
	if ((*bitPosition) > 7)
	{
		*bytePotion += 1;
		*bitPosition = 0;
	}

	return val;
}

static int32_t get_uev_code_num(uint8_t *buf, uint32_t * bytePotion, uint32_t * bitPosition)
{
	if ((NULL == buf) || (NULL == bytePotion) || (NULL == bitPosition) || (*bitPosition >= 8))
	{
		return  -1;
	}

	uint8_t val = 0, prefixZeroCount = 0;
	uint32_t prefix = 0, surfix = 0;

	while (true)
	{
		val = wv_get_bit_at_position(buf, bytePotion, bitPosition);
		if (val == 0)
		{
			prefixZeroCount++;
		}
		else
		{
			break;
		}
	}
	prefix = (1 << prefixZeroCount) - 1;
	for (size_t i = 0; i < prefixZeroCount; i++)
	{
		val = wv_get_bit_at_position(buf, bytePotion, bitPosition);
		surfix += val * (1 << (prefixZeroCount - i - 1));
	}

	prefix += surfix;

	return prefix;
}

int32_t wv_getExpGolobm(WV_H264Rbsp * rbspData)
{
	int32_t codeNum = 0;

	if((NULL == rbspData) || (NULL == rbspData->buf))
	{
		printf("invalid paramters!\n");
		return -1;
	}

	if ((rbspData->bytePosition * 8 + rbspData->bitPosition) >= rbspData->dataLengthInBits)
	{
		printf("arrive end!\n");
		return -2;
	}

	codeNum = get_uev_code_num(rbspData->buf, &(rbspData->bytePosition), &(rbspData->bitPosition));

	return codeNum;
}

int32_t expGolobmTest(void)
{
	int32_t codeNum = 0;
	uint8_t strArray[6] = { 0xA6, 0x42, 0x98, 0xE2, 0x04, 0x8A };

	WV_H264Rbsp testData;
	testData.buf = strArray;
	testData.bytePosition = 0;
	testData.bitPosition = 0;
	testData.dataLengthInBits = sizeof(strArray) * 8;

	do
	{
		codeNum = wv_getExpGolobm(&testData);
		printf("ExpoColumb codeNum = %d\n", codeNum);
	} while (codeNum >= 0);

	return 0;
}

int32_t wv_getBit(WV_H264Rbsp * rbspData, int32_t nBits)
{
	int32_t i = 0;
	int32_t nBit = 0;
	int32_t nRet = 0;

	if ((NULL == rbspData) || (NULL == rbspData->buf))
	{
		printf("invalid paramters!\n");
		return -1;
	}

	for (i = 0; i < nBits; i++)
	{
		nBit = wv_get_bit_at_position(rbspData->buf, &(rbspData->bytePosition), &(rbspData->bitPosition));
		if (nBit >= 0)
		{
			nRet += (nBit << (nBits - i - 1));
		}
		else
		{
			printf("Get bit failed!\n");
			return -1;
		}
	}

	return nRet;
}

