#ifndef WV_H264SLICEHEADER_H
#define WV_H264SLICEHEADER_H

#include <stdint.h>
#include <stdlib.h>
#include <cstdbool>
#include "wv_h264Bits.h"
#include "wv_h264sps.h"
#include "wv_h264pps.h"

typedef struct DecRefPicMarking
{
	bool m_no_output_of_prior_pics_flag;
	bool m_long_term_reference_flag;
} WV_DecRefPicMarking;

typedef struct wv_h264sliceheader
{
	WV_H264SPS *m_sps_active;
	WV_H264PPS *m_pps_active;
	uint8_t	   *m_pSODB;
	uint32_t   m_nalType;

	uint32_t   m_disable_deblocking_filter_idc;
	uint32_t   m_slice_alpha_c0_offset;
	uint32_t   m_slice_beta_offset;

	uint32_t   m_first_mb_in_slice;
	uint32_t   m_slice_type;
	uint32_t   m_pps_id;
	uint32_t   m_colour_plane_id;
	uint32_t   m_frame_num;
	bool       m_field_pic_flag;
	bool       m_bottom_field_flag;
	uint32_t   m_idr_pic_id;
	uint32_t   m_poc;
	uint32_t   m_delta_poc_bottom;
	WV_DecRefPicMarking m_dec_ref_pic_marking;
	uint32_t   m_slice_qp_delta;

	uint32_t   m_reserved;
}WV_H264SLiceHeader;

int32_t Parse_as_pic_nal(WV_H264SLiceHeader * pSliceHeader, WV_H264SPS * sps, WV_H264PPS * pps, uint32_t nalType, uint8_t * m_pSODB);
#endif