#include "xutils.h"
#include "os.h"

namespace utils {
	bool isBytesEquals(void* pa, void* pb, int size) {
		uint8_t* a = (uint8_t*)pa;
		uint8_t* b = (uint8_t*)pb;
		if (!a && !b) {
			return true;
		}
		if (!a || !b) {
			return false;
		}
		for (int i = 0; i < size; i++) {
			if (a[i] != b[i]) {
				return false;
			}
		}
		return true;
	}
	void randomGenerate(char* bytes, int size) {
		static bool initialized = false;
		if (!initialized) {
			srand(0);
			initialized = true;
		}
		for (int i = 0; i < size; i++) {
			bytes[i] = 0x0f + (rand() % (256 - 0x0f - 0x0f));
		}
	}

	unsigned short uintFrom2BytesBE(unsigned char* data) {
		unsigned int value(0);
		value
			= (((int)data[1] << 8) & 0x0000ff00)
			+ (((int)data[2] << 0) & 0x000000ff);
		return value;
	}
	unsigned int uintFrom3BytesBE(unsigned char* data) {
		unsigned int value(0);
		value
			= (((int)data[0] << 16) & 0x00ff0000)
			+ (((int)data[1] << 8) & 0x0000ff00)
			+ (((int)data[2] << 0) & 0x000000ff);
		return value;
	}
	unsigned int uintFrom4BytesBE(unsigned char* data) {
		unsigned int value(0);
		value
			= (((int)data[0] << 24) & 0xff000000)
			+ (((int)data[1] << 16) & 0x00ff0000)
			+ (((int)data[2] << 8) & 0x0000ff00)
			+ (((int)data[3] << 0) & 0x000000ff);
		return value;
	}

	unsigned char* uintTo2BytesBE(unsigned short value, unsigned char* data) {
		*data++ = (value >> 8) & 0xff;
		*data++ = (value >> 0) & 0xff;
		return data;
	}
	unsigned char* uintTo3BytesBE(unsigned int value, unsigned char* data) {
		*data++ = (value >> 16) & 0xff;
		*data++ = (value >> 8) & 0xff;
		*data++ = (value >> 0) & 0xff;
		return data;
	}
	unsigned char* uintTo4BytesBE(unsigned int value, unsigned char* data) {
		*data++ = (value >> 24) & 0xff;
		*data++ = (value >> 16) & 0xff;
		*data++ = (value >> 8) & 0xff;
		*data++ = (value >> 0) & 0xff;
		return data;
	}

	unsigned short uintFrom2BytesLE(unsigned char* data) {
		unsigned int value(0);
		value
			= (((int)data[0] << 0) & 0x000000ff)
			+ (((int)data[1] << 8) & 0x0000ff00);
		return value;
	}
	unsigned int uintFrom3BytesLE(unsigned char* data) {
		unsigned int value(0);
		value
			= (((int)data[0] << 0) & 0x000000ff)
			+ (((int)data[1] << 8) & 0x0000ff00)
			+ (((int)data[2] << 16) & 0x00ff0000);
		return value;
	}
	unsigned int uintFrom4BytesLE(unsigned char* data) {
		unsigned int value(0);
		value
			= (((int)data[0] << 0) & 0x000000ff)
			+ (((int)data[1] << 8) & 0x0000ff00)
			+ (((int)data[2] << 16) & 0x00ff0000)
			+ (((int)data[3] << 24) & 0xff000000);
		return value;
	}

	unsigned char* uintTo2BytesLE(unsigned short value, unsigned char* data) {
		*data++ = (value >> 0) & 0xff;
		*data++ = (value >> 8) & 0xff;
		return data;
	}
	unsigned char* uintTo3BytesLE(unsigned int value, unsigned char* data) {
		*data++ = (value >> 0) & 0xff;
		*data++ = (value >> 8) & 0xff;
		*data++ = (value >> 16) & 0xff;
		return data;
	}
	unsigned char* uintTo4BytesLE(unsigned int value, unsigned char* data) {
		*data++ = (value >> 0) & 0xff;
		*data++ = (value >> 8) & 0xff;
		*data++ = (value >> 16) & 0xff;
		*data++ = (value >> 24) & 0xff;
		return data;
	}

	std::vector<std::string> split(const std::string& s, char seperator) {
		std::vector<std::string> output;
		std::string::size_type prev_pos = 0;
		std::string::size_type pos = 0;
		while ((pos = s.find(seperator, pos)) != std::string::npos) {
			std::string substring(s.substr(prev_pos, pos - prev_pos));
			output.push_back(substring);
			prev_pos = ++pos;
		}
		output.push_back(s.substr(prev_pos, pos - prev_pos)); 
		return output;
	}
};