#include "wv_h264sps.h"


int32_t Parse_as_seq_param_set(WV_H264SPS * sps, uint8_t * m_pSODB)
{
	uint8_t  profile_idc = 0;
	uint8_t  level_idc = 0;
	uint8_t  sps_id = 0;

	uint8_t  chroma_format_idc = 0;
	bool   separate_colour_plane_flag = 0;
	uint8_t  bit_depth_luma = 0;
	uint8_t  bit_depth_chroma = 0;
	bool   qpprime_y_zero_transform_bypass_flag = 0;
	bool   seq_scaling_matrix_present_flag = 0;

	uint32_t log2_max_frame_num = 0;
	uint8_t  poc_type = 0;
	uint32_t log2_max_poc_cnt = 0;
	uint32_t max_num_ref_frames = 0;
	bool   gaps_in_frame_num_value_allowed_flag = 0;
	uint16_t pic_width_in_mbs = 0;
	uint16_t pic_height_in_map_units = 0;
	uint16_t pic_height_in_mbs = 0;	// not defined in spec, derived...
	bool   frame_mbs_only_flag = 0;
	bool   mb_adaptive_frame_field_flag = 0;
	bool   direct_8x8_inference_flag = 0;
	bool   frame_cropping_flag = 0;
	uint32_t frame_crop_offset[4] = { 0 };
	bool   vui_parameters_present_flag = 0;

	uint8_t  sodbBitPosition = 0;
	uint32_t sodbBypePosition = 3;

	uint8_t  * bitPosition = &sodbBitPosition;
	uint32_t * bypePosition = &sodbBypePosition;
	uint32_t flags = 0;
	uint32_t idx = 0;

	profile_idc = m_pSODB[0];
	level_idc = m_pSODB[2];
	sps_id = Get_uev_code_num(m_pSODB, bypePosition, bitPosition);

	if (profile_idc == 100 || profile_idc == 110 || profile_idc == 122 || profile_idc == 244 || profile_idc == 44 ||
		profile_idc == 83 || profile_idc == 86 || profile_idc == 118 || profile_idc == 128)
	{
		chroma_format_idc = Get_uev_code_num(m_pSODB, bypePosition, bitPosition);
		if (chroma_format_idc == 3)
		{
			separate_colour_plane_flag = Get_bit_at_position(m_pSODB, bypePosition, bitPosition);
			flags |= (separate_colour_plane_flag << 21);
		}
		bit_depth_luma = Get_uev_code_num(m_pSODB, bypePosition, bitPosition) + 8;
		bit_depth_chroma = Get_uev_code_num(m_pSODB, bypePosition, bitPosition) + 8;
		qpprime_y_zero_transform_bypass_flag = Get_bit_at_position(m_pSODB, bypePosition, bitPosition);
		flags |= (qpprime_y_zero_transform_bypass_flag << 20);

		seq_scaling_matrix_present_flag = Get_bit_at_position(m_pSODB, bypePosition, bitPosition);
		flags |= (seq_scaling_matrix_present_flag << 19);
		if (seq_scaling_matrix_present_flag)
		{
			return -1;
		}
	}
	log2_max_frame_num = Get_uev_code_num(m_pSODB, bypePosition, bitPosition) + 4;
	poc_type = Get_uev_code_num(m_pSODB, bypePosition, bitPosition);
	if (0 == poc_type)
	{
		log2_max_poc_cnt = Get_uev_code_num(m_pSODB, bypePosition, bitPosition) + 4;
	}
	else
	{
		return -2;
	}
	max_num_ref_frames = Get_uev_code_num(m_pSODB, bypePosition, bitPosition);
	gaps_in_frame_num_value_allowed_flag = Get_bit_at_position(m_pSODB, bypePosition, bitPosition);
	flags |= (gaps_in_frame_num_value_allowed_flag << 5);

	pic_width_in_mbs = Get_uev_code_num(m_pSODB, bypePosition, bitPosition) + 1;
	pic_height_in_map_units = Get_uev_code_num(m_pSODB, bypePosition, bitPosition) + 1;
	frame_mbs_only_flag = Get_bit_at_position(m_pSODB, bypePosition, bitPosition);
	flags |= (frame_mbs_only_flag << 4);
	if (!frame_mbs_only_flag)
	{
		mb_adaptive_frame_field_flag = Get_bit_at_position(m_pSODB, bypePosition, bitPosition);
		flags |= (mb_adaptive_frame_field_flag << 3);
	}

	direct_8x8_inference_flag = Get_bit_at_position(m_pSODB, bypePosition, bitPosition);
	flags |= (direct_8x8_inference_flag << 2);
	frame_cropping_flag = Get_bit_at_position(m_pSODB, bypePosition, bitPosition);
	flags |= (direct_8x8_inference_flag << 1);
	if (frame_cropping_flag)
	{
		for (idx = 0; idx < 4; idx++)
		{
			frame_crop_offset[idx] = Get_uev_code_num(m_pSODB, bypePosition, bitPosition);
		}

	}
	vui_parameters_present_flag = Get_bit_at_position(m_pSODB, bypePosition, bitPosition);
	flags |= vui_parameters_present_flag;

	sps->m_profile_idc = profile_idc;
	sps->m_level_idc = level_idc;
	sps->m_sps_id = sps_id;
	sps->m_chroma_format_idc = chroma_format_idc;
	sps->m_bit_depth_luma = bit_depth_luma;
	sps->m_bit_depth_chroma = bit_depth_chroma;
	sps->m_log2_max_frame_num = log2_max_frame_num;
	sps->m_poc_type = poc_type;
	sps->m_log2_max_poc_cnt = log2_max_poc_cnt;
	sps->m_log2_max_num_ref_frames = max_num_ref_frames;

	sps->m_separate_colour_plane_flag = flags & (1 << 21);
	sps->m_qpprime_y_zero_transform_bypass_flag = flags & (1 << 20);
	sps->m_seq_scaling_matrix_present_flag = flags & (1 << 19);
	sps->m_gaps_in_frame_num_value_allowed_flag = flags & (1 << 5);
	sps->m_frame_mbs_only_flag = flags & (1 << 4);
	sps->m_mb_adaptive_frame_field_flag = flags & (1 << 3);
	sps->m_direct_8x8_inference_flag = flags & (1 << 2);
	sps->m_frame_cropping_flag = flags & (1 << 1);
	sps->m_vui_parameters_present_flag = flags & 1;

	sps->m_pic_width_in_mbs = pic_width_in_mbs;
	sps->m_pic_height_in_map_units = pic_height_in_map_units;
	sps->m_pic_height_in_mbs = sps->m_frame_mbs_only_flag ? sps->m_pic_height_in_map_units : 2 * sps->m_pic_height_in_map_units;

	if (frame_cropping_flag)
	{
		for (idx = 0; idx < 4; idx++)
		{
			sps->m_frame_crop_offset[idx] = frame_crop_offset[idx];
		}
	}

	return 0;
}