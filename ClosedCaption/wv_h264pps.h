#ifndef WV_H264PPS_H
#define WV_H264PPS_H

#include <stdint.h>
#include <stdlib.h>
#include <cstdbool>
#include "wv_h264Bits.h"

typedef struct wv_h264pps
{
	uint8_t  m_pps_id;
	uint8_t  m_sps_id;
	bool     m_entropy_coding_flag;
	bool     m_bottom_field_pic_order_in_frame_present_flag;
	uint8_t  m_num_slice_groups;
	uint8_t  m_num_ref_idx_l0_default_active;
	uint8_t  m_num_ref_idx_l1_default_active;
	bool     m_weighted_pred_flag;
	uint8_t  m_weighted_bipred_idc;
	int32_t  m_pic_init_qp;
	int32_t  m_pic_init_qs;
	int32_t  m_chroma_qp_index_offset;
	bool     m_deblocking_filter_control_present_flag;
	bool     m_constrained_intra_pred_flag;
	bool     m_redundant_pic_cnt_present_flag;
	bool     m_transform_8x8_mode_flag;
}WV_H264PPS;

int32_t Parse_as_pic_param_set(WV_H264PPS * sps, uint8_t * m_pSODB);



#endif