#include <cstdbool>
#include "wv_h264cc.h"
#include "wv_expGolobm.h"
#include "wv_h264parser.h"
#include "wv_h264sliceheader.h"

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
		return NULL;

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
	uint32_t nal_type = 0;

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
		if(NULL == buf_ptr)
		{
			printf("can't find nal %d\n", nalCode);
			return false;
		}
		nal_type = start_code & 0x1F;
		//h264_printNal(nalu_type);

		if (nalCode == nal_type)
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

/*****************************************************************************
Function:     h264_InitParserCtx
Description:  init context of parser
Input:
inputFrame    - encoded h264 frame
Return:
true - success
false - failed
Author:       dadi.zeng 2018/08/26
Modify:       xxx / 20xx/xx/xx
*****************************************************************************/
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

/*****************************************************************************
Function:     h264_InitParserMalloc
Description:  malloc context of h264 parser
Input:
pstCtx        - pointer to parser
Return:
true - success
false - failed
Author:       dadi.zeng 2018/08/26
Modify:       xxx / 20xx/xx/xx
*****************************************************************************/
bool h264_InitParserMalloc(WV_H264PARSER_st ** pstCtx)
{
	*pstCtx = (WV_H264PARSER_st *)malloc(sizeof(WV_H264PARSER_st));	
	(*pstCtx)->poc.PicOrderCntMsb = 0;
	(*pstCtx)->poc.MaxPicOrderCntLsb = 0;
	(*pstCtx)->poc.PrevPicOrderCntLsb = 0;
	(*pstCtx)->poc.PrevPicOrderCntMsb = 0;
}


/*****************************************************************************
Function:     h264_DelParserCtx
Description:  free context of h264 parser
Input:
inputFrame    - encoded h264 frame
Return:
true - success
false - failed
Author:       dadi.zeng 2018/08/26
Modify:       xxx / 20xx/xx/xx
*****************************************************************************/
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

/*****************************************************************************
Function:     h264_GetPicNalInfo
Description:  get naltype and slicetype from nal-unit
Input:
inputFrame     - encoded h264 frame
Output:
pic_nalType    - NAL type of nal-unit
pic_sliceType  - slice type ( I / P / B)
pic_offset     - offset of encoded slice in encoded stream
Return:
true - success
false - failed
Author:       dadi.zeng 2018/08/26
Modify:       xxx / 20xx/xx/xx
*****************************************************************************/
static bool  h264_GetPicNalInfo(wv_encodedFrame * inputFrame, int32_t * pic_nalType, int32_t * pic_sliceType, int32_t * pic_offset)
{
	uint32_t start_code = -1;
	uint8_t *buf_ptr = NULL;
	uint8_t *buf_end = NULL;
	uint32_t nal_type = 0;
	uint32_t nal_offset = 0;
	int32_t codeNum = 0;

	if ((NULL == inputFrame) || (NULL == inputFrame->data))
	{
		printf("[%s]%d:invalid parameters!\n", __func__, __LINE__);
		return false;
	}

	buf_ptr = inputFrame->data;
	buf_end = buf_ptr + inputFrame->size;

	/* find picture nal */
	nal_offset = -1;
	for (;;)
	{
		start_code = -1;
		buf_ptr = h264_find_start_code(buf_ptr, buf_end, &start_code);
		nal_type = start_code & 0x1F;

		if ((WV_H264_NAL_SLICE == nal_type) ||
			(WV_H264_NAL_IDR_SLICE == nal_type))
		{
			if (buf_ptr > inputFrame->data)
			{
				nal_offset = (int32_t)(buf_ptr - inputFrame->data);
			}
			break;
		}
	}

	/* get nal type */
	if(NULL != pic_nalType)
	{
		*pic_nalType = nal_type;
	}

	/* get slice type */
	if(NULL != pic_sliceType)
	{
		WV_H264Rbsp rbsp;
		rbsp.buf = &(inputFrame->data[nal_offset]);
		rbsp.bitPosition = rbsp.bytePosition = 0;
		rbsp.dataLengthInBits = 22; // slice header	
		codeNum = wv_getExpGolobm(&rbsp); /* firs_mb_in_slice */	
		*pic_sliceType = wv_getExpGolobm(&rbsp); /* slice_type */
	}
	
	/* offset of picture nal */
	if(NULL != pic_offset)
	{
		*pic_offset = nal_offset;
	}
		
	return ((nal_offset > 0) ? true : false);
}


/*****************************************************************************
Function:     h264_GetPictureType
Description:  get picture type from encoded h264 frame
Input:
inputFrame     - encoded h264 frame
Output:
type           - picture type ( I / P / B)
Return:
true - success
false - failed
Author:       dadi.zeng 2018/08/26
Modify:       xxx / 20xx/xx/xx
*****************************************************************************/
bool h264_GetPictureType(wv_encodedFrame * inputFrame, WV_PICTURE_TYPE * type)
{
	int32_t nalOffset = 0;
	int32_t nalType = 0;
	int32_t sliceType = 0;
	int32_t codeNum = 0;
	int32_t idrFlag = 0;

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
	if (h264_GetPicNalInfo(inputFrame, &nalType, &sliceType, NULL))
	{
		//printf("find nal slice offset = %d\n", nalOffset);
	}
	else
	{
		printf("can't find first slcie offset\n");
		return false;
	}

	/* PICTURE_HEADER.PICTURE_CODING_TYPE */
	switch (sliceType)
	{
		case WV_H264_I_SLICE:			
			if(WV_H264_NAL_IDR_SLICE == nalType)
			{
				*type = WV_PICTURE_TYPE_IDR;	
			}
			else
			{
				*type = WV_PICTURE_TYPE_I;
			}
			break;
		case WV_H264_P_SLICE:
			*type = WV_PICTURE_TYPE_P;
			break;
		case WV_H264_B_SLICE:
			*type = WV_PICTURE_TYPE_B;
			break;
		default:
			*type = WV_PICTURE_TYPE_UNKNOWN;
			printf("Unkown h264 picture type %d\n", sliceType);
			break;
	}

	return true;
}


/*****************************************************************************
Function:     h264_CalcPoc
Description:  calculae POC of h264 frame
Input:
pstPOC              - pointer of POC struct
idr_flag            - idr flag (1 ; idr frame  0: non_idr frame)
pic_order_cnt_lsb   - pic_order_cnt_lsb of slice header
Output:
                    - none
Return:
					- POC of picture
Author:       dadi.zeng 2018/08/26
Modify:       xxx / 20xx/xx/xx
*****************************************************************************/
static int32_t h264_CalcPoc(WV_H264POC_st * pstPOC, int32_t idr_flag, int32_t pic_order_cnt_lsb)
{
	int32_t nRet = -1;

	if(NULL == pstPOC)
	{
		printf("[%s]%d:invalid parameters!\n", __func__, __LINE__);
		return -1;
	}

	if(0 == idr_flag)
	{
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
	}
	else
	{
		nRet = 0;
		pstPOC->PrevPicOrderCntLsb = 0;
		pstPOC->PrevPicOrderCntMsb = 0;	
	}
	
	return nRet;
}

/*****************************************************************************
Function:     h264_GetPictureDisplayOrder
Description:  get display order of  encoded h264 frame
Input:
inputFrame     - encoded h264 frame
Output:
order          - display order
Return:
true - success
false - failed
Author:       dadi.zeng 2018/08/26
Modify:       xxx / 20xx/xx/xx
*****************************************************************************/
bool h264_GetPictureDisplayOrder(wv_encodedFrame * inputFrame, int32_t * order)
{
	int32_t nalOffset = 0;
	int32_t picNalOffset = 0;
	int32_t pic_order_cnt_lsb = 0;
	int32_t codeNum = 0;
	int32_t i  = 0;
	int32_t idrFlag = 0;
	int32_t nalType = 0;
	int32_t sliceType = 0;
	
	WV_H264PARSER_st * pstCtx = NULL;
	WV_PICTURE_TYPE picType = WV_PICTURE_TYPE_UNKNOWN;

	/* verity input parameters */
	if ((NULL == order) ||
		(NULL == inputFrame) ||
		(NULL == inputFrame->data) ||
		(inputFrame->size <= 0))
	{
		printf("[%s]%d:invalid parameters!\n", __func__, __LINE__);
		return false;
	}


	/* get picture information  */
	if(!h264_GetPicNalInfo(inputFrame, &nalType, &sliceType, &picNalOffset))
	{
		printf("can't get picture type information!\n");
		return false;
	}

	/* need to analysis sps or pps when capturing I frame */
	if(WV_H264_I_SLICE == sliceType)
	{
		/* initialize parser when first get I/IDR frame */
		if(false == inputFrame->ctxIsInit)
		{
			h264_InitParserMalloc(&pstCtx);
			inputFrame->ctx = (uint8_t *)pstCtx;
			inputFrame->ctxIsInit = true;
		}
		else
		{
			pstCtx = inputFrame->ctx;
		}

		/* get sps and pps if get I / IDR frame  */				
		if (h264_findNal(inputFrame, WV_H264_NAL_SPS, &nalOffset))
		{
			/* analyze sps */
			Parse_as_seq_param_set(&(pstCtx->sps), &(inputFrame->data[nalOffset]));
		}
		else
		{
			printf("can't find sps\n");
		}
		
		if (h264_findNal(inputFrame, WV_H264_NAL_PPS, &nalOffset))
		{
			/* analyze pps */
			Parse_as_pic_param_set(&(pstCtx->pps), &(inputFrame->data[nalOffset]));
		}
		else
		{
			printf("can't find pps\n");
		}

		/* poc */
		//pstCtx->poc.MaxPicOrderCntLsb = pow(2, pstCtx->sps.m_log2_max_poc_cnt);
		pstCtx->poc.MaxPicOrderCntLsb = 1;
		for(i=0; i<pstCtx->sps.m_log2_max_poc_cnt; i++)
		{
			pstCtx->poc.MaxPicOrderCntLsb *= 2;
		}		
	}
	else
	{
		pstCtx = inputFrame->ctx;
	}

	if(WV_H264_NAL_IDR_SLICE == nalType)
	{
		idrFlag = 1;
	}
	else
	{
		idrFlag = 0;
	}
	
	if( false == inputFrame->ctxIsInit  )
	{
		printf("ctx is not initialized , can't get order\n");
		return false;
	}

	/* calculate the POR of encoded picture */
	if(true == inputFrame->ctxIsInit )
	{
		Parse_as_pic_nal(&(pstCtx->sliceHeader), &(pstCtx->sps), &(pstCtx->pps), nalType, &(inputFrame->data[picNalOffset]));
		*order = h264_CalcPoc(&(pstCtx->poc), idrFlag, pstCtx->sliceHeader.m_poc);
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



