#ifndef WV_FFMPEG4CC_H
#define WV_FFMPEG4CC_H

/*
一些函数借用了ffmpeg的写法，但为了使得程序和ffmpeg不关联，因此将ffmpeg中的一些宏或者函数改写到此文件中
*/


/*  枚举 */
typedef enum
{
	WV_VIDEO_TYPE_MPEG2,
	WV_VIDEO_TYPE_H264,
	WV_VIDEO_TYPE_H265,

	WV_AUDIO_TYPE_MP1L2,
	WV_AUDIO_TYPE_AAC,
	WV_AUDIO_TYPE_AC3,
}WV_ENCODE_TYPE;

/*  枚举 */
typedef enum
{
	WV_PICTURE_TYPE_I,
	WV_PICTURE_TYPE_P,
	WV_PICTURE_TYPE_B,
	WV_PICTURE_TYPE_IDR,
	WV_PICTURE_TYPE_UNKNOWN,
}WV_PICTURE_TYPE;

/* 函数 */
#define WV_FFMIN(a,b) ((a) > (b) ? (b) : (a))
#define WV_RB32(x)                                \
    (((uint32_t)((const uint8_t*)(x))[0] << 24) |    \
               (((const uint8_t*)(x))[1] << 16) |    \
               (((const uint8_t*)(x))[2] <<  8) |    \
                ((const uint8_t*)(x))[3])




#endif
