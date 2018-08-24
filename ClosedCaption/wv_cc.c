#include "wv_cc.h"
#include <stdint.h>

// list all cc handler
extern WV_CC_Handle mp2CCHandle;
extern WV_CC_Handle h264CCHandle;

static WV_CC_Handle * s_stCCHandleMap[] =
{
	&mp2CCHandle,
	&h264CCHandle,
	NULL,
};

/*****************************************************************************
Function:     wvcc_FindHandler
Description:  find corresponding handler to get/insert cc data  according to encode type 
Input:        enEncodeType     -   encode type
Output:       cc handler
Return:       none
Author:       dadi.zeng
*****************************************************************************/
static inline WV_CC_Handle * wvcc_FindHandler(WV_ENCODE_TYPE enEncodeType)
{
	WV_CC_Handle ** pstHandler = s_stCCHandleMap;

	while (NULL != pstHandler)
	{
		if (NULL == *pstHandler)
		{
			break;
		}

		if ((*pstHandler)->encodeType == enEncodeType)
		{
			return *pstHandler;
		}
		pstHandler++;
	}

	return NULL;
}


/*****************************************************************************
Function:     wvcc_findCCContent
Description:  get cc data from encoded picture data
Input:        inputFrame     -   encoded picture
              outputCCBufferLen - length of cc buffer
Output:			 
			  outputCCBuffer -   buffer to load cc data
			  outputCCLen - number of cc data
Return:       true - success
		      false - failed
Author:       dadi.zeng 2018/07/15
Modify:       xxx / 20xx/xx/xx
*****************************************************************************/
bool wvcc_findCCContent(wv_encodedFrame * inputFrame, uint8_t * outputCCBuffer, const uint32_t outputCCBufferLen, uint32_t * outputCCLen)
{
	int32_t offset = 0;
	WV_CC_Handle * pstCCHandler = NULL;

	/* verity input parameters */
	if ((NULL == outputCCBuffer) ||
		(NULL == inputFrame) ||
		(NULL == inputFrame->data) ||
		(inputFrame->size <= 0))
	{
		printf("input parameters is invalid!\n");
		return false;
	}

	/* find handler */
	pstCCHandler = wvcc_FindHandler(inputFrame->encodeType);
	if (NULL == pstCCHandler)
	{
		printf("can't find corresponding cc function\n");
		return false;
	}

	/* find cc offset */
	if (!(pstCCHandler->wvcc_findCCOffset(inputFrame, &offset)))
	{
		//printf("can't get cc offset\n");
		return false;
	}

	/* get cc content */
	if (!(pstCCHandler->wvcc_GetCCContent(inputFrame->data + offset, outputCCBuffer, outputCCBufferLen, outputCCLen)))
	{
		//printf("can't get cc content\n");
		return false;
	}
	return true;
}

/*****************************************************************************
Function:     wvcc_insertCCContent
Description:  insert cc data to encoded picture data
Input:        pstFrame   -   encoded picture
                             !! Alarm: pstFrame->data is changed after this call
			  ccDataBuffer - cc data
			  ccDataLen - length of cc data 
Return:       true - success
			  false - failed
Author:       dadi.zeng 2018/07/15
Modify:       xxx / 20xx/xx/xx
*****************************************************************************/
bool wvcc_insertCCContent(wv_encodedFrame * pstFrame, uint8_t * ccDataBuffer, uint32_t ccDataLen)
{
	WV_CC_Handle * pstCCHandler = NULL;
	uint32_t offset = 0;
	uint8_t CCChunk[1024] = { 0 };
	uint32_t CCOffset = 0;
	uint32_t CCChunkLen = 0;
	uint8_t * newFrameBuffer = NULL;

	/* verity input parameters */
	if ((NULL == ccDataBuffer) ||
		(NULL == pstFrame) ||
		(NULL == pstFrame->data))
	{
		printf("input parameters is invalid!\n");
		return false;
	}

	/* find handler */
	pstCCHandler = wvcc_FindHandler(pstFrame->encodeType);
	if (NULL == pstCCHandler)
	{
		printf("can't find corresponded handler!\n");
		return false;
	}

	/* find offset to insert cc data */
	if (!(pstCCHandler->wvcc_findCCInsertOffset(pstFrame, &CCOffset)))
	{
		printf("can't find point to insert cc data\n");
		return false;
	}

	/* constrcut cc chunk/nal */
	if (!(pstCCHandler->wvcc_constructCCChunk(ccDataBuffer, ccDataLen, CCChunk, sizeof(CCChunk), &CCChunkLen)))
	{
		printf("cant't contruct cc data area\n");
		return false;
	}

	/* insert cc content */
	newFrameBuffer = malloc(pstFrame->size + CCChunkLen);
	if (NULL == newFrameBuffer)
	{
		printf("malloc failed!\n");
		return false;
	}
	memcpy(newFrameBuffer, pstFrame->data, CCOffset);
	memcpy(newFrameBuffer + CCOffset, CCChunk, CCChunkLen);
	memcpy(newFrameBuffer + CCOffset + CCChunkLen, pstFrame->data + CCOffset, pstFrame->size - CCOffset);

	pstFrame->data = newFrameBuffer;
	pstFrame->size += CCChunkLen;

	return true;
}


bool wvcc_getPictureType(wv_encodedFrame * inputFrame, WV_PICTURE_TYPE * pictureType)
{
	WV_CC_Handle * pstCCHandler = NULL;

	/* verity input parameters */
	if ((NULL == inputFrame) ||
		(NULL == pictureType))
	{
		printf("input parameters is invalid!\n");
		return false;
	}

	/* find handler */
	pstCCHandler = wvcc_FindHandler(inputFrame->encodeType);
	if (NULL == pstCCHandler)
	{
		printf("can't find corresponded handler!\n");
		return false;
	}

	/* get type */
	if (!(pstCCHandler->wvcc_GetPictureType(inputFrame, pictureType)))
	{
		printf("can't find  picture type!\n");
		return false;
	}

	switch (*pictureType)
	{
		case WV_PICTURE_TYPE_I:
			printf("### I picture!\n");
			break;
		case WV_PICTURE_TYPE_B:
			printf("### B picture!\n");
			break;
		case WV_PICTURE_TYPE_P:
			printf("### P picture!\n");
			break;
		default:
			printf("Unknown picture!\n");
			break;
	}

	return true;
}

bool wvcc_getPictureOrder(wv_encodedFrame * inputFrame, uint32_t * pictureorder)
{
	WV_CC_Handle * pstCCHandler = NULL;

	/* verity input parameters */
	if ((NULL == inputFrame) ||
		(NULL == pictureorder))
	{
		printf("input parameters is invalid!\n");
		return false;
	}

	/* find handler */
	pstCCHandler = wvcc_FindHandler(inputFrame->encodeType);
	if(NULL == pstCCHandler)
	{
		printf("can't find corresponded handler!\n");
		return false;
	}

	/* get type */
	if(!(pstCCHandler->wvcc_GetPictureDisplayOrder(inputFrame, pictureorder)))
	{
		printf("can't find  picture order!\n");
		return false;
	}
	printf("order %d\n\n", *pictureorder);

	return true;
}

bool wvcc_InitParserCtx(wv_encodedFrame * inputFrame)
{
	WV_CC_Handle * pstCCHandler = NULL;

	/* verity input parameters */
	if (NULL == inputFrame)
	{
		printf("input parameters is invalid!\n");
		return false;
	}

	inputFrame->ctxIsInit = false;
	inputFrame->ctx = NULL;

	return true;
}

bool wvcc_DelParserCtx(wv_encodedFrame * inputFrame)
{
	WV_CC_Handle * pstCCHandler = NULL;

	/* verity input parameters */
	if (NULL == inputFrame)
	{
		printf("input parameters is invalid!\n");
		return false;
	}

	/* find handler */
	pstCCHandler = wvcc_FindHandler(inputFrame->encodeType);
	if(NULL == pstCCHandler)
	{
		printf("can't find corresponded handler!\n");
		return false;
	}

	/* init context of parser */
	inputFrame->ctxIsInit = false;
	if(!(pstCCHandler->wvcc_DelFrameCtx(inputFrame)))
	{
		printf("can't init	context of parser!\n");
		return false;
	}

	return true;
}



