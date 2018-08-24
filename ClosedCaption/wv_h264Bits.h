#ifndef WV_H264BITS_H
#define WV_H264BITS_H

#include <stdint.h>
#include <stdlib.h>
#include <cstdbool>

typedef uint8_t UINT8;
typedef uint32_t  UINT32;

//********************Parsing Bitstream**********************
// Get bool value from bit position..
int Get_bit_at_position(UINT8 *buf, UINT32  * bytePosition, UINT8  * bitPosition);

// Parse bit stream using Expo-Columb coding
int Get_uev_code_num(UINT8 *buf, UINT32  * bytePosition, UINT8  * bitPosition);

// Parse bit stream using signed-Expo-Columb coding
int Get_sev_code_num(UINT8 *buf, UINT32  * bytePosition, UINT8   * bitPosition);

// Parse bit stream as unsigned int bits
int Get_uint_code_num(UINT8 *buf, UINT32  * bytePosition, UINT8  * bitPosition, UINT8 length);
int Peek_uint_code_num(UINT8 *buf, UINT32 bytePosition, UINT8 bitPosition, UINT8 length);

// Parse bit stream as me(coded_block_pattern)
int Get_me_code_num(UINT8 *buf, UINT32  * bytePosition, UINT8  * bitPosition, UINT8 mode);
//***********************************************************



#endif