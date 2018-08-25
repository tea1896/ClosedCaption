#ifndef WV_H264PARSER_H
#define WV_H264PARSER_H

#include "wv_h264sps.h"
#include "wv_h264pps.h"

typedef struct
{
	int32_t PicOrderCntMsb;
	int32_t MaxPicOrderCntLsb;
	int32_t PrevPicOrderCntLsb;
	int32_t PrevPicOrderCntMsb;
}WV_H264POC_st;

typedef struct 
{
	//bool isInit;
	WV_H264POC_st poc;
	WV_H264SPS sps;
	WV_H264PPS pps;	
}WV_H264PARSER_st;


#endif
