#include "ByteStream.h"
#include "xsystem.h"

CByteStream::CByteStream()
	: p(NULL)
	, bytes(NULL)
	, nb_bytes(0) {
	set_value(NULL, 0);
}
CByteStream::CByteStream(uint8_t* b, int nb_b) {
	set_value(b, nb_b);
}
CByteStream::CByteStream(char* b, int nb_b) {
	set_value(reinterpret_cast<uint8_t*>(b), nb_b);
}
CByteStream::~CByteStream() {
}

void CByteStream::set_value(uint8_t* b, int nb_b) {
	p = bytes = b;
	nb_bytes = nb_b;
	assert(std::isLittleEndian());
}
bool CByteStream::initialize(uint8_t* b, int nb) {
	if (!b) return false;
	if (nb <= 0) return false;
	nb_bytes = nb;
	p = bytes = b;
	return true;
}
bool CByteStream::initialize(char* b, int nb) {
	return initialize(reinterpret_cast<uint8_t*>(b), nb);
}
bool CByteStream::empty() {
	return !bytes || (p >= bytes + nb_bytes);
}
bool CByteStream::require(int required_size) {
	assert(required_size >= 0);
	return required_size <= nb_bytes - (p - bytes);
}

void CByteStream::skip(int size) {
	assert(p);
	p += size;
}
int8_t CByteStream::read_1bytes() {
	assert(require(1));
	return (int8_t)*p++;
}
int16_t CByteStream::read_2bytes() {
	assert(require(2));
	int16_t value;
	char* pp = (char*)&value;
	pp[1] = *p++;
	pp[0] = *p++;
	return value;
}
int32_t CByteStream::read_3bytes() {
	assert(require(3));
	int32_t value = 0x00;
	char* pp = (char*)&value;
	pp[2] = *p++;
	pp[1] = *p++;
	pp[0] = *p++;
	return value;
}
int32_t CByteStream::read_4bytes() {
	assert(require(4));
	int32_t value;
	char* pp = (char*)&value;
	pp[3] = *p++;
	pp[2] = *p++;
	pp[1] = *p++;
	pp[0] = *p++;
	return value;
}
int64_t CByteStream::read_8bytes() {
	assert(require(8));
	int64_t value;
	char* pp = (char*)&value;
	pp[7] = *p++;
	pp[6] = *p++;
	pp[5] = *p++;
	pp[4] = *p++;
	pp[3] = *p++;
	pp[2] = *p++;
	pp[1] = *p++;
	pp[0] = *p++;
	return value;
}
std::string CByteStream::read_string(int len) {
	assert(require(len));
	std::string value;
	value.append(reinterpret_cast<char*>(p), len);
	p += len;
	return value;
}
void CByteStream::read_bytes(uint8_t* data, int size) {
	assert(require(size));
	memcpy(data, p, size);
	p += size;
}
void CByteStream::read_bytes(char* data, int size) {
	assert(require(size));
	memcpy(data, p, size);
	p += size;
}

void CByteStream::write_1bytes(int8_t value) {
	assert(require(1));
	*p++ = value;
}
void CByteStream::write_2bytes(int16_t value) {
	assert(require(2));
	char* pp = (char*)&value;
	*p++ = pp[1];
	*p++ = pp[0];
}
void CByteStream::write_4bytes(int32_t value) {
	assert(require(4));
	char* pp = (char*)&value;
	*p++ = pp[3];
	*p++ = pp[2];
	*p++ = pp[1];
	*p++ = pp[0];
}
void CByteStream::write_3bytes(int32_t value) {
	assert(require(3));
	char* pp = (char*)&value;
	*p++ = pp[2];
	*p++ = pp[1];
	*p++ = pp[0];
}
void CByteStream::write_8bytes(int64_t value) {
	assert(require(8));
	char* pp = (char*)&value;
	*p++ = pp[7];
	*p++ = pp[6];
	*p++ = pp[5];
	*p++ = pp[4];
	*p++ = pp[3];
	*p++ = pp[2];
	*p++ = pp[1];
	*p++ = pp[0];
}
void CByteStream::write_string(std::string value) {
	assert(require((int)value.length()));
	memcpy(p, value.data(), value.length());
	p += value.length();
}
void CByteStream::write_bytes(const void* data, int size) {
	assert(require(size));
	memcpy(p, data, size);
	p += size;
}
