#ifndef WV_CC_H
#define WV_CC_H
#include <stdint.h>
#include <stdlib.h>
#include <cstdbool>

#include "wv_ffmpeg4cc.h"

typedef struct
{
	WV_ENCODE_TYPE encodeType;
	uint8_t * data;
	int32_t size;
	uint8_t * ctx;
}wv_encodedFrame;

typedef struct CCFuncMap
{
	WV_ENCODE_TYPE encodeType;

	/* find offset of cc data in a encoded picture */
	bool(*wvcc_findCCOffset)(wv_encodedFrame * inputFrame, int32_t * CCOffset);
	/* get content of cc data from a encoded picture */
	bool(*wvcc_GetCCContent)(const uint8_t * CCData, const uint8_t * outputCCBuffer, const uint32_t outputCCBufferLen, uint32_t * outputCCLen);

	/* find offset to insert cc data in a encoded picture */
	bool (*wvcc_findCCInsertOffset)(wv_encodedFrame * inputFrame, int32_t * CCOffset);
	/* construct cc chunk/nal according syntax of encode type */
	bool(*wvcc_constructCCChunk)(const uint8_t * CCData, const uint32_t ccDataLen, uint8_t * CCChunk, const uint32_t  CCChunkBufferLen, uint32_t * CCChunkLen);

	/* init parser */
	bool(*wvcc_InitFrameCtx)(wv_encodedFrame * inputFrame);
	/* del parser */
	bool(*wvcc_DelFrameCtx)(wv_encodedFrame * inputFrame);

	/* get picture type */
	bool(*wvcc_GetPictureType)(wv_encodedFrame * inputFrame, WV_PICTURE_TYPE * type);
	/* get picture order */
	bool(*wvcc_GetPictureDisplayOrder)(wv_encodedFrame * inputFrame, int32_t * order);

}WV_CC_Handle;

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
bool wvcc_findCCContent(wv_encodedFrame * inputFrame, uint8_t * outputCCBuffer, const uint32_t outputCCBufferLen, uint32_t * outputCCLen);

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
bool wvcc_insertCCContent(wv_encodedFrame * pstFrame, uint8_t * ccDataBuffer, uint32_t ccDataLen);


bool wvcc_getPictureType(wv_encodedFrame * inputFrame, WV_PICTURE_TYPE * pictureType);
bool wvcc_getPictureOrder(wv_encodedFrame * inputFrame, uint32_t * pictureorder);
#endif

