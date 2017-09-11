#pragma once
#include "os.h"

class CByteStream {
public:
	CByteStream();
	CByteStream(uint8_t* b, int nb_b);
	CByteStream(char* b, int nb_b);
	~CByteStream();

public:
	//stream never free the bytes, user must free it.
	bool initialize(uint8_t* b, int nb);
	bool initialize(char* b, int nb);
	uint8_t* data() const { return p; }
	int size() const { return nb_bytes; }
	int pos() const { return (int)(p - bytes); }
	bool empty();
	//whether required size is ok.
	bool require(int required_size);

public:
	void skip(int size);
	int8_t read_1bytes();
	int16_t read_2bytes();
	int32_t read_3bytes();
	int32_t read_4bytes();
	int64_t read_8bytes();
	std::string read_string(int len);
	void read_bytes(uint8_t* data, int size);
	void read_bytes(char* data, int size);

	void write_1bytes(int8_t value);
	void write_2bytes(int16_t value);
	void write_4bytes(int32_t value);
	void write_3bytes(int32_t value);
	void write_8bytes(int64_t value);
	void write_string(std::string value);
	void write_bytes(uint8_t* data, int size);
	void write_bytes(char* data, int size);

private:
	void set_value(uint8_t* b, int nb_b);

private:
	// current position at bytes.
	uint8_t* p;
	// the bytes data for stream to read or write.
	uint8_t* bytes;
	// the total number of bytes.
	int nb_bytes;
};

