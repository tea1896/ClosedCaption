#include <cstdbool>
#include "mp2cc.h"

/*****************************************************************************
Function:     mp2_printStartCode
Description:  convert start code to string
Input:        startcode  
Return:       none
Author:       dadi.zeng 2018/07/15
Modify:       xxx / 20xx/xx/xx
*****************************************************************************/
void  mp2_printStartCode(const uint32_t startcode)
{
	switch (startcode)
	{
		case SEQ_START_CODE:
			/*printf("mp2 get SEQ_START_CODE\n");*/
			break;
		case SEQ_END_CODE:
			/*printf("mp2 get SEQ_END_CODE\n");*/
			break;
		case GOP_START_CODE:
			/*printf("mp2 get GOP_START_CODE\n");*/
			break;
		case PICTURE_START_CODE:
			/*printf("mp2 get PICTURE_START_CODE\n");*/
			break;
		case EXT_START_CODE:
			/*printf("mp2 get EXT_START_CODE\n");*/
			break;
		case USER_START_CODE:
			/*printf("mp2 get USER_START_CODE\n");*/
			break;
		default:
			if ((startcode >= SLICE_MIN_START_CODE) && (startcode <= SLICE_MAX_START_CODE))
			{
				/*printf("mp2 get slice code 0x%x\n", startcode);*/
			}
			else
			{
				/*printf("mp2 get unknow start code 0x%x\n", startcode);*/
			}
			break;
	}
}

/*****************************************************************************
Function:     mp2_find_start_code
Description:  find a start code in a mp2 frame
Input:        
			  p - start pointer of data buffer
			  end - end pointer of data buffer
Output:
			  state - found stard_code
Return:       new pointer after start_code have been found
Author:       dadi.zeng 2018/07/15
Modify:       xxx / 20xx/xx/xx
*****************************************************************************/
const uint8_t *mp2_find_start_code(const uint8_t * p,const uint8_t *end, uint32_t * state)
{
	int32_t i = 0;

	if ((NULL == p) || (NULL == end) || (NULL == state))
	{
		return NULL;
	}

	if (p >= end)
		return end;

	for (i = 0; i < 3; i++) 
	{
		uint32_t tmp = *state << 8;
		*state = tmp + *(p++);
		if (tmp == 0x100 || p == end)
			return p;
	}

	while (p < end)
	{
		if (p[-1] > 1) p += 3;
		else if (p[-2]) p += 2;
		else if (p[-3] | (p[-1] - 1)) p++;
		else {
			p++;
			break;
		}
	}

	p = WV_FFMIN(p, end) - 4;
	*state = WV_RB32(p);

	return p + 4;
}

/*****************************************************************************
Function:     mp2_findChunk
Description:  find a specified chunk in a mp2 frame
Input:
			  inputFrame - encoded frame
			  chunkCode - start_code of specified chunk 
Output:
			  offset - offset of specified chunk in encoded frame
Return:       
		      true - success
			  false - failed
Author:       dadi.zeng 2018/07/15
Modify:       xxx / 20xx/xx/xx
*****************************************************************************/
bool mp2_findChunk(wv_encodedFrame * inputFrame, int32_t chunkCode, int32_t * offset)
{
	uint32_t start_code = -1;
	uint8_t *buf_ptr = NULL;
	uint8_t *buf_end = NULL;

	if ((NULL == inputFrame) || (NULL == offset))
	{
		printf("invalid input parameters\n");
		return false;
	}

	buf_ptr = inputFrame->data;
	buf_end = buf_ptr + inputFrame->size;

	*offset = -1;
	for (;;)
	{
		start_code = -1;
		buf_ptr = mp2_find_start_code(buf_ptr, buf_end, &start_code);
		if (buf_ptr == buf_end)
		{
			break;
		}

		mp2_printStartCode(start_code);

		if (chunkCode == start_code)
		{
			if (buf_ptr > inputFrame->data)
			{
				*offset = (int32_t)(buf_ptr - inputFrame->data);
			}
			break;
		}
	}	

	return ((*offset > 0) ? true : false);
}

/*****************************************************************************
Function:     mp2_GetCCContent
Description:  get cc data from a user_data chunk of mp2 frame
Input:
			  CCChunk - user_data chunk which contain cc data
              outputCCBufferLen -  length of cc data buffer
Output:
		      outputCCBuffer - cc data buffer
			  outputCCLen - length of cc data
Return:
			  true - success
			  false - failed
Author:       dadi.zeng 2018/07/15
Modify:       xxx / 20xx/xx/xx
*****************************************************************************/
bool mp2_GetCCContent(const uint8_t * CCChunk, const uint8_t * outputCCBuffer, const uint32_t outputCCBufferLen, uint32_t * outputCCLen)
{
	int32_t ccCount = 0;
	int32_t len = 0;
	int32_t i = 0;
	int32_t validOffset = 0;

	if ((NULL == CCChunk) || (NULL == outputCCBuffer) || (NULL == outputCCLen))
	{
		printf("input parameters is wrong!\n");
		return false;
	}

	/* check mpeg2 userdata chunk */
	if ((0x47 != CCChunk[0]) || (0x41 != CCChunk[1]) || (0x39 != CCChunk[2]) || (0x34 != CCChunk[3]) ||  // atsc_identifier
		(0x03 != CCChunk[4]))   // user_data_type_code
	{
		printf("header is invalid\n");
		return false;
	}

	/* get cc data */
	ccCount = (int32_t)(CCChunk[5] & 0x1F);
	len = ccCount * 3;  // marker_bits + 16bits CC data
	if (len > outputCCBufferLen)
	{
		printf("cc output buffer is not enough\n");
		return false;
	}
	validOffset = 7;
	memcpy(outputCCBuffer, CCChunk+ validOffset, len);
	*outputCCLen = len;

	return true;
}

/*****************************************************************************
Function:     mp2_findCCOffset
Description:  find offset of cc data in a encoded mp2 frame
Input:
              inputFrame - encoded mp2 frame
Output:
              CCOffset - offset to get cc data
Return:
              true - success
              false - failed
Author:       dadi.zeng 2018/07/15
Modify:       xxx / 20xx/xx/xx
*****************************************************************************/
bool mp2_findCCOffset(wv_encodedFrame * inputFrame, int32_t * CCOffset)
{
	/* verity input parameters */
	if ((NULL == CCOffset) ||
		(NULL == inputFrame) ||
		(NULL == inputFrame->data) ||
		(inputFrame->size <= 0))
	{
		printf("input parameters is invalid!\n");
		return false;
	}

	/* find user-data chunk offset in encoded frame */
	if (mp2_findChunk(inputFrame, USER_START_CODE, CCOffset))
	{
		//printf("find user data chunk offset = %d\n", *CCOffset);
		return true;
	}
	else
	{
		printf("can't find user data chunk\n");
		return false;
	}
}

/*****************************************************************************
Function:     mp2_findCCInsertOffset
Description:  find offset to insert cc data in a encoded mp2 frame
Input:
			  inputFrame - encoded mp2 frame
Output:
			  CCOffset - offset to insert cc data
Return:
true - success
false - failed
Author:       dadi.zeng 2018/07/15
Modify:       xxx / 20xx/xx/xx
*****************************************************************************/
bool mp2_findCCInsertOffset(wv_encodedFrame * inputFrame, int32_t * CCOffset)
{
	/* verity input parameters */
	if ((NULL == CCOffset) ||
		(NULL == inputFrame) ||
		(NULL == inputFrame->data) ||
		(inputFrame->size <= 0))
	{
		printf("input parameters is invalid!\n");
		return false;
	}

	/* find offset first slice */
	if (mp2_findChunk(inputFrame, SLICE_MIN_START_CODE, CCOffset))
	{
		printf("find first slice offset = %d\n", *CCOffset);
		(*CCOffset) -= 4;
		return true;
	}
	else
	{
		printf("can't find first slcie offset\n");
		return false;
	}

	return true;
}

/*****************************************************************************
Function:     mp2_constructCCChunk
Description:  construct a user_data chunk to storage cc data
Input:
			  CCData - cc data
			  ccDataLen - length of cc data
			  CCChunkBufferLen - length of user_data chunk  
Output:
		      CCChunk - user_data chunk which contain cc data 
			  CCChunkLen - valid length of a user_data chunk
Return:
			  true - success
              false - failed
Author:       dadi.zeng 2018/07/15
Modify:       xxx / 20xx/xx/xx
*****************************************************************************/
bool mp2_constructCCChunk(const uint8_t * CCData, const uint32_t ccDataLen, uint8_t * CCChunk, const uint32_t  CCChunkBufferLen, uint32_t * CCChunkLen)
{
	uint32_t index = 0;

	/* verity input parameters */
	if ((NULL == CCData) || (NULL == CCChunk) || (NULL == CCChunkLen))
	{
		return false;
	}

	/* check output buffer is enough */
	if (ccDataLen + 12 > CCChunkBufferLen) /* 11 bytes = u-ser_data + atsc_identifier + user_data_type_code + reserved + appened_marker_bits */
	{
		return false;
	}

	/* user_data  */
	CCChunk[index++] = 0x00;
	CCChunk[index++] = 0x00;
	CCChunk[index++] = 0x01;
	CCChunk[index++] = 0xb2;

	/* atsc_identifier */
	CCChunk[index++] = 0x47;
	CCChunk[index++] = 0x41;
	CCChunk[index++] = 0x39;
	CCChunk[index++] = 0x34;

	/* user_data_type_code */
	CCChunk[index++] = 0x3;

	/*	reserved (1bit) + process_cc_data_flag (1bit) + additional_data_flag (1bit) + cc_count (5bits) */
	CCChunk[index++] = (0x0 << 7) | (0x1 << 6) | (0x0 << 5) | (ccDataLen / 3); // 3 =  marker_bits(5bits) + cc_valid(1bit) + cc_type(2bits) + cc_data1(8bits) + ccdata2(8bits)

	/* reserved */
	CCChunk[index++] = 0xff;

	/* cc data */
	memcpy(CCChunk + index, CCData, ccDataLen);
	index += ccDataLen;

	/* appened_marker_bits */
	CCChunk[index++] = 0xFF;

	/* return result */
	*CCChunkLen = index;

	return true;
}

/* get picture type */
bool  mp2_GetPictureType(wv_encodedFrame * inputFrame, WV_PICTURE_TYPE * type)
{
	int32_t offset = 0;
	WV_PICTURE_TYPE tmp = 0;

	/* verity input parameters */
	if ((NULL == inputFrame) || (NULL == type))
	{
		return false;
	}

	if (mp2_findChunk(inputFrame, PICTURE_START_CODE, &offset))
	{
		/* PICTURE_HEADER.PICTURE_CODING_TYPE */
		tmp = (inputFrame->data[offset + 1] >> 3) & 0x7;
        switch(tmp)
        {
            case 1:
                *type = WV_PICTURE_TYPE_I;
                break;
            case 2:
                *type = WV_PICTURE_TYPE_P;
                break;
            case 3:
                *type = WV_PICTURE_TYPE_B;
                break;
            default:
                *type = WV_PICTURE_TYPE_UNKNOWN;
				printf("Unkown mp2 picture type %d\n", tmp);
                break;                
        }
        
	}
	else
	{
		return false;
	}

	return true;
}

/* get picture order */
bool  mp2_GetPictureDisplayOrder(wv_encodedFrame * inputFrame, int32_t * order)
{
	int32_t offset = 0;
	int32_t tmp = 0;

	/* verity input parameters */
	if ((NULL == inputFrame) || (NULL == order))
	{
		return false;
	}

	if (mp2_findChunk(inputFrame, PICTURE_START_CODE, &offset))
	{
		/* PICTURE_HEADER.TEMPORAL_REFRENCE */
		tmp = (inputFrame->data[offset] << 2) | ((inputFrame->data[offset + 1] >> 6 & 0x3));
		*order = tmp;
	}
	else
	{
		return false;
	}

	return true;
}


/* declare closedcaption hanlder for mpeg2 video  */
const WV_CC_Handle mp2CCHandle =
{
	WV_VIDEO_TYPE_MPEG2,

	/* get cc data */
	.wvcc_findCCOffset = mp2_findCCOffset,
	.wvcc_GetCCContent = mp2_GetCCContent,

	/* insert cc data  */
	.wvcc_findCCInsertOffset = mp2_findCCInsertOffset,
	.wvcc_constructCCChunk = mp2_constructCCChunk,

	/* tools */
	.wvcc_GetPictureType = mp2_GetPictureType,
	.wvcc_GetPictureDisplayOrder = mp2_GetPictureDisplayOrder,
};


