#include <algorithm>
#include "AVCConfig.h"
#include "BitReader.h"
#include "BitWriter.h"

CAVCConfig::CAVCConfig(void)
	: configurationVersion(0x01)
	, profileIndication(0x42)
	, profile_compatibility(0xc0)
	, levelIndication(0x33)
	, lengthSizeMinusOne(3)

	, m_vsnSPS(new std::vector<CSmartNal<unsigned char>>)
	, m_vsnPPS(new std::vector<CSmartNal<unsigned char>>) {
}
CAVCConfig::~CAVCConfig(void) {
    delete m_vsnSPS;
    delete m_vsnPPS;
}

bool CAVCConfig::Serialize(CSmartNal<unsigned char>& snConfig) {
	int						nSize(CalcBufSize());
	unsigned char*          lpData(snConfig.Allocate(nSize));
	if (lpData == nullptr)return false;

	CBitWriter				bw;
	bw.Initialize(snConfig.GetArr(), snConfig.GetSize());
	bw.WriteBYTE(configurationVersion);
	bw.WriteBYTE(profileIndication);
	bw.WriteBYTE(profile_compatibility);
	bw.WriteBYTE(levelIndication);

	unsigned char					bt(lengthSizeMinusOne);
	bt						|= 0xFC;
	bw.WriteBYTE(bt);

	bt						= SpsCount();
	bt						|= 0xE0;
	bw.WriteBYTE(bt);	
	std::for_each(m_vsnSPS->begin(), m_vsnSPS->end(),
		[&bw](CSmartNal<unsigned char> sps){
			bw.WriteWORD(sps.GetSize(), true);
			bw.WriteData(sps.GetArr(), sps.GetSize());
	});

	bt						= PpsCount();
	bw.WriteBYTE(bt);
	std::for_each(m_vsnPPS->begin(), m_vsnPPS->end(),
		[&bw](CSmartNal<unsigned char> pps){
			bw.WriteWORD(pps.GetSize(), true);
			bw.WriteData(pps.GetArr(), pps.GetSize());
	});

	return true;
}

void CAVCConfig::Unserialize(const unsigned char* const config, const int size) {
	CBitReader				bp;
	bp.Initialize(config, size);
	configurationVersion	= bp.GetBits(8);
	profileIndication		= bp.GetBits(8);
	profile_compatibility	= bp.GetBits(8);
	levelIndication			= bp.GetBits(8);
	bp.GetBits(6);
	lengthSizeMinusOne		= bp.GetBits(2);

	static const uint8_t	NalHeader[] = { 0x00, 0x00, 0x00, 0x01 };
	bp.GetBits(3);
	int						numOfSPS(bp.GetBits(5));
	for (int i =0; i < numOfSPS; i++) {
		int					size(bp.GetBits(16));
		int					offset(bp.GetBitsOffset());
		CSmartNal<unsigned char>		sps(size + 4);
		memcpy(sps.GetArr(), NalHeader, 4);
		memcpy(sps.GetArr() + 4, config + offset / 8, size);
		bp.SkipBits(size * 8);
		m_vsnSPS->push_back(sps);
	};

	int						numOfPPS = bp.GetBits(8);
	for (int i =0; i < numOfPPS; i++) {
		int					size(bp.GetBits(16));
		int					offset(bp.GetBitsOffset());
		CSmartNal<unsigned char>		pps(size + 4);
		memcpy(pps.GetArr(), NalHeader, 4);
		memcpy(pps.GetArr() + 4, config + offset / 8, size);
		bp.SkipBits(size * 8);
		m_vsnPPS->push_back(pps);
	};
}

void CAVCConfig::AddSps(const unsigned char* const sps, const int size) {
	const unsigned char*	_sps(sps);
	int						_size(size);
	if (_sps[0] == 0x00 && _sps[1] == 0x00 && _sps[2] == 0x00 && _sps[3] == 0x01) {
		_sps				+= 4;
		_size				-= 4;
	} else if (_sps[0] == 0x00 && _sps[1] == 0x00 && _sps[2] == 0x01) {
		_sps				+= 3;
		_size				-= 3;
	}		
	auto					result(std::find_if(m_vsnSPS->begin(), m_vsnSPS->end(),
		[_sps, _size](CSmartNal<unsigned char> spsi) -> bool{
			if (spsi.GetSize() == _size && memcmp(spsi.GetArr(), _sps, _size) == 0)return true;
			else return false;
	}));
	if (result == m_vsnSPS->end()) {
		CSmartNal<unsigned char>		spsi(_size);
		memcpy(spsi.GetArr(), _sps, _size);
		m_vsnSPS->push_back(spsi);
	}
}
void CAVCConfig::AddPps(const unsigned char* pps, const int size) {
	const unsigned char*	_pps(pps);
	int						_size(size);
	if (_pps[0] == 0x00 && _pps[1] == 0x00 && _pps[2] == 0x00 && _pps[3] == 0x01) {
		_pps += 4;
		_size -= 4;
	}
	else if (_pps[0] == 0x00 && _pps[1] == 0x00 && _pps[2] == 0x01) {
		_pps += 3;
		_size -= 3;
	}
	auto					result(std::find_if(m_vsnPPS->begin(), m_vsnPPS->end(),
		[_pps, _size](CSmartNal<unsigned char> ppsi) -> bool{
			if (ppsi.GetSize() == _size && memcmp(ppsi.GetArr(), _pps, _size) == 0)return true;
			else return false;
	}));
	if (result == m_vsnPPS->end()) {
		CSmartNal<unsigned char>		ppsi(_size);
		memcpy(ppsi.GetArr(), _pps, _size);
		m_vsnPPS->push_back(ppsi);
	}
}

int CAVCConfig::SpsCount() {
	return (int)m_vsnSPS->size();
}
int CAVCConfig::PpsCount() {
	return (int)m_vsnPPS->size();
}

bool CAVCConfig::GetSps(const int index, CSmartNal<unsigned char>& sps) {
	if (index < 0 || index >= (int)m_vsnSPS->size())return false;
	sps					= (*m_vsnSPS)[index];
	return true;
} 
bool CAVCConfig::GetPps(const int index, CSmartNal<unsigned char>& pps) {
	if (index < 0 || index >= (int)m_vsnPPS->size())return nullptr;
	pps					= (*m_vsnPPS)[index];
	return true;
}

int CAVCConfig::CalcBufSize() {
	int					size(0);
	std::for_each(m_vsnSPS->begin(), m_vsnSPS->end(), 
		[&size](CSmartNal<unsigned char> sps){
			size		+= sps.GetSize() + 2;
	});
	std::for_each(m_vsnPPS->begin(), m_vsnPPS->end(), 
		[&size](CSmartNal<unsigned char> pps){
			size		+= pps.GetSize() + 2;
	});
	size				+= 7;
	return size;
}
