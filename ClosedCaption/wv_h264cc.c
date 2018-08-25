#include <cstdbool>
#include "wv_h264cc.h"
#include "wv_expGolobm.h"
#include "wv_h264parser.h"

/*****************************************************************************
Function:     h264_printNal
Description:  convert start code to string
Input:        startcode
Return:       none
Author:       dadi.zeng 2018/08/10
Modify:       xxx / 20xx/xx/xx
*****************************************************************************/
void  h264_printNal(const uint32_t startcode)
{
	switch (startcode)
	{
	case WV_H264_NAL_SLICE:
		printf("h264 get WV_H264_NAL_SLICE\n");
		break;
	case WV_H264_NAL_IDR_SLICE:
		printf("h264 get WV_H264_NAL_IDR_SLICE\n");
		break;
	case WV_H264_NAL_SPS:
		printf("h264 get WV_H264_NAL_SPS\n");
		break;
	case WV_H264_NAL_PPS:
		printf("h264 get WV_H264_NAL_PPS\n");
		break;
	case WV_H264_NAL_SPS_EXT:
		printf("h264 get WV_H264_NAL_SPS_EXT\n");
		break;
	case WV_H264_NAL_SEI:
		printf("h264 get WV_H264_NAL_SEI\n");
		break;
	case WV_H264_NAL_AUD:
		printf("h264 get WV_H264_NAL_AUD\n");
		break;
	default:
			printf("h264 get nal %d\n", startcode);
		break;
	}
}

/*****************************************************************************
Function:     h264_find_start_code
Description:  find a start code in a h264 frame
Input:
p - start pointer of data buffer
end - end pointer of data buffer
Output:
state - found stard_code
Return:       new pointer after start_code have been found
Author:       dadi.zeng 2018/08/10
Modify:       xxx / 20xx/xx/xx
*****************************************************************************/
static const uint8_t * h264_find_start_code(const uint8_t * p, const uint8_t *end, uint32_t * state)
{
	int i;

	if (p > end)
	{
		printf("invalid parameters!\n");
		return NULL;
	}

	if (p >= end)
		return end;

	for (i = 0; i < 3; i++) {
		uint32_t tmp = *state << 8;
		*state = tmp + *(p++);
		if (tmp == 0x100 || p == end)
			return p;
	}

	while (p < end) {
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
Function:     h264_findNal
Description:  find a specified nal in a h264 frame
Input:
inputFrame - encoded frame
nalCode - start_code of specified nal
Output:
offset - offset of specified nal in encoded frame
Return:
true - success
false - failed
Author:       dadi.zeng 2018/08/10
Modify:       xxx / 20xx/xx/xx
*****************************************************************************/
static bool h264_findNal(wv_encodedFrame * inputFrame, int32_t nalCode, int32_t * offset)
{
	uint32_t start_code = -1;
	uint8_t *buf_ptr = NULL;
	uint8_t *buf_end = NULL;
	uint32_t nalu_type = 0;

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
		buf_ptr = h264_find_start_code(buf_ptr, buf_end, &start_code);
		nalu_type = start_code & 0x1F;
		//h264_printNal(nalu_type);

		if (nalCode == nalu_type)
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
Function:     h264_GetCCContent
Description:  get cc data from a sei nal of h264 frame
Input:
CCChunk - sei nal which contain cc data
outputCCBufferLen -  length of cc data buffer
Output:
outputCCBuffer - cc data buffer
outputCCLen - length of cc data
Return:
true - success
false - failed
Author:       dadi.zeng 2018/08/10
Modify:       xxx / 20xx/xx/xx
*****************************************************************************/
static bool h264_GetCCContent(const uint8_t * CCNAL, const uint8_t * outputCCBuffer, const uint32_t outputCCBufferLen, uint32_t * outputCCLen)
{
	int32_t ccCount = 0;
	int32_t len = 0;
	int32_t i = 0;
	int32_t validOffset = 0;

	if ((NULL == CCNAL) || (NULL == outputCCBuffer) || (NULL == outputCCLen))
	{
		printf("input parameters is wrong!\n");
		return false;
	}

	/* check mpeg2 userdata chunk */
	if ((0x47 != CCNAL[0]) || (0x41 != CCNAL[1]) || (0x39 != CCNAL[2]) || (0x34 != CCNAL[3]) ||  // atsc_identifier
		(0x03 != CCNAL[4]))   // user_data_type_code
	{
		//printf("header is invalid\n");
		return false;
	}

	/* get cc data */
	ccCount = (int32_t)(CCNAL[5] & 0x1F);
	len = ccCount * 3;  // marker_bits + 16bits CC data
	if (len > outputCCBufferLen)
	{
		printf("cc output buffer is not enough\n");
		return false;
	}
	validOffset = 7;
	memcpy(outputCCBuffer, CCNAL + validOffset, len);
	*outputCCLen = len;

	return true;
}

/*****************************************************************************
Function:     h264_findCCOffset
Description:  find offset of cc data in a encoded h264 frame
Input:
inputFrame - encoded h264 frame
Output:
CCOffset - offset to get cc data
Return:
true - success
false - failed
Author:       dadi.zeng 2018/08/10
Modify:       xxx / 20xx/xx/xx
*****************************************************************************/
static bool h264_findCCOffset(wv_encodedFrame * inputFrame, int32_t * CCOffset)
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

	/* find SEI nal offset in encoded frame */
	if (h264_findNal(inputFrame, WV_H264_NAL_SEI, CCOffset))
	{
		//printf("find user data chunk offset = %d\n", *CCOffset);
		return true;
	}
	else
	{
		//printf("can't find user data chunk\n");
		return false;
	}
}

/*****************************************************************************
Function:     h264_findCCInsertOffset
Description:  find offset to insert cc data in a encoded h264 frame
Input:
inputFrame - encoded h264 frame
Output:
CCOffset - offset to insert cc data
Return:
true - success
false - failed
Author:       dadi.zeng 2018/08/10
Modify:       xxx / 20xx/xx/xx
*****************************************************************************/
static bool h264_findCCInsertOffset(wv_encodedFrame * inputFrame, int32_t * CCOffset)
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
	if (h264_findNal(inputFrame, WV_H264_NAL_SLICE, CCOffset))
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
Function:     h264_constructCCCNal
Description:  construct a SEI nal to storage cc data
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
Author:       dadi.zeng 2018/08/10
Modify:       xxx / 20xx/xx/xx
*****************************************************************************/
static bool h264_constructCCCNal(const uint8_t * CCData, const uint32_t ccDataLen, uint8_t * CCChunk, const uint32_t  CCChunkBufferLen, uint32_t * CCChunkLen)
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

/* init parser */
bool h264_InitParserCtx(wv_encodedFrame * inputFrame)
{
	WV_H264PARSER_st * pstCtx = NULL;

	/* verity input parameters */
	if (NULL == inputFrame)
	{
		printf("input parameters is invalid!\n");
		return false;
	}

	inputFrame->ctx = NULL;
	
	return true;
}

bool h264_InitParserMalloc(WV_H264PARSER_st ** pstCtx)
{
	*pstCtx = (WV_H264PARSER_st *)malloc(sizeof(WV_H264PARSER_st));	
	(*pstCtx)->poc.PicOrderCntMsb = 0;
	(*pstCtx)->poc.MaxPicOrderCntLsb = 0;
	(*pstCtx)->poc.PrevPicOrderCntLsb = 0;
	(*pstCtx)->poc.PrevPicOrderCntMsb = 0;
}


/* del parser */
bool h264_DelParserCtx(wv_encodedFrame * inputFrame)
{
	/* verity input parameters */
	if (NULL == inputFrame)
	{
		printf("input parameters is invalid!\n");
		return false;
	}

	if(NULL != inputFrame)
	{
		free(inputFrame->ctx);
	}

	return true;
}

/* get picture type */
bool    h264_GetPictureType(wv_encodedFrame * inputFrame, WV_PICTURE_TYPE * type)
{
	int32_t nalOffset = 0;
	int32_t sliceType = 0;
	int32_t codeNum = 0;

	/* verity input parameters */
	if ((NULL == type) ||
		(NULL == inputFrame) ||
		(NULL == inputFrame->data) ||
		(inputFrame->size <= 0))
	{
		printf("input parameters is invalid!\n");
		return false;
	}

	/* find offset first slice */
	if (h264_findNal(inputFrame, WV_H264_NAL_SLICE, &nalOffset))
	{
		//printf("find nal slice offset = %d\n", nalOffset);
	}
	else
	{
		printf("can't find first slcie offset\n");
		return false;
	}

	/* PICTURE_HEADER.PICTURE_CODING_TYPE */
	WV_H264Rbsp rbsp;
	rbsp.buf = &(inputFrame->data[nalOffset]);
	rbsp.bitPosition = rbsp.bytePosition = 0;
	rbsp.dataLengthInBits = 22; // slice header

	/* firs_mb_in_slice */
	codeNum = wv_getExpGolobm(&rbsp);
	/* slice_type */
	sliceType = wv_getExpGolobm(&rbsp);
	switch (sliceType)
	{
		case 7:
			*type = WV_PICTURE_TYPE_I;
			break;
		case 5:
			*type = WV_PICTURE_TYPE_P;
			break;
		case 6:
			*type = WV_PICTURE_TYPE_B;
			break;
		default:
			*type = WV_PICTURE_TYPE_UNKNOWN;
			printf("Unkown h264 picture type %d\n", sliceType);
			break;
	}

	return true;
}

int32_t h264_CalcPoc(WV_H264POC_st * pstPOC, int32_t pic_order_cnt_lsb)
{
	int32_t nRet = -1;

	if(NULL == pstPOC)
	{
		printf("invalid parameters!\n");
		return -1;
	}

	if ((pic_order_cnt_lsb  <  pstPOC->PrevPicOrderCntLsb) &&
	    ((pstPOC->PrevPicOrderCntLsb - pic_order_cnt_lsb) >= (pstPOC->MaxPicOrderCntLsb / 2)))	
	{
		pstPOC->PicOrderCntMsb = pstPOC->PrevPicOrderCntMsb + pstPOC->MaxPicOrderCntLsb;
	}
	else if ((pic_order_cnt_lsb  >  pstPOC->PrevPicOrderCntLsb) &&
	         ((pic_order_cnt_lsb - pstPOC->PrevPicOrderCntLsb)  >  (pstPOC->MaxPicOrderCntLsb / 2)))	
	{
		pstPOC->PicOrderCntMsb = pstPOC->PrevPicOrderCntMsb - pstPOC->MaxPicOrderCntLsb;
	}
	else
	{
		pstPOC->PicOrderCntMsb = pstPOC->PrevPicOrderCntMsb;
	}

	nRet = pstPOC->PicOrderCntMsb + pic_order_cnt_lsb;

	pstPOC->PrevPicOrderCntLsb = pic_order_cnt_lsb;
	pstPOC->PrevPicOrderCntMsb = pstPOC->PicOrderCntMsb;
	
	return nRet;
}

/* get picture order */
bool h264_GetPictureDisplayOrder(wv_encodedFrame * inputFrame, int32_t * order)
{
	int32_t nalOffset = 0;
	int32_t pic_order_cnt_lsb = 0;
	int32_t codeNum = 0;
	int32_t i  = 0;
	
	static int32_t PicOrderCntMsb = 0;
	static int32_t MaxPicOrderCntLsb = 32;
	static int32_t PrevPicOrderCntLsb = 0;
	static int32_t PrevPicOrderCntMsb = 0;

	WV_H264PARSER_st * pstCtx = NULL;
	WV_PICTURE_TYPE picType = WV_PICTURE_TYPE_UNKNOWN;

	/* verity input parameters */
	if ((NULL == order) ||
		(NULL == inputFrame) ||
		(NULL == inputFrame->data) ||
		(inputFrame->size <= 0))
	{
		printf("input parameters is invalid!\n");
		return false;
	}


	/* init context of parser  */
	if(h264_GetPictureType(inputFrame, &picType))
	{
		if(WV_PICTURE_TYPE_I == picType)
		{
			/* init parser */
			if(false == inputFrame->ctxIsInit)
			{
				h264_InitParserMalloc(&pstCtx);
				inputFrame->ctx = (uint8_t *)pstCtx;
				inputFrame->ctxIsInit = true;
			}
		}
	}
	if( false == inputFrame->ctxIsInit  )
	{
		printf("ctx is not initialized , can't get order\n");
		return false;
	}
	else
	{
		pstCtx = inputFrame->ctx;
	}

	/* get sps and pps if get I frame  */
	if(h264_GetPictureType(inputFrame, &picType))
	{
		if(WV_PICTURE_TYPE_I == picType)
		{
			/* analysis sps */
			if (h264_findNal(inputFrame, WV_H264_NAL_SPS, &nalOffset))
			{
				Parse_as_seq_param_set(&(pstCtx->sps), &(inputFrame->data[nalOffset]));
			}

			/* analysis pps */
			if (h264_findNal(inputFrame, WV_H264_NAL_PPS, &nalOffset))
			{
				Parse_as_pic_param_set(&(pstCtx->pps), &(inputFrame->data[nalOffset]));
			}

			/* poc */
			pstCtx->poc.MaxPicOrderCntLsb = 1;
			for(i=0; i<pstCtx->sps.m_log2_max_poc_cnt; i++)
			{
				pstCtx->poc.MaxPicOrderCntLsb *= 2;
			}
			//pstCtx->poc.MaxPicOrderCntLsb = pow(2, pstCtx->sps.m_log2_max_poc_cnt);
		}
	}


	if(true ==  inputFrame->ctxIsInit )
	{
		/* find offset first slice */
		if (h264_findNal(inputFrame, WV_H264_NAL_SLICE, &nalOffset))
		{
			//printf("find nal slice offset = %d\n", nalOffset);
		}
		else
		{
			printf("can't find first slcie offset\n");
			return false;
		}

		/* PICTURE_HEADER.PICTURE_CODING_TYPE */
		WV_H264Rbsp rbsp;
		rbsp.buf = &(inputFrame->data[nalOffset]);
		rbsp.bitPosition = rbsp.bytePosition = 0;
		rbsp.dataLengthInBits = 100; // slice header

		/* firs_mb_in_slice */
		codeNum = wv_getExpGolobm(&rbsp);
		/* slice_type */
		codeNum = wv_getExpGolobm(&rbsp);
		/* pic_parameter_set_id */
		codeNum = wv_getExpGolobm(&rbsp);
		/* frame_num */
		codeNum = wv_getBit(&rbsp, 4);
		/* field_pic_flag */
		codeNum = wv_getBit(&rbsp, 1);
		/* pic_order_cnt_lst */
		pic_order_cnt_lsb = wv_getBit(&rbsp, 5);

		*order = h264_CalcPoc(&(pstCtx->poc), pic_order_cnt_lsb);
	}
	else
	{
		printf("context of parser is not initalized, can't get order\n");
		return false;
	}

	return true;
}



/* declare closedcaption hanlder for mpeg2 video  */
const WV_CC_Handle h264CCHandle =
{
	WV_VIDEO_TYPE_H264,

	/* get cc data */
	.wvcc_findCCOffset = h264_findCCOffset,
	.wvcc_GetCCContent = h264_GetCCContent,

	/* insert cc data  */
	.wvcc_findCCInsertOffset = h264_findCCInsertOffset,
	.wvcc_constructCCChunk = h264_constructCCCNal,

	/* tools */
	.wvcc_GetPictureType = h264_GetPictureType,
	.wvcc_GetPictureDisplayOrder = h264_GetPictureDisplayOrder,

    /* ctx */
    .wvcc_InitFrameCtx = h264_InitParserCtx,
    .wvcc_DelFrameCtx = h264_DelParserCtx,
};



