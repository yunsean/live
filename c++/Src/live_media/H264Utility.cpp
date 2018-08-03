#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "SmartArr.h"
#include "H264Utility.h"
#include "h264.h"
#include "Writelog.h"

CH264Utility::CH264Utility(void)
	: m_bySps()
	, m_byPps()
	, m_byCache()
	, m_byNal()
	, m_spsinfo(NULL)
	, m_nLatestFrameNum(-1)
	, m_nLatestIsKey(false) {
}

CH264Utility::~CH264Utility(void) {
	delete m_spsinfo;
	m_spsinfo = NULL;
}

CH264Utility::H264NalType CH264Utility::GetNalType(const unsigned char* const data) {
	int						nal_type(data[4]);
	return (H264NalType)(nal_type & 0x1f);
}

int CH264Utility::NextNalStart(const unsigned char* const data, const int size, const int from /* = 0 */) {
	if (size < 4 || from < 0)return -1;
	if (from + 4 >= size)return -1;
	for (int start = from; start + 3 < size; start++) {
		if (data[start] == 0 && data[start + 1] == 0 && ((data[start + 2] == 1) || (data[start + 2] == 0 && data[start + 3] == 1))) {
			return start;
		}
	}
	return -1;
}

bool CH264Utility::SetExtraData(const unsigned char* const data, const int size, int* width, int* height) {
	static const unsigned char  header[] = { 0, 0, 0, 1 };
	int						    pos(0);
	unsigned char               versionIndication(data[pos++]);
	unsigned char               profileIndication(data[pos++]);
	unsigned char               profileCompatibility(data[pos++]);
	unsigned char               levelIndication(data[pos++]);
	int						    lengthSizeMinusOne((data[pos++] & 0x03) + 1);
	int						    numOfSequenceParameterSets(data[pos++] & 0x1f);
	bool					    hasSps(false);
	if (m_byCache == 0)m_byCache.EnsureSize(1024);
	m_byPps.Clear();
	m_bySps.Clear();
	for (int i = 0; i < numOfSequenceParameterSets; i++) {
		int					sequenceParameterSetLength((data[pos] << 8) | data[pos + 1]);
		pos += 2;
		if (!hasSps) {
			int				sps_size(sequenceParameterSetLength);
			h264_decode_annexb(m_byCache, &sps_size, data + pos + 1, sps_size - 1);
			if (m_spsinfo == NULL)
				m_spsinfo = new h264_sps_t();
			memset(m_spsinfo, 0, sizeof(h264_sps_t));
			if (h264_decode_seq_parameter_set(m_byCache, sps_size, m_spsinfo))
				hasSps = true;
		}
		wli("dylan: sps node[%d], %dbytes", i, sequenceParameterSetLength);
		m_bySps.AppendData(header, 4);
		m_bySps.AppendData(data + pos, sequenceParameterSetLength);
		pos += sequenceParameterSetLength;
	}
	if (!hasSps) {
		wle("dylan: Parse sps info failed.");
		return false;
	}
	wli("dylan: avc width: %d, height = %d", m_spsinfo->mb_width, m_spsinfo->mb_height);

	int						numOfPictureParameterSets(data[pos++]);
	for (int i = 0; i < numOfPictureParameterSets; i++) {
		int					pictureParameterSetLength((data[pos] << 8) | data[pos + 1]);
		pos += 2;
		m_byPps.AppendData(header, 4);
		m_byPps.AppendData(data + pos, pictureParameterSetLength);
		pos += pictureParameterSetLength;
		wli("dylan: pps node[%d], %dbytes", i, pictureParameterSetLength);
	}
	if (width)*width = m_spsinfo->mb_width;
	if (height)*height = m_spsinfo->mb_height;

	(void)versionIndication;
	(void)profileIndication;
	(void)profileCompatibility;
	(void)levelIndication;
	(void)lengthSizeMinusOne;
	return true;
}

const unsigned char* CH264Utility::NormalizeH264(const unsigned char* const data, const int size, int& len, bool& key) {
	if ((m_bySps.GetSize() < 1 || m_byPps.GetSize() < 1) && !PickExtraData(data, size)) return NULL;
	const unsigned char*    src(data);
	len = size;
	int						pos(NextNalStart(src, len, 0));
	key = true;
	bool					unk(false);
	while (pos >= 0) {
		int					nal_type(data[4] & 0x1f);
		if (nal_type == 0x07)return src;
		if (nal_type == 0x08)return src;
		if (nal_type == 0x05)break;
		if (nal_type < 0x05) {
			unk = true;
			break;
		}
		src += pos;
		len -= pos;
		pos = NextNalStart(src, len, 4);
	}
	if (unk)key = (GetSliceInfo(src, len) == islice);
	if (key) {
		m_byCache.FillData(m_bySps);
		m_byCache.AppendData(m_byPps);
		m_byCache.AppendData(src, len);
		len = m_byCache.GetSize();
		return (unsigned char*)m_byCache.GetData();
	}
	return src;
}

void CH264Utility::NormalizeH264(const unsigned char* const data, const int size, const std::function<void(const unsigned char* const data, const int size, bool key)>& callback, const bool autoAddConfig /*= true*/) {
#if 0
	if ((m_bySps.GetSize() < 1 || m_byPps.GetSize() < 1) && !PickExtraData(data, size))return;
	static unsigned char*	zero(0);
	auto					callCB = [callback, autoAddConfig, this](const unsigned char* const nalData, const int nalSize) {
		auto				headSize(nalData[2] == 0x01 ? 3 : 4);
		auto				nal((H264NalType)(nalData[headSize] & 0x1f));
		if (nal > 5) return;
		auto				type(GetSliceInfo(nalData, nalSize));
		if (type == islice) {
			m_byNal.Clear();
			if (autoAddConfig) {
				m_byNal.AppendData(m_bySps);
				m_byNal.AppendData(m_byPps);
			}
			if (headSize == 3) m_byNal.AppendData(&zero, 1);
			m_byNal.AppendData(nalData, nalSize);
			callback(m_byNal, m_byNal, true);
		} else if (headSize == 3) {
			m_byNal.FillData(&zero, 1);
			m_byNal.AppendData(nalData, nalSize);
			callback(m_byNal, m_byNal, false);
		} else {
			callback(nalData, nalSize, false);
		}
	};
	int						start(-3);
	while (true) {
		int					next(NextNalStart(data, size - start, start + 3));
		if (next == -1)break;
		if (start >= 0) callCB(data + start, next - start);
		start = next;
	}
	if (start < size - 3 && start >= 0) {
		callCB(data + start, size - start);
	}
#else 
	if ((m_bySps.GetSize() < 1 || m_byPps.GetSize() < 1) && !PickExtraData(data, size))return;
	static unsigned char*	zero(0);
	auto					callCB = [callback, autoAddConfig, this](const unsigned char* const nalData, const int nalSize) {
		auto				headSize(nalData[2] == 0x01 ? 3 : 4);
		auto				nal((H264NalType)(nalData[headSize] & 0x1f));
		if (nal > 5) return;
		int					framenum(0);
		auto				type(GetSliceInfo(nalData, nalSize, &framenum));
		if (framenum != m_nLatestFrameNum) {
			if (m_byNal.GetSize() > 0) callback(m_byNal, m_byNal, m_nLatestIsKey);
			m_byNal.Clear();
			m_nLatestIsKey = false;
			m_nLatestFrameNum = framenum;
		}
		if (type == islice && m_byNal.GetSize() < 1) {
			m_nLatestIsKey = true;
			if (autoAddConfig) {
				m_byNal.AppendData(m_bySps);
				m_byNal.AppendData(m_byPps);
			}
		}
		if (headSize == 3) m_byNal.AppendData(&zero, 1);
		m_byNal.AppendData(nalData, nalSize);
	};
	int						start(-3);
	while (true) {
		int					next(NextNalStart(data, size - start, start + 3));
		if (next == -1)break;
		if (start >= 0) callCB(data + start, next - start);
		start = next;
	}
	if (start < size - 3 && start >= 0) {
		callCB(data + start, size - start);
	}
#endif
}

bool CH264Utility::PickExtraData(const unsigned char* const data, const int size, int* width, int* height) {
	int						start(-1);
	int						from(0);
	H264NalType				nal(unknal);
	CSmartArr<unsigned char>spsNal;
	while (1) {
		int					next(NextNalStart(data, size - from, from));
		from = next + 4;
		if (next == -1)break;
		if (start == -1) {
			start = next;
			if (start + 3 < size)nal = (H264NalType)(data[start + 4] & 0x1f);
			else break;
		} else {
			if (sps == nal && !m_byCache)m_bySps.FillData(data + start, next - start);
			else if (pps == nal && !m_byPps)m_byPps.FillData(data + start, next - start);
			start = next;
			if (next + 3 < size)nal = (H264NalType)(data[next + 4] & 0x1f);
			else nal = unknal;
		}
	}
	if (start < size - 3) {
		int					next(size);
		if (sps == nal && !m_byCache)m_bySps.FillData(data + start, next - start);
		else if (pps == nal && !m_byPps)m_byPps.FillData(data + start, next - start);
	}
	if (m_bySps.GetSize() < 1 || m_byPps.GetSize() < 1)return false;
	if (m_byCache.GetData() == NULL)m_byCache.EnsureSize(64);

	int						sps_size(m_bySps.GetSize());
	h264_decode_annexb(m_byCache, &sps_size, (unsigned char*)m_bySps.GetData() + 5, m_bySps.GetSize() - 5);
	if (!m_spsinfo)m_spsinfo = new h264_sps_t();
	memset(m_spsinfo, 0, sizeof(h264_sps_t));
	if (!h264_decode_seq_parameter_set(m_byCache, sps_size, m_spsinfo))return false;
	if (width)*width = m_spsinfo->mb_width;
	if (height)*height = m_spsinfo->mb_height;

	return true;
}

const unsigned char* CH264Utility::GetSps(int& len) {
	if (m_bySps.GetSize() < 1) return NULL;
	len = m_bySps.GetSize();
	return m_bySps;
}
const unsigned char* CH264Utility::GetPps(int& len) {
	if (m_byPps.GetSize() < 1) return NULL;
	len = m_byPps.GetSize();
	return m_byPps;
}
bool CH264Utility::GetImageSize(int* width, int* height) {
	if (m_spsinfo == nullptr)return false;
	if (width)*width = m_spsinfo->mb_width;
	if (height)*height = m_spsinfo->mb_height;
}

CH264Utility::H264SliceType CH264Utility::GetSliceInfo(const unsigned char* const data, const int size, int* framenum /*= nullptr*/) {
	int						dstlen(0);
	h264_slice_t			slice;
	auto					headSize(data[2] == 0x01 ? 3 : 4);
	int						srclen((size - headSize - 1 < 60) ? size - headSize - 1 : 60);

	if (m_byCache.GetData() == NULL)m_byCache.EnsureSize(64);
	h264_decode_annexb(m_byCache, &dstlen, data + headSize + 1, srclen);
	h264_decode_slice(&slice, m_byCache, dstlen, data[headSize] & 0x1f, m_spsinfo);
	if (framenum != nullptr) *framenum = slice.i_frame_num;
	switch (slice.i_slice_type) {
	case 2:
	case 7:
	case 4:
	case 9:
		return islice;
	case 0:
	case 5:
	case 3:
	case 8:
		return pslice;
	case 1:
	case 6:
		return bslice;
	}
	return unkslice;
}
