#ifndef _Q_DVBTS_H_
#define _Q_DVBTS_H_

#include "qstdinc.h"
#include "qstdvectors.h"
#include "qbuffer.h"

/* Classic TS packet */
struct QDVBTSPacket
{
	// Generic fields
	uint8_t		sync_byte;
	uint8_t		transport_error_indicator;
	uint8_t		payload_unit_start_indicator;
	uint8_t		transport_priority;
	uint16_t	PID;
	uint8_t		transport_scrambling_control;
	uint8_t		adaptation_field_control;
	uint8_t		continuity_counter;

	// Adaptation field
	uint8_t		adaptation_field_length;
	uint8_t		discontinuity_indicator;
	uint8_t		random_access_indicator;
	uint8_t		elementary_stream_priority_indicator;
	uint8_t		PCR_flag;
	uint8_t		OPCR_flag;
	uint8_t		splicing_point_flag;
	uint8_t		transport_private_data_flag;
	uint8_t		adaptation_field_extension_flag;

	// Program clock
	uint64_t	program_clock_reference_base;
	uint16_t	program_clock_reference_extension;

	// Original clock
	uint64_t	original_program_clock_reference_base;
	uint16_t	original_program_clock_reference_extension;

	// Splice related
	uint8_t		splice_countdown;

	uint8_t		transport_private_data_length;
	uint8_t*	private_data_byte;

	uint8_t		adaptation_field_extension_length;
	uint8_t		ltw_flag;
	uint8_t		piecewise_rate_flag;
	uint8_t		seamless_splice_flag;

	uint8_t		ltw_valid_flag;
	uint16_t	ltw_offset;

	uint32_t	piecewise_rate;

	uint8_t		splice_type;
	uint32_t	DTS_next_AU;

	uint8_t*	data;
	uint8_t		data_size;
};

/* Classic PES header */
struct QDVBPESHdr
{
	uint32_t	packet_start_code_prefix;
	uint8_t		stream_id;
	uint16_t	PES_packet_length;

		uint8_t	PES_scrambling_control;			// 2 bslbf
		uint8_t	PES_priority;					// 1 bslbf
		uint8_t data_alignment_indicator;		// 1 bslbf
		uint8_t	copyright;						// 1 bslbf
		uint8_t	original_or_copy;				// 1 bslbf
		uint8_t	PTS_DTS_flags;					// 2 bslbf
		uint8_t	ESCR_flag;						// 1 bslbf
		uint8_t	ES_rate_flag;					// 1 bslbf
		uint8_t	DSM_trick_mode_flag;			// 1 bslbf
		uint8_t	additional_copy_info_flag;		// 1 bslbf
		uint8_t	PES_CRC_flag;					// 1 bslbf
		uint8_t	PES_extension_flag;				// 1 bslbf
		uint8_t	PES_header_data_length;			// 8 uimsbf

			uint64_t	PTS;
			uint64_t	DTS;

			uint64_t	ESCR_base;
			uint16_t	ESCR_ext;
			uint32_t	ES_rate;

			uint8_t	trick_mode_control;			// 3 uimsbf

				uint8_t	field_id;				// 2 bslbf
				uint8_t	intra_slice_refresh;	// 1 bslbf
				uint8_t	frequency_truncation;	// 2 bslbf
				uint8_t	rep_cntrl;				// 5 uimsbf

			uint8_t		additional_copy_info;	// 7 bslbf
			uint16_t	previous_PES_packet_CRC;// 16 bslbf

		uint8_t PES_private_data_flag;			// 1 bslbf
		uint8_t pack_header_field_flag;			// 1 bslbf
		uint8_t program_packet_sequence_counter_flag;	//1 bslbf
		uint8_t P_STD_buffer_flag;				// 1 bslbf
		uint8_t PES_extension_flag_2;			// 1 bslbf

		uint8_t private_data[16];				// 128 uimsbf
		uint8_t pack_field_length;				// 8 uimsbf
		uint8_t pack_field[256];				// 256 bytes

		uint8_t program_packet_sequence_counter;// 7 uimsbf
		uint8_t MPEG1_MPEG2_identifier;			// 1 bslbf
		uint8_t original_stuff_length;			// 6 uimsbf
		uint8_t P_STD_buffer_scale;				// 1 bslbf
		uint16_t P_STD_buffer_size;				// 13 uimsbf
		uint8_t PES_extension_field_length;		// 7 uimsbf
		uint8_t PES_extension_field[128];		// 128 bytes

	uint8_t*	data;
	uint32_t	data_size;
};

/* Classic PAT packet */
struct QDVBPATProg
{
	QDVBPATProg()
	{
		program_number = 0;
		network_PID = 0;
		program_map_PID = 0;
	}

	uint16_t	program_number;
	uint16_t	network_PID;
	uint16_t	program_map_PID;
};
typedef std::vector<QDVBPATProg> QDVBPATProgs;

/* Classic PAT packet */
class QDVBPAT
{
public:
	QDVBPAT();
	virtual ~QDVBPAT();

	/* Prepare for parsing transport_stream_id, 0 - first available, -1 - just gather info */
	/* Parse current or next PAT */
	virtual void Prepare(int nID = 0, bool bCurrent = true);
	virtual void RePrepare();

	/* Add data */
	virtual bool AddData(bool, uint8_t* pData, int nSize);

	/* Data access */
	virtual bool Started();					// At least one segment arrived and parsed
	virtual bool Ready();					// PAT parsing is done
	virtual bool HasCurrent();				// Is there packets with current_next_indicator = 1;
	virtual bool HasAnnounces();			// Is there packets with current_next_indicator = 0;

	virtual int GetVersion();				// Return current PAT version
	virtual int GetID();					// Return current PAT version
	virtual void GetProgs(QDVBPATProgs&);	// Retrieve programs

protected:
	/* Locally used functions */
	virtual void PrepareChecks();			// Prepare check section
	virtual void AddCheck(int, int);		// Add/setup section
	virtual bool SecsReady();				// All sections ready
	virtual void CheckAddTran(uint16_t);	// Add transport
	virtual bool ParseData(uint8_t*, int);	// Parse section

	int				m_nID;					// Transport ID to parse
	bool			m_bDoCurrent;			// Do currents or announces
	int				m_nCurVer;				// Current version

	bool			m_bCurrents;			// current_next_indicator N1
	bool			m_bAnnounces;			// current_next_indicator N2

	uint64_t		m_donesec[4];			// 256 sections is max, bit field
	uint64_t		m_checksec[4];			// Prepared bit field

	QDVBPATProgs	m_programs;				// Parsed programs
	QWordArray		m_transports;
	QBuffer			m_section;
};

/* Generic description packet */
class QDVBDesc
{
public:
	QDVBDesc();
	virtual ~QDVBDesc();

	/* Create desc duplicate */
	QDVBDesc* Duplicate();

	uint8_t		descriptor_tag;
	QByteArray	data;
};
typedef std::vector<QDVBDesc*> QDVBDescs;

/* Conditional access descriptor */
class QDVBDescCA
{
public:
	QDVBDescCA();
	QDVBDescCA(QDVBDesc*);
	virtual ~QDVBDescCA();

	/* Create desc duplicate */
	QDVBDescCA* Duplicate();

	uint8_t		descriptor_tag;
    uint16_t    CA_system_ID;
    uint16_t    CA_PID;

	QByteArray	data;
};

/* Stream description packet */
class QDVBStrm
{
public:
	QDVBStrm();
	virtual ~QDVBStrm();
	
	uint8_t		stream_type;
	uint16_t	elementary_PID;

	/* Create stream duplicate */
	QDVBStrm* Duplicate();

	QDVBDescs	descs;
};
typedef std::vector<QDVBStrm*> QDVBStrms;

/* Classic PMT packet */
class QDVBPMT
{
public:
	QDVBPMT();
	virtual ~QDVBPMT();

	/* Prepare for parsing PMT */
	/* Parse current or next PAT */
	virtual void Prepare(int nSID, bool bCurrent);
	virtual void RePrepare();

	/* Add data */
	virtual bool AddData(bool, uint8_t* pData, int nSize);

	/* Data access */
	virtual bool Ready();					// PMT parsing is done
	virtual bool HasCurrent();				// Is there packets with current_next_indicator = 1;
	virtual bool HasAnnounces();			// Is there packets with current_next_indicator = 0;

	virtual int GetVersion();				// Return current PMT version
	virtual int GetSID();					// Return SID
	virtual int GetPCR();					// Return PCR PID
	virtual void GetDescs(QDVBDescs&);		// Get parsed descs
	virtual void GetStreams(QDVBStrms&);	// Get parsed streams

protected:

	void PrepareChecks();
	void AddCheck(int nCurSec, int nLastSec);
	bool SecsReady();
	virtual bool ParseData(uint8_t*, int);

	int				m_nSID;					// Program ID (SID) to parse
	int				m_nPCR_PID;

	bool			m_bDoCurrent;			// Do currents or announces
	int				m_nCurVer;				// Current version

	bool			m_bCurrents;			// current_next_indicator N1
	bool			m_bAnnounces;			// current_next_indicator N2

	uint64_t		m_donesec[4];			// 256 sections is max, bit field
	uint64_t		m_checksec[4];			// Prepared bit field

	QDVBDescs		m_descs;
	QDVBStrms		m_streams;

	QBuffer			m_section;
};

/* Main parser functions */
/* TS */
bool QDVBParseTS(uint8_t*, QDVBTSPacket*);
bool QDVBMakeTS(uint8_t*, QDVBTSPacket*);

bool QDVBPatchPCR(uint8_t*, QDVBTSPacket*);
void QDVBPatchCC(uint8_t* pData, int);

/* PES */
bool QDVBParsePES(uint8_t*, int, QDVBPESHdr*, bool bHdrOnly = false);
bool QDVBCreatePES(uint8_t*, long&, uint8_t*, long, QDVBPESHdr*, bool bUnlim = false);

/* CRC related functions */
bool QDVBValidCRC(uint8_t* pData, uint32_t nSize);
uint32_t QDVBCalcCRC(uint8_t* pStart, uint8_t* pEnd);

/* Some inlines */
inline void QDVBFastSetPID(uint16_t nPID, uint8_t* pData)
{
	pData[2] = nPID & 0x00FF;
	pData[1] = (pData[1] & 0xE0) | ((nPID >> 8) & 0x1F);
}

#endif
