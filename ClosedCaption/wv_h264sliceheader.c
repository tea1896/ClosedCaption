#include "wv_h264sliceheader.h"


int32_t Parse_as_pic_nal(WV_H264SLiceHeader * pSliceHeader, WV_H264SPS * sps, WV_H264PPS * pps, uint32_t nalType, uint8_t * m_pSODB)
{
	uint32_t sliceHeaderLengthInBits = 0;
	uint32_t sodbBitPosition = 0;
	uint32_t sodbBytePosition = 0;
	uint32_t * bitPosition = &sodbBitPosition;
	uint32_t * bytePosition = &sodbBytePosition;

	pSliceHeader->m_sps_active = sps;
	pSliceHeader->m_pps_active = pps;
	pSliceHeader->m_nalType = nalType;

	pSliceHeader->m_first_mb_in_slice = Get_uev_code_num(m_pSODB, bytePosition, bitPosition);
	pSliceHeader->m_slice_type = Get_uev_code_num(m_pSODB, bytePosition, bitPosition);
	pSliceHeader->m_slice_type %= 5;
	pSliceHeader->m_pps_id = Get_uev_code_num(m_pSODB, bytePosition, bitPosition);

	if (pSliceHeader->m_sps_active->m_separate_colour_plane_flag)
	{
		pSliceHeader->m_colour_plane_id = Get_uint_code_num(m_pSODB, bytePosition, bitPosition, 2);
	}

	pSliceHeader->m_frame_num = Get_uint_code_num(m_pSODB, bytePosition, bitPosition, pSliceHeader->m_sps_active->m_log2_max_frame_num);

	if (!(pSliceHeader->m_sps_active->m_frame_mbs_only_flag))
	{
		pSliceHeader->m_field_pic_flag = Get_bit_at_position(m_pSODB, bytePosition, bitPosition);
		if (pSliceHeader->m_field_pic_flag)
		{
			pSliceHeader->m_bottom_field_flag = Get_bit_at_position(m_pSODB, bytePosition, bitPosition);
		}
	}

	if (pSliceHeader->m_nalType == 5)
	{
		pSliceHeader->m_idr_pic_id = Get_uev_code_num(m_pSODB, bytePosition, bitPosition);
	}

	if (pSliceHeader->m_sps_active->m_poc_type == 0)
	{
		pSliceHeader->m_poc = Get_uint_code_num(m_pSODB, bytePosition, bitPosition, pSliceHeader->m_sps_active->m_log2_max_poc_cnt);
		if ((!pSliceHeader->m_field_pic_flag) && pSliceHeader->m_pps_active->m_bottom_field_pic_order_in_frame_present_flag)
		{
			pSliceHeader->m_delta_poc_bottom = Get_sev_code_num(m_pSODB, bytePosition, bitPosition);
		}
	}

	if (pSliceHeader->m_nalType == 5)
	{
		pSliceHeader->m_dec_ref_pic_marking.m_no_output_of_prior_pics_flag = Get_bit_at_position(m_pSODB, bytePosition, bitPosition);
		pSliceHeader->m_dec_ref_pic_marking.m_long_term_reference_flag = Get_bit_at_position(m_pSODB, bytePosition, bitPosition);
	}

	pSliceHeader->m_slice_qp_delta = Get_sev_code_num(m_pSODB, bytePosition, bitPosition);

	if (pSliceHeader->m_pps_active->m_deblocking_filter_control_present_flag)
	{
		pSliceHeader->m_disable_deblocking_filter_idc = Get_uev_code_num(m_pSODB, bytePosition, bitPosition);
		if (pSliceHeader->m_disable_deblocking_filter_idc != 1)
		{
			pSliceHeader->m_slice_alpha_c0_offset = 2 * Get_sev_code_num(m_pSODB, bytePosition, bitPosition);
			pSliceHeader->m_slice_beta_offset = 2 * Get_sev_code_num(m_pSODB, bytePosition, bitPosition);
		}
	}

	sliceHeaderLengthInBits = 8 *(*bytePosition) + bitPosition;

	return sliceHeaderLengthInBits;
}


