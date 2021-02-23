/***************************************************************************
 *   Copyright (C) 2008 by root   *
 *   root@skystar   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#include "qstdinc.h"
#include "qdvbts.h"

/* Parser helpers */
#define QDVB_CheckFinish() if (dsIndex>=dsSize) goto dsBounds;
#define QDVB_GotError() goto dsBounds;
#define QDVB_GotSuccess() goto dsBounds;
#define QDVB_NextPos() if (dsMask == 0x01) { dsMask = 0x80; dsIndex++; } else dsMask >>= 1;
#define QDVB_NextPosW() if (dsMask == 0x80) { dsMask = 0x01; dsIndex++; } else dsMask <<= 1;
#define QDVB_NextBit(D) D <<= 1; if (dsData[dsIndex] & dsMask) D |= 1; QDVB_NextPos();
#define QDVB_ReadBits(D, X) D = 0; dsBits = X; while(dsBits--) { QDVB_CheckFinish(); QDVB_NextBit(D); }
#define QDVB_ReadBitsNZ(D, X) dsBits = X; while(dsBits--) { QDVB_CheckFinish(); QDVB_NextBit(D); }
#define QDVB_ReadBitsCheck(X, C) nDummy2 = 0; QDVB_ReadBits(nDummy2, X); if (nDummy2 != C) goto dsBounds;
#define QDVB_SkipBits(X) dsBits = X; while(dsBits--) { QDVB_CheckFinish(); QDVB_NextBit(nDummy); }
#define QDVB_Start(P, N) uint8_t* dsData = (uint8_t*)P; long dsSize; \
	uint8_t dsMask = 0x80; long dsIndex = 0; long dsBits = 0; bool dsBCError = false; long nDummy; uint64_t nDummy2; nDummy = 0; nDummy2 = 0; dsSize = (long)N;

#define QDVB_StartWrite(P, N) uint8_t* dsData = (uint8_t*)P; long& dsSize = N; \
	uint8_t dsMask = 0x80; long dsIndex = 0; long dsBits = 0; bool dsBCError = false; long nDummy; uint64_t nDummy2; uint64_t nData; nData = 0; nDummy = 0; nDummy2 = 0; dsSize = dsSize;
#define QDVB_WriteNextBit() if (nData & nDummy2) dsData[dsIndex] |= dsMask; else dsData[dsIndex] &= ~dsMask; QDVB_NextPos();
#define QDVB_WriteBits(D, X) dsBits = X; nData = (uint64_t)D; nDummy2 = (uint64_t)(((uint64_t)1) << (uint64_t)(dsBits - 1)); while(dsBits--) { QDVB_WriteNextBit(); nDummy2 >>= 1; }

#define QDVB_BoundsError goto dsExit; dsBounds: dsBCError = true; dsExit:;
#define QDVB_BCError() !dsBCError
#define QDVB_RoundNextByte() if (dsMask != 0x80) { dsMask = 0x80; dsIndex++; }
#define QDVB_CurrentBuffer() (uint8_t*)&dsData[dsIndex]
#define QDVB_CurrentSize() (dsSize - dsIndex)

static uint32_t s_crc32_table[256] =
{
  0x00000000, 0x04c11db7, 0x09823b6e, 0x0d4326d9,
  0x130476dc, 0x17c56b6b, 0x1a864db2, 0x1e475005,
  0x2608edb8, 0x22c9f00f, 0x2f8ad6d6, 0x2b4bcb61,
  0x350c9b64, 0x31cd86d3, 0x3c8ea00a, 0x384fbdbd,
  0x4c11db70, 0x48d0c6c7, 0x4593e01e, 0x4152fda9,
  0x5f15adac, 0x5bd4b01b, 0x569796c2, 0x52568b75,
  0x6a1936c8, 0x6ed82b7f, 0x639b0da6, 0x675a1011,
  0x791d4014, 0x7ddc5da3, 0x709f7b7a, 0x745e66cd,
  0x9823b6e0, 0x9ce2ab57, 0x91a18d8e, 0x95609039,
  0x8b27c03c, 0x8fe6dd8b, 0x82a5fb52, 0x8664e6e5,
  0xbe2b5b58, 0xbaea46ef, 0xb7a96036, 0xb3687d81,
  0xad2f2d84, 0xa9ee3033, 0xa4ad16ea, 0xa06c0b5d,
  0xd4326d90, 0xd0f37027, 0xddb056fe, 0xd9714b49,
  0xc7361b4c, 0xc3f706fb, 0xceb42022, 0xca753d95,
  0xf23a8028, 0xf6fb9d9f, 0xfbb8bb46, 0xff79a6f1,
  0xe13ef6f4, 0xe5ffeb43, 0xe8bccd9a, 0xec7dd02d,
  0x34867077, 0x30476dc0, 0x3d044b19, 0x39c556ae,
  0x278206ab, 0x23431b1c, 0x2e003dc5, 0x2ac12072,
  0x128e9dcf, 0x164f8078, 0x1b0ca6a1, 0x1fcdbb16,
  0x018aeb13, 0x054bf6a4, 0x0808d07d, 0x0cc9cdca,
  0x7897ab07, 0x7c56b6b0, 0x71159069, 0x75d48dde,
  0x6b93dddb, 0x6f52c06c, 0x6211e6b5, 0x66d0fb02,
  0x5e9f46bf, 0x5a5e5b08, 0x571d7dd1, 0x53dc6066,
  0x4d9b3063, 0x495a2dd4, 0x44190b0d, 0x40d816ba,
  0xaca5c697, 0xa864db20, 0xa527fdf9, 0xa1e6e04e,
  0xbfa1b04b, 0xbb60adfc, 0xb6238b25, 0xb2e29692,
  0x8aad2b2f, 0x8e6c3698, 0x832f1041, 0x87ee0df6,
  0x99a95df3, 0x9d684044, 0x902b669d, 0x94ea7b2a,
  0xe0b41de7, 0xe4750050, 0xe9362689, 0xedf73b3e,
  0xf3b06b3b, 0xf771768c, 0xfa325055, 0xfef34de2,
  0xc6bcf05f, 0xc27dede8, 0xcf3ecb31, 0xcbffd686,
  0xd5b88683, 0xd1799b34, 0xdc3abded, 0xd8fba05a,
  0x690ce0ee, 0x6dcdfd59, 0x608edb80, 0x644fc637,
  0x7a089632, 0x7ec98b85, 0x738aad5c, 0x774bb0eb,
  0x4f040d56, 0x4bc510e1, 0x46863638, 0x42472b8f,
  0x5c007b8a, 0x58c1663d, 0x558240e4, 0x51435d53,
  0x251d3b9e, 0x21dc2629, 0x2c9f00f0, 0x285e1d47,
  0x36194d42, 0x32d850f5, 0x3f9b762c, 0x3b5a6b9b,
  0x0315d626, 0x07d4cb91, 0x0a97ed48, 0x0e56f0ff,
  0x1011a0fa, 0x14d0bd4d, 0x19939b94, 0x1d528623,
  0xf12f560e, 0xf5ee4bb9, 0xf8ad6d60, 0xfc6c70d7,
  0xe22b20d2, 0xe6ea3d65, 0xeba91bbc, 0xef68060b,
  0xd727bbb6, 0xd3e6a601, 0xdea580d8, 0xda649d6f,
  0xc423cd6a, 0xc0e2d0dd, 0xcda1f604, 0xc960ebb3,
  0xbd3e8d7e, 0xb9ff90c9, 0xb4bcb610, 0xb07daba7,
  0xae3afba2, 0xaafbe615, 0xa7b8c0cc, 0xa379dd7b,
  0x9b3660c6, 0x9ff77d71, 0x92b45ba8, 0x9675461f,
  0x8832161a, 0x8cf30bad, 0x81b02d74, 0x857130c3,
  0x5d8a9099, 0x594b8d2e, 0x5408abf7, 0x50c9b640,
  0x4e8ee645, 0x4a4ffbf2, 0x470cdd2b, 0x43cdc09c,
  0x7b827d21, 0x7f436096, 0x7200464f, 0x76c15bf8,
  0x68860bfd, 0x6c47164a, 0x61043093, 0x65c52d24,
  0x119b4be9, 0x155a565e, 0x18197087, 0x1cd86d30,
  0x029f3d35, 0x065e2082, 0x0b1d065b, 0x0fdc1bec,
  0x3793a651, 0x3352bbe6, 0x3e119d3f, 0x3ad08088,
  0x2497d08d, 0x2056cd3a, 0x2d15ebe3, 0x29d4f654,
  0xc5a92679, 0xc1683bce, 0xcc2b1d17, 0xc8ea00a0,
  0xd6ad50a5, 0xd26c4d12, 0xdf2f6bcb, 0xdbee767c,
  0xe3a1cbc1, 0xe760d676, 0xea23f0af, 0xeee2ed18,
  0xf0a5bd1d, 0xf464a0aa, 0xf9278673, 0xfde69bc4,
  0x89b8fd09, 0x8d79e0be, 0x803ac667, 0x84fbdbd0,
  0x9abc8bd5, 0x9e7d9662, 0x933eb0bb, 0x97ffad0c,
  0xafb010b1, 0xab710d06, 0xa6322bdf, 0xa2f33668,
  0xbcb4666d, 0xb8757bda, 0xb5365d03, 0xb1f740b4
};


bool QDVBValidCRC(uint8_t* pData, uint32_t nSize)
{
	// Create CRC32
    uint32_t nCRC32 = 0xffffffff;

    for (int i=0; i<(int)(nSize - 4); i++)
    {
		uint8_t nByte = pData[i];
    	nCRC32 =  (nCRC32 << 8) ^ s_crc32_table[(nCRC32 >> 24) ^ nByte];
    }

	uint32_t nCRCOrg = 0;
	nCRCOrg = pData[nSize - 4];  nCRCOrg = nCRCOrg << 8;
	nCRCOrg = nCRCOrg | ((uint32_t)(pData[nSize - 3])); nCRCOrg = nCRCOrg << 8;
	nCRCOrg = nCRCOrg | ((uint32_t)(pData[nSize - 2])); nCRCOrg = nCRCOrg << 8;
	nCRCOrg = nCRCOrg | ((uint32_t)(pData[nSize - 1]));

	return nCRCOrg == nCRC32;
}

uint32_t QDVBCalcCRC(uint8_t* pStart, uint8_t* pEnd)
{
	int nSize = pEnd - pStart;
	uint32_t nCRC32 = 0xffffffff;
	for (int i=0; i<nSize; i++)
	{
		uint8_t nByte = pStart[i];
		nCRC32 =  (nCRC32 << 8) ^ s_crc32_table[(nCRC32 >> 24) ^ nByte];
	}

	return nCRC32;
}

bool QDVBParseTS(uint8_t* pData, QDVBTSPacket* pTS)
{
	QDVB_Start(pData, 188);

	memset(pTS, 0, sizeof(QDVBTSPacket));

	// Common
	QDVB_ReadBits(pTS->sync_byte, 8);
	QDVB_ReadBits(pTS->transport_error_indicator, 1);
	QDVB_ReadBits(pTS->payload_unit_start_indicator, 1);
	QDVB_ReadBits(pTS->transport_priority, 1);
	QDVB_ReadBits(pTS->PID, 13);
	QDVB_ReadBits(pTS->transport_scrambling_control, 2);
	QDVB_ReadBits(pTS->adaptation_field_control, 2);
	QDVB_ReadBits(pTS->continuity_counter, 4);

	// Read adaptation here
	if(pTS->adaptation_field_control & 0x2)
	{
		QDVB_ReadBits(pTS->adaptation_field_length, 8);
		long nCurIndex = dsIndex;
		if (pTS->adaptation_field_length > 0)
		{
			QDVB_ReadBits(pTS->discontinuity_indicator, 1);
			QDVB_ReadBits(pTS->random_access_indicator, 1);
			QDVB_ReadBits(pTS->elementary_stream_priority_indicator, 1);
			QDVB_ReadBits(pTS->PCR_flag, 1);
			QDVB_ReadBits(pTS->OPCR_flag, 1);
			QDVB_ReadBits(pTS->splicing_point_flag, 1);
			QDVB_ReadBits(pTS->transport_private_data_flag, 1);
			QDVB_ReadBits(pTS->adaptation_field_extension_flag, 1);
			// PCR
			if (pTS->PCR_flag == 1)
			{
				QDVB_ReadBits(pTS->program_clock_reference_base, 33);
				QDVB_SkipBits(6);
				QDVB_ReadBits(pTS->program_clock_reference_extension, 9);
			}

			// OPCR
			if (pTS->OPCR_flag == 1)
			{
				QDVB_ReadBits(pTS->original_program_clock_reference_base, 33);
				QDVB_SkipBits(6);
				QDVB_ReadBits(pTS->original_program_clock_reference_extension, 9);
			}

			// Splice
			if (pTS->splicing_point_flag == 1)
			{
				QDVB_ReadBits(pTS->splice_countdown, 8);
			}

			// Private
			if (pTS->transport_private_data_flag == 1)
			{
				QDVB_ReadBits(pTS->transport_private_data_length, 8);
				pTS->private_data_byte = QDVB_CurrentBuffer();
				QDVB_SkipBits(8 * pTS->transport_private_data_length);
			}

			// Extension
			if (pTS->adaptation_field_extension_flag == 1)
			{
				QDVB_ReadBits(pTS->adaptation_field_extension_length, 8);
				QDVB_ReadBits(pTS->ltw_flag, 1);
				QDVB_ReadBits(pTS->piecewise_rate_flag, 1);
				QDVB_ReadBits(pTS->seamless_splice_flag, 1);
				QDVB_SkipBits(5);

				// LTW
				if (pTS->ltw_flag == 1) 
				{
					QDVB_ReadBits(pTS->ltw_valid_flag, 1);
					QDVB_ReadBits(pTS->ltw_offset, 15);
				}

				// Piecewise
				if (pTS->piecewise_rate_flag == 1) 
				{
					QDVB_SkipBits(2);
					QDVB_ReadBits(pTS->piecewise_rate, 22);
				}
				
				// Seamless splices
				if (pTS->seamless_splice_flag == 1) 
				{
					QDVB_ReadBits(pTS->splice_type, 4);
					QDVB_ReadBits(pTS->DTS_next_AU, 3);
					QDVB_SkipBits(1);
					QDVB_ReadBits(pTS->DTS_next_AU, 15);
					QDVB_SkipBits(1);
					QDVB_ReadBits(pTS->DTS_next_AU, 15);
					QDVB_SkipBits(1);
				}

			}
		}

		QDVB_RoundNextByte();
		dsIndex = nCurIndex + (pTS->adaptation_field_length);
	}

	// Fix buffer position
	pTS->data = QDVB_CurrentBuffer();
	pTS->data_size = (uint8_t)QDVB_CurrentSize();

	QDVB_BoundsError;
	return QDVB_BCError();
}

bool QDVBMakeTS(uint8_t* pData, QDVBTSPacket* pTS)
{
	long nTmp = 0;
	QDVB_StartWrite(pData, nTmp);

	// Common
	QDVB_WriteBits(pTS->sync_byte, 8);
	QDVB_WriteBits(pTS->transport_error_indicator, 1);
	QDVB_WriteBits(pTS->payload_unit_start_indicator, 1);
	QDVB_WriteBits(pTS->transport_priority, 1);
	QDVB_WriteBits(pTS->PID, 13);
	QDVB_WriteBits(pTS->transport_scrambling_control, 2);
	QDVB_WriteBits(pTS->adaptation_field_control, 2);
	QDVB_WriteBits(pTS->continuity_counter, 4);

	// Read adaptation here
	if (pTS->adaptation_field_control & 0x2)
	{
		QDVB_WriteBits(pTS->adaptation_field_length, 8);
		long nCurIndex = dsIndex;
		if (pTS->adaptation_field_length > 0)
		{
			QDVB_WriteBits(pTS->discontinuity_indicator, 1);
			QDVB_WriteBits(pTS->random_access_indicator, 1);
			QDVB_WriteBits(pTS->elementary_stream_priority_indicator, 1);
			QDVB_WriteBits(pTS->PCR_flag, 1);
			QDVB_WriteBits(pTS->OPCR_flag, 1);
			QDVB_WriteBits(pTS->splicing_point_flag, 1);
			QDVB_WriteBits(pTS->transport_private_data_flag, 1);
			QDVB_WriteBits(pTS->adaptation_field_extension_flag, 1);
			// PCR
			if (pTS->PCR_flag == 1)
			{
				QDVB_WriteBits(pTS->program_clock_reference_base, 33);
				QDVB_WriteBits(0, 6);
				QDVB_WriteBits(pTS->program_clock_reference_extension, 9);
			}

			// OPCR
			if (pTS->OPCR_flag == 1)
			{
				QDVB_WriteBits(pTS->original_program_clock_reference_base, 33);
				QDVB_WriteBits(0, 6);
				QDVB_WriteBits(pTS->original_program_clock_reference_extension,
						9);
			}

			// Splice
			if (pTS->splicing_point_flag == 1)
			{
				QDVB_WriteBits(pTS->splice_countdown, 8);
			}

			// Private
			if (pTS->transport_private_data_flag == 1)
			{
				QDVB_WriteBits(pTS->transport_private_data_length, 8);
				for (int i = 0; i < pTS->transport_private_data_length; i++)
				{
					QDVB_WriteBits(pTS->private_data_byte[i], 8);
				}
			}

			// Extension
			if (pTS->adaptation_field_extension_flag == 1)
			{
				QDVB_WriteBits(pTS->adaptation_field_extension_length, 8);
				QDVB_WriteBits(pTS->ltw_flag, 1);
				QDVB_WriteBits(pTS->piecewise_rate_flag, 1);
				QDVB_WriteBits(pTS->seamless_splice_flag, 1);
				QDVB_WriteBits(0, 5);

				// LTW
				if (pTS->ltw_flag == 1)
				{
					QDVB_WriteBits(pTS->ltw_valid_flag, 1);
					QDVB_WriteBits(pTS->ltw_offset, 15);
				}

				// Piecewise
				if (pTS->piecewise_rate_flag == 1)
				{
					QDVB_WriteBits(0, 2);
					QDVB_WriteBits(pTS->piecewise_rate, 22);
				}

				// Seamless splices
				if (pTS->seamless_splice_flag == 1)
				{
					QDVB_WriteBits(pTS->splice_type, 4);
					QDVB_WriteBits(pTS->DTS_next_AU >> 30, 3);
					QDVB_WriteBits(0, 1);
					QDVB_WriteBits(pTS->DTS_next_AU >> 15, 15);
					QDVB_WriteBits(0, 1);
					QDVB_WriteBits(pTS->DTS_next_AU, 15);
					QDVB_WriteBits(0, 1);
				}

			}
		}

		QDVB_RoundNextByte();
		dsIndex = nCurIndex + (pTS->adaptation_field_length);
	}

	// Fix buffer position
	for (int i = 0; i < pTS->data_size; i++)
	{
		QDVB_WriteBits(pTS->data[i], 8);
	}

	return QDVB_BCError();
}

bool QDVBPatchPCR(uint8_t* pData, QDVBTSPacket* pTS)
{
	long nTmp = 0;
	QDVB_StartWrite(pData, nTmp);

	// Common
	QDVB_WriteBits(pTS->sync_byte, 8);
	QDVB_WriteBits(pTS->transport_error_indicator, 1);
	QDVB_WriteBits(pTS->payload_unit_start_indicator, 1);
	QDVB_WriteBits(pTS->transport_priority, 1);
	QDVB_WriteBits(pTS->PID, 13);
	QDVB_WriteBits(pTS->transport_scrambling_control, 2);
	QDVB_WriteBits(pTS->adaptation_field_control, 2);
	QDVB_WriteBits(pTS->continuity_counter, 4);

	if (pTS->adaptation_field_control & 0x2)
	{
		QDVB_WriteBits(pTS->adaptation_field_length, 8);
		if (pTS->adaptation_field_length > 0)
		{
			QDVB_WriteBits(pTS->discontinuity_indicator, 1);
			QDVB_WriteBits(pTS->random_access_indicator, 1);
			QDVB_WriteBits(pTS->elementary_stream_priority_indicator, 1);
			QDVB_WriteBits(pTS->PCR_flag, 1);
			QDVB_WriteBits(pTS->OPCR_flag, 1);
			QDVB_WriteBits(pTS->splicing_point_flag, 1);
			QDVB_WriteBits(pTS->transport_private_data_flag, 1);
			QDVB_WriteBits(pTS->adaptation_field_extension_flag, 1);

			// PCR
			if (pTS->PCR_flag == 1)
			{
				QDVB_WriteBits(pTS->program_clock_reference_base, 33);
			}
		}
	}

	return QDVB_BCError();
}

void QDVBPatchCC(uint8_t* pData, int nCC)
{
	pData[0x03] = (pData[0x03] & 0xf0) | (nCC & 0x0f);
}


bool QDVBParsePES(uint8_t* pData, int nSize, QDVBPESHdr* pPES, bool bHdrOnly)
{
	QDVB_Start(pData, nSize);
	uint8_t temp = 0;
	int i = 0;
	uint8_t* pExtPtr = NULL;
	uint64_t nComb[3];

	memset(pPES, 0, sizeof(QDVBPESHdr));

	// Common
	QDVB_ReadBits(pPES->packet_start_code_prefix, 24);
	QDVB_ReadBits(pPES->stream_id, 8);
	QDVB_ReadBits(pPES->PES_packet_length, 16);

	QDVB_ReadBits(temp, 2);									// 2 bslbf '10'
	QDVB_ReadBits(pPES->PES_scrambling_control, 2);			// 2 bslbf
	QDVB_ReadBits(pPES->PES_priority, 1);					// 1 bslbf
	QDVB_ReadBits(pPES->data_alignment_indicator, 1);		// 1 bslbf
	QDVB_ReadBits(pPES->copyright, 1);						// 1 bslbf
	QDVB_ReadBits(pPES->original_or_copy, 1);				// 1 bslbf
	QDVB_ReadBits(pPES->PTS_DTS_flags, 2);					// 2 bslbf
	QDVB_ReadBits(pPES->ESCR_flag, 1);						// 1 bslbf
	QDVB_ReadBits(pPES->ES_rate_flag, 1);					// 1 bslbf
	QDVB_ReadBits(pPES->DSM_trick_mode_flag, 1);				// 1 bslbf
	QDVB_ReadBits(pPES->additional_copy_info_flag, 1);		// 1 bslbf
	QDVB_ReadBits(pPES->PES_CRC_flag, 1);					// 1 bslbf
	QDVB_ReadBits(pPES->PES_extension_flag, 1);				// 1 bslbf
	QDVB_ReadBits(pPES->PES_header_data_length, 8);			// 8 uimsbf
	pExtPtr = QDVB_CurrentBuffer();

	// DTS modes
	if (pPES->PTS_DTS_flags == 2)
	{
		nComb[2] = nComb[1] = nComb[0] = 0;
		QDVB_ReadBits(temp, 4);
		QDVB_ReadBits(nComb[2], 3);
		QDVB_SkipBits(1);
		QDVB_ReadBits(nComb[1], 15);
		QDVB_SkipBits(1);
		QDVB_ReadBits(nComb[0], 15);
		QDVB_SkipBits(1);
		pPES->PTS = nComb[0] | (nComb[1] << 15) | (nComb[2] << 30);
	}
	else if (pPES->PTS_DTS_flags == 3)
	{
		nComb[2] = nComb[1] = nComb[0] = 0;
		QDVB_ReadBits(temp, 4);
		QDVB_ReadBits(nComb[2], 3);
		QDVB_SkipBits(1);
		QDVB_ReadBits(nComb[1], 15);
		QDVB_SkipBits(1);
		QDVB_ReadBits(nComb[0], 15);
		QDVB_SkipBits(1);
		pPES->PTS = nComb[0] | (nComb[1] << 15) | (nComb[2] << 30);

		nComb[2] = nComb[1] = nComb[0] = 0;
		QDVB_ReadBits(temp, 4);
		QDVB_ReadBits(nComb[2], 3);
		QDVB_SkipBits(1);
		QDVB_ReadBits(nComb[1], 15);
		QDVB_SkipBits(1);
		QDVB_ReadBits(nComb[0], 15);
		QDVB_SkipBits(1);
		pPES->DTS = nComb[0] | (nComb[1] << 15) | (nComb[2] << 30);
	}

	// ESCR
	if (pPES->ESCR_flag == 1)
	{
		nComb[2] = nComb[1] = nComb[0] = 0;
		QDVB_ReadBits(temp, 2);
		QDVB_ReadBits(nComb[2], 3);
		QDVB_SkipBits(1);
		QDVB_ReadBits(nComb[1], 15);
		QDVB_SkipBits(1);
		QDVB_ReadBits(nComb[0], 15);
		QDVB_SkipBits(1);
		QDVB_ReadBits(pPES->ESCR_ext, 9);
		QDVB_SkipBits(1);
		pPES->ESCR_base = nComb[0] | (nComb[1] << 15) | (nComb[2] << 30);
	}

	// ESCR rate
	if (pPES->ES_rate_flag == 1)
	{
		QDVB_SkipBits(1);
		QDVB_ReadBits(pPES->ES_rate, 22);
		QDVB_SkipBits(1);
	}

	// DSM_trick_mode_flag
	if (pPES->DSM_trick_mode_flag == 1)
	{
		QDVB_ReadBits(pPES->trick_mode_control, 3);
		if (pPES->trick_mode_control == 0)
		{
			QDVB_ReadBits(pPES->field_id, 2);
			QDVB_ReadBits(pPES->intra_slice_refresh, 1);
			QDVB_ReadBits(pPES->frequency_truncation, 2);
		}
		else if (pPES->trick_mode_control == 1)
		{
			QDVB_ReadBits(pPES->rep_cntrl, 5);
		}
		else if (pPES->trick_mode_control == 2)
		{
			QDVB_ReadBits(pPES->field_id, 2);
			QDVB_SkipBits(3);
		}
		else if (pPES->trick_mode_control == 3)
		{
			QDVB_ReadBits(pPES->field_id, 2);
			QDVB_ReadBits(pPES->intra_slice_refresh, 1);
			QDVB_ReadBits(pPES->frequency_truncation, 2);
		}
		else if (pPES->trick_mode_control == 4)
		{
			QDVB_ReadBits(pPES->rep_cntrl, 5);
		}
		else
		{
			QDVB_ReadBits(temp, 5);
		}
	}

	if (pPES->additional_copy_info_flag == 1)
	{
		QDVB_SkipBits(1);
		QDVB_ReadBits(pPES->additional_copy_info, 7);
	}

	if (pPES->PES_CRC_flag == 1)
	{
		QDVB_ReadBits(pPES->previous_PES_packet_CRC, 16);
	}

	if (pPES->PES_extension_flag == 1)
	{
		QDVB_ReadBits(pPES->PES_private_data_flag, 1);
		QDVB_ReadBits(pPES->pack_header_field_flag, 1);
		QDVB_ReadBits(pPES->program_packet_sequence_counter_flag, 1);
		QDVB_ReadBits(pPES->P_STD_buffer_flag, 1);
		QDVB_SkipBits(3);
		QDVB_ReadBits(pPES->PES_extension_flag_2, 1);

		if (pPES->PES_private_data_flag == 1)
		{
			for (int i=0; i<16; i++)
			{
				QDVB_ReadBits(pPES->private_data[i], 8);
			}
		}

		if (pPES->pack_header_field_flag == 1)
		{
			QDVB_ReadBits(pPES->pack_field_length, 8);
			for (int i=0; i<pPES->pack_field_length; i++)
			{
				QDVB_ReadBits(pPES->pack_field[i], 8);
			}
		}

		if (pPES->program_packet_sequence_counter_flag == 1)
		{
			QDVB_SkipBits(1);
			QDVB_ReadBits(pPES->program_packet_sequence_counter, 7);
			QDVB_SkipBits(1);
			QDVB_ReadBits(pPES->MPEG1_MPEG2_identifier, 1);
			QDVB_ReadBits(pPES->original_stuff_length, 6);
		}

		if (pPES->P_STD_buffer_flag == 1)
		{
			QDVB_SkipBits(2);
			QDVB_ReadBits(pPES->P_STD_buffer_scale, 1);
			QDVB_ReadBits(pPES->P_STD_buffer_size, 13);
		}

		if (pPES->PES_extension_flag_2 == 1)
		{
			QDVB_SkipBits(1);
			QDVB_ReadBits(pPES->PES_extension_field_length, 7);
			for (i = 0; i < pPES->PES_extension_field_length; i++)
			{
				QDVB_ReadBits(pPES->PES_extension_field[i], 8);
			}
		}
	}

	if (!bHdrOnly)
	{
		while((QDVB_CurrentBuffer() - pExtPtr) < pPES->PES_header_data_length)
		{
			// Skip stuffing bytes
			QDVB_SkipBits(8);
		}

		// Data is here
		pPES->data = QDVB_CurrentBuffer();
		pPES->data_size = QDVB_CurrentSize();
	}

QDVB_BoundsError;
	return QDVB_BCError() && (pPES->packet_start_code_prefix == 1);
}

bool QDVBCreatePES(uint8_t* pBuffer, long& nBufferSize, uint8_t* pData, long nDataSize, QDVBPESHdr* pPES, bool bUnlim)
{
	QDVB_StartWrite(pBuffer, nBufferSize);
	uint8_t* pHdrPos = NULL;
	uint8_t* pLenPos = NULL;
	uint16_t nComb[3];
	int nHdrLen = 0;
	int nPESLen = 0;

	// Common
	QDVB_WriteBits(pPES->packet_start_code_prefix, 24);
	QDVB_WriteBits(pPES->stream_id, 8);
	pLenPos = QDVB_CurrentBuffer();
	QDVB_WriteBits(0, 16);

	QDVB_WriteBits(2, 2);									// 2 bslbf '10'
	QDVB_WriteBits(pPES->PES_scrambling_control, 2);			// 2 bslbf
	QDVB_WriteBits(pPES->PES_priority, 1);					// 1 bslbf
	QDVB_WriteBits(pPES->data_alignment_indicator, 1);		// 1 bslbf
	QDVB_WriteBits(pPES->copyright, 1);						// 1 bslbf
	QDVB_WriteBits(pPES->original_or_copy, 1);				// 1 bslbf
	QDVB_WriteBits(pPES->PTS_DTS_flags, 2);					// 2 bslbf
	QDVB_WriteBits(pPES->ESCR_flag, 1);						// 1 bslbf
	QDVB_WriteBits(pPES->ES_rate_flag, 1);					// 1 bslbf
	QDVB_WriteBits(pPES->DSM_trick_mode_flag, 1);			// 1 bslbf
	QDVB_WriteBits(pPES->additional_copy_info_flag, 1);		// 1 bslbf
	QDVB_WriteBits(pPES->PES_CRC_flag, 1);					// 1 bslbf
	QDVB_WriteBits(pPES->PES_extension_flag, 1);				// 1 bslbf
	pHdrPos = QDVB_CurrentBuffer();
	QDVB_WriteBits(0, 8);									// 8 uimsbf

	// DTS modes
	if (pPES->PTS_DTS_flags == 2)
	{
		QDVB_WriteBits(2, 4);				// 4 bslbf

		nComb[0] = pPES->PTS & 0x7fff;
		nComb[1] = (pPES->PTS >> 15) & 0x7fff;
		nComb[2] = (pPES->PTS >> 30) & 0x7;

		QDVB_WriteBits(nComb[2], 3);
		QDVB_WriteBits(1, 1);
		QDVB_WriteBits(nComb[1], 15);
		QDVB_WriteBits(1, 1);
		QDVB_WriteBits(nComb[0], 15);
		QDVB_WriteBits(1, 1);
	}
	else if (pPES->PTS_DTS_flags == 3)
	{
		QDVB_WriteBits(3, 4);				// 4 bslbf

		nComb[0] = pPES->PTS & 0x7fff;
		nComb[1] = (pPES->PTS >> 15) & 0x7fff;
		nComb[2] = (pPES->PTS >> 30) & 0x7;

		QDVB_WriteBits(nComb[2], 3);
		QDVB_WriteBits(1, 1);
		QDVB_WriteBits(nComb[1], 15);
		QDVB_WriteBits(1, 1);
		QDVB_WriteBits(nComb[0], 15);
		QDVB_WriteBits(1, 1);

		QDVB_WriteBits(1, 4);				// 4 bslbf

		nComb[0] = pPES->DTS & 0x7fff;
		nComb[1] = (pPES->DTS >> 15) & 0x7fff;
		nComb[2] = (pPES->DTS >> 30) & 0x7;

		QDVB_WriteBits(nComb[2], 3);
		QDVB_WriteBits(1, 1);
		QDVB_WriteBits(nComb[1], 15);
		QDVB_WriteBits(1, 1);
		QDVB_WriteBits(nComb[0], 15);
		QDVB_WriteBits(1, 1);
	}

	// ESCR
	if (pPES->ESCR_flag == 1)
	{
		QDVB_WriteBits(0, 2);

		nComb[0] = pPES->ESCR_base & 0x7fff;
		nComb[1] = (pPES->ESCR_base >> 15) & 0x7fff;
		nComb[2] = (pPES->ESCR_base >> 30) & 0x7;

		QDVB_WriteBits(nComb[2], 3);
		QDVB_WriteBits(1, 1);
		QDVB_WriteBits(nComb[1], 15);
		QDVB_WriteBits(1, 1);
		QDVB_WriteBits(nComb[0], 15);
		QDVB_WriteBits(1, 1);
		QDVB_WriteBits(pPES->ESCR_ext, 9);
		QDVB_WriteBits(1, 1);
	}

	// ESCR rate
	if (pPES->ES_rate_flag == 1)
	{
		QDVB_WriteBits(1, 1);
		QDVB_WriteBits(pPES->ES_rate, 22);
		QDVB_WriteBits(1, 1);
	}

	// DSM_trick_mode_flag
	if (pPES->DSM_trick_mode_flag == 1)
	{
		QDVB_WriteBits(pPES->trick_mode_control, 3);
		if (pPES->trick_mode_control == 0)
		{
			QDVB_WriteBits(pPES->field_id, 2);
			QDVB_WriteBits(pPES->intra_slice_refresh, 1);
			QDVB_WriteBits(pPES->frequency_truncation, 2);
		}
		else if (pPES->trick_mode_control == 1)
		{
			QDVB_WriteBits(pPES->rep_cntrl, 5);
		}
		else if (pPES->trick_mode_control == 2)
		{
			QDVB_WriteBits(pPES->field_id, 2);
			QDVB_WriteBits(0, 3);
		}
		else if (pPES->trick_mode_control == 3)
		{
			QDVB_WriteBits(pPES->field_id, 2);
			QDVB_WriteBits(pPES->intra_slice_refresh, 1);
			QDVB_WriteBits(pPES->frequency_truncation, 2);
		}
		else if (pPES->trick_mode_control == 4)
		{
			QDVB_WriteBits(pPES->rep_cntrl, 5);
		}
		else
		{
			QDVB_WriteBits(0, 5);
		}
	}

	if (pPES->additional_copy_info_flag == 1)
	{
		QDVB_WriteBits(1, 1);
		QDVB_WriteBits(pPES->additional_copy_info, 7);
	}

	if (pPES->PES_CRC_flag == 1)
	{
		QDVB_WriteBits(pPES->previous_PES_packet_CRC, 16);
	}

	if (pPES->PES_extension_flag == 1)
	{
		QDVB_WriteBits(pPES->PES_private_data_flag, 1);
		QDVB_WriteBits(pPES->pack_header_field_flag, 1);
		QDVB_WriteBits(pPES->program_packet_sequence_counter_flag, 1);
		QDVB_WriteBits(pPES->P_STD_buffer_flag, 1);
		QDVB_WriteBits(0, 3);
		QDVB_WriteBits(pPES->PES_extension_flag_2, 1);

		if (pPES->PES_private_data_flag == 1)
		{
			for (int i=0; i<16; i++)
			{
				QDVB_WriteBits(pPES->private_data[i], 8);
			}
		}

		if (pPES->pack_header_field_flag == 1)
		{
			QDVB_WriteBits(pPES->pack_field_length, 8);
			for (int i=0; i<pPES->pack_field_length; i++)
			{
				QDVB_WriteBits(pPES->pack_field[i], 8);
			}
		}

		if (pPES->program_packet_sequence_counter_flag == 1)
		{
			QDVB_WriteBits(1, 1);
			QDVB_WriteBits(pPES->program_packet_sequence_counter, 7);
			QDVB_WriteBits(1, 1);
			QDVB_WriteBits(pPES->MPEG1_MPEG2_identifier, 1);
			QDVB_WriteBits(pPES->original_stuff_length, 6);
		}

		if (pPES->P_STD_buffer_flag == 1)
		{
			QDVB_WriteBits(1, 2);
			QDVB_WriteBits(pPES->P_STD_buffer_scale, 1);
			QDVB_WriteBits(pPES->P_STD_buffer_size, 13);
		}

		if (pPES->PES_extension_flag_2 == 1)
		{
			QDVB_WriteBits(1, 1);
			QDVB_WriteBits(pPES->PES_extension_field_length, 7);
			for (int i = 0; i < pPES->PES_extension_field_length; i++)
			{
				QDVB_WriteBits(pPES->PES_extension_field[i], 8);
			}
		}
	}

	nHdrLen = QDVB_CurrentBuffer() - pHdrPos - 1;
	*pHdrPos = nHdrLen;
	for (int i=0; i<nDataSize; i++)
	{
		QDVB_WriteBits(pData[i], 8);
	}

	uint16_t nOutLen = 0;
	nPESLen = QDVB_CurrentBuffer() - pBuffer - 6; // Not count starting 6 bytes
	if ((nPESLen < 0xFFFF) && (!bUnlim))
	{
		nOutLen = nPESLen;
	}

	pLenPos[0] = (nOutLen >> 8) & 0xff;
	pLenPos[1] = (nOutLen) & 0xff;

	nBufferSize = nPESLen + 6;

	return QDVB_BCError();
}

QDVBDescCA::QDVBDescCA()
{
	/* Preinit vars */
	descriptor_tag = 0;
    CA_system_ID = (uint16_t)-1;
    CA_PID = (uint16_t)-1;
}

QDVBDescCA::QDVBDescCA(QDVBDesc* pSrc)
{
	/* Preinit vars */
	descriptor_tag = 0;
    CA_system_ID = (uint16_t)-1;
    CA_PID = (uint16_t)-1;

    /* Parse source */
    if (pSrc->descriptor_tag != 9) return;
    descriptor_tag = pSrc->descriptor_tag;

	QDVB_Start(pSrc->data.data(), pSrc->data.size());
	QDVB_ReadBits(CA_system_ID, 16);
	QDVB_SkipBits(3);
	QDVB_ReadBits(CA_PID, 13);
    data.resize(QDVB_CurrentSize());
    memcpy(data.data(), QDVB_CurrentBuffer(), QDVB_CurrentSize());

QDVB_BoundsError;
	return;
}

QDVBDescCA::~QDVBDescCA()
{

}

QDVBDescCA* QDVBDescCA::Duplicate()
{
	QDVBDescCA* pDst = new QDVBDescCA();

	pDst->descriptor_tag = descriptor_tag;
	pDst->CA_system_ID = CA_system_ID;
    pDst->CA_PID = CA_PID;

	pDst->data.resize(data.size());
	memcpy(pDst->data.data(), data.data(), data.size());
	return pDst;
}

QDVBDesc::QDVBDesc()
{
	descriptor_tag = 0;
}

QDVBDesc::~QDVBDesc()
{
}

QDVBDesc* QDVBDesc::Duplicate()
{
	QDVBDesc* pRetVal = new QDVBDesc();
	pRetVal->descriptor_tag = descriptor_tag;

	/* Copy data */
	pRetVal->data.resize(data.size());
	if (pRetVal->data.size())
	{
		memcpy(pRetVal->data.data(), data.data(), pRetVal->data.size());
	}
	return pRetVal;
}

QDVBStrm::QDVBStrm()
{
	stream_type = 0;
	elementary_PID = 0;
}

QDVBStrm::~QDVBStrm()
{
	for (int i=0; i<(int)descs.size(); i++)
	{
		QDVBDesc* pDesc = descs[i];
		delete pDesc;
	}
}

QDVBStrm* QDVBStrm::Duplicate()
{
	QDVBStrm* pRetVal = new QDVBStrm();

	pRetVal->elementary_PID = elementary_PID;
	pRetVal->stream_type = stream_type;
	for (int i=0; i<(int)descs.size(); i++)
	{
		pRetVal->descs.push_back(descs[i]->Duplicate());
	}

	return pRetVal;
}

