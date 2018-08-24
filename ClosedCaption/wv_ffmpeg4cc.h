#ifndef WV_FFMPEG4CC_H
#define WV_FFMPEG4CC_H

/*
һЩ����������ffmpeg��д������Ϊ��ʹ�ó����ffmpeg����������˽�ffmpeg�е�һЩ����ߺ�����д�����ļ���
*/


/*  ö�� */
typedef enum
{
	WV_VIDEO_TYPE_MPEG2,
	WV_VIDEO_TYPE_H264,
	WV_VIDEO_TYPE_H265,

	WV_AUDIO_TYPE_MP1L2,
	WV_AUDIO_TYPE_AAC,
	WV_AUDIO_TYPE_AC3,
}WV_ENCODE_TYPE;

/*  ö�� */
typedef enum
{
	WV_PICTURE_TYPE_I,
	WV_PICTURE_TYPE_P,
	WV_PICTURE_TYPE_B,
	WV_PICTURE_TYPE_IDR,
	WV_PICTURE_TYPE_UNKNOWN,
}WV_PICTURE_TYPE;

/* ���� */
#define WV_FFMIN(a,b) ((a) > (b) ? (b) : (a))
#define WV_RB32(x)                                \
    (((uint32_t)((const uint8_t*)(x))[0] << 24) |    \
               (((const uint8_t*)(x))[1] << 16) |    \
               (((const uint8_t*)(x))[2] <<  8) |    \
                ((const uint8_t*)(x))[3])




#endif
