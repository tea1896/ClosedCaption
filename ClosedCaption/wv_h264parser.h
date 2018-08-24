#ifndef WV_H264PARSER_H
#define WV_H264PARSER_H

#include "wv_h264sps.h"
#include "wv_h264pps.h"

typedef struct 
{
	bool isInit;
	int32_t sps_id;
	int32_t pps_id;
	WV_H264SPS sps;
	//WV_H264PPS pps;
}WV_H264PARSER_st;


#endif
