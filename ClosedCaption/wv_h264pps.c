#include "wv_h264pps.h"



int32_t Parse_as_pic_param_set(WV_H264PPS * pps, uint8_t * m_pSODB)
{
	uint8_t  pps_id = 0;
	uint8_t  sps_id = 0;
	bool     entropy_coding_flag = 0;
	bool     bottom_field_pic_order_in_frame_present_flag = 0;
	uint8_t  num_slice_groups = 0;
	uint8_t  num_ref_idx_l0_default_active = 0;
	uint8_t  num_ref_idx_l1_default_active = 0;
	bool     weighted_pred_flag = 0;
	uint8_t  weighted_bipred_idc = 0;
	int32_t  pic_init_qp = 0;
	int32_t  pic_init_qs = 0;
	int32_t  chroma_qp_index_offset = 0;
	bool     deblocking_filter_control_present_flag = 0;
	bool     constrained_intra_pred_flag = 0;
	bool     redundant_pic_cnt_present_flag = 0;

	uint8_t    sodbBitPosition = 0;
	uint32_t   sodbBypePosition = 0;

	uint8_t   * bitPosition = &sodbBitPosition;
	uint32_t  *  bypePosition = &sodbBypePosition;
	uint16_t flags = 0;

	pps_id = Get_uev_code_num(m_pSODB, bypePosition, bitPosition);
	sps_id = Get_uev_code_num(m_pSODB, bypePosition, bitPosition);

	entropy_coding_flag = Get_bit_at_position(m_pSODB, bypePosition, bitPosition);
	flags |= entropy_coding_flag;
	bottom_field_pic_order_in_frame_present_flag = Get_bit_at_position(m_pSODB, bypePosition, bitPosition);
	flags |= bottom_field_pic_order_in_frame_present_flag << 1;

	num_slice_groups = Get_uev_code_num(m_pSODB, bypePosition, bitPosition) + 1;

	if (num_slice_groups > 1)
	{
		return -1;
	}

	num_ref_idx_l0_default_active = Get_uev_code_num(m_pSODB, bypePosition, bitPosition) + 1;
	num_ref_idx_l1_default_active = Get_uev_code_num(m_pSODB, bypePosition, bitPosition) + 1;

	weighted_pred_flag = Get_bit_at_position(m_pSODB, bypePosition, bitPosition);
	flags |= weighted_pred_flag << 2;
	weighted_bipred_idc = Get_uint_code_num(m_pSODB, bypePosition, bitPosition, 2);
//	weighted_bipred_idc = Get_bit_at_position(m_pSODB, bypePosition, bitPosition) << 1 + Get_bit_at_position(m_pSODB, bypePosition, bitPosition);
	
	pic_init_qp = Get_sev_code_num(m_pSODB, bypePosition, bitPosition) + 26;
	pic_init_qs = Get_sev_code_num(m_pSODB, bypePosition, bitPosition) + 26;
	chroma_qp_index_offset = Get_sev_code_num(m_pSODB, bypePosition, bitPosition);
	
	deblocking_filter_control_present_flag = Get_bit_at_position(m_pSODB, bypePosition, bitPosition);
	flags |= deblocking_filter_control_present_flag << 3;
	constrained_intra_pred_flag = Get_bit_at_position(m_pSODB, bypePosition, bitPosition);
	flags |= constrained_intra_pred_flag << 4;
	redundant_pic_cnt_present_flag = Get_bit_at_position(m_pSODB, bypePosition, bitPosition);
	flags |= redundant_pic_cnt_present_flag << 5;


	pps->m_pps_id = pps_id;
	pps->m_sps_id = sps_id;
	pps->m_num_slice_groups = num_slice_groups;
	pps->m_num_ref_idx_l0_default_active = num_ref_idx_l0_default_active;
	pps->m_num_ref_idx_l1_default_active = num_ref_idx_l1_default_active;
	pps->m_weighted_bipred_idc = weighted_bipred_idc;
	pps->m_pic_init_qp = pic_init_qp;
	pps->m_pic_init_qs = pic_init_qs;
	pps->m_chroma_qp_index_offset = chroma_qp_index_offset;
	pps->m_entropy_coding_flag = flags & 1;
	pps->m_bottom_field_pic_order_in_frame_present_flag = flags & (1 << 1);
	pps->m_weighted_pred_flag = flags & (1 << 2);
	pps->m_deblocking_filter_control_present_flag = flags & (1 << 3);
	pps->m_constrained_intra_pred_flag = flags & (1 << 4);
	pps->m_redundant_pic_cnt_present_flag = flags & (1 << 5);

	return 0;
}


