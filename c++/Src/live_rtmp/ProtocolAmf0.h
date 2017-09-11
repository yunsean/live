#pragma once
#include <string>
#include "ByteStream.h"
#include "os.h"

//////////////////////////////////////////////////////////////////////////
// AMF0 marker
#define RTMP_AMF0_Number			0x00
#define RTMP_AMF0_Boolean			0x01
#define RTMP_AMF0_String			0x02
#define RTMP_AMF0_Object			0x03
#define RTMP_AMF0_MovieClip			0x04 // reserved, not supported
#define RTMP_AMF0_Null				0x05
#define RTMP_AMF0_Undefined			0x06
#define RTMP_AMF0_Reference			0x07
#define RTMP_AMF0_EcmaArray			0x08
#define RTMP_AMF0_ObjectEnd			0x09
#define RTMP_AMF0_StrictArray		0x0A
#define RTMP_AMF0_Date				0x0B
#define RTMP_AMF0_LongString		0x0C
#define RTMP_AMF0_UnSupported		0x0D
#define RTMP_AMF0_RecordSet			0x0E // reserved, not supported
#define RTMP_AMF0_XmlDocument		0x0F
#define RTMP_AMF0_TypedObject		0x10
// AVM+ object is the AMF3 object.
#define RTMP_AMF0_AVMplusObject		0x11
// origin array whos data takes the same form as LengthValueBytes
#define RTMP_AMF0_OriginStrictArray	0x20
// User defined
#define RTMP_AMF0_Invalid			0x3F

namespace amf0 {
	class Amf0Any;
	class Amf0Object;
	class Amf0EcmaArray;
	class Amf0StrictArray;
	class Amf0ObjectEOF;
	class UnSortedHashtable;

	bool amf0_read_string(CByteStream* stream, std::string& value);
	bool amf0_write_string(CByteStream* stream, std::string value);
	bool amf0_read_boolean(CByteStream* stream, bool& value);
	bool amf0_write_boolean(CByteStream* stream, bool value);
	bool amf0_read_number(CByteStream* stream, double& value);
	bool amf0_write_number(CByteStream* stream, double value);
	bool amf0_read_null(CByteStream* stream);
	bool amf0_write_null(CByteStream* stream);
	bool amf0_read_undefined(CByteStream* stream);
	bool amf0_write_undefined(CByteStream* stream);
	bool amf0_read_any(CByteStream* stream, Amf0Any** ppvalue);
	bool amf0_write_any(CByteStream* stream, Amf0Any* value);

	bool amf0_read_utf8(CByteStream* stream, std::string& value);
	bool amf0_write_utf8(CByteStream* stream, std::string value);
	bool amf0_is_object_eof(CByteStream* stream);
	bool amf0_write_object_eof(CByteStream* stream, Amf0ObjectEOF* value);

	bool amf0_read(CByteStream* stream, std::string& value);
	bool amf0_read(CByteStream* stream, bool& value);
	bool amf0_read(CByteStream* stream, double& value);
	bool amf0_read(CByteStream* stream, Amf0Any*& ppvalue);

	bool amf0_write(CByteStream* stream, std::string value);
	bool amf0_write(CByteStream* stream, bool value);
	bool amf0_write(CByteStream* stream, double value);
	bool amf0_write(CByteStream* stream, Amf0Any* value);

	class Amf0Any {
	public:
		Amf0Any();
		virtual ~Amf0Any();
	public:
		virtual char marker() const { return m_marker; }
		virtual bool isString();
		virtual bool isBoolean();
		virtual bool isNumber();
		virtual bool isNull();
		virtual bool isUndefined();
		virtual bool isObject();
		virtual bool isObjectEof();
		virtual bool isEcmaArray();
		virtual bool isStrictArray();
		virtual bool isDate();
		virtual bool isComplexObject();

		virtual std::string toString();
		virtual const char* toStringRaw();
		virtual bool toBoolean();
		virtual double toNumber();
		virtual int64_t toDate();
		virtual int16_t toDateTimeZone();
		virtual Amf0Object* toObject();
		virtual Amf0EcmaArray* toEcmaArray();
		virtual Amf0StrictArray* toStrictArray();

		virtual void setNumber(double value);

		virtual int totalSize() = 0;
		virtual bool read(CByteStream* stream) = 0;
		virtual bool write(CByteStream* stream) = 0;
		virtual Amf0Any* copy() = 0;
		virtual char* humanPrint(char** pdata, int* psize);

		static Amf0Any* string(const char* value = NULL);
		static Amf0Any* boolean(bool value = false);
		static Amf0Any* number(double value = 0.0);
		static Amf0Any* date(int64_t value = 0);
		static Amf0Any* null();
		static Amf0Any* undefined();
		static Amf0Object* object();
		static Amf0Any* objectEof();
		static Amf0EcmaArray* ecmaArray();
		static Amf0StrictArray* strictArray();

		static bool discovery(CByteStream* stream, Amf0Any** ppvalue);

	protected:
		char m_marker;
	};

	class Amf0String : public Amf0Any {
	public:
		virtual ~Amf0String();
	public:
		virtual int totalSize();
		virtual bool read(CByteStream* stream);
		virtual bool write(CByteStream* stream);
		virtual Amf0Any* copy();
	private:
		friend class Amf0Any;
		Amf0String(const char* value);
	private:
		std::string m_value;
	};

	class Amf0Boolean : public Amf0Any {
	public:
		virtual ~Amf0Boolean();
	public:
		virtual int totalSize();
		virtual bool read(CByteStream* stream);
		virtual bool write(CByteStream* stream);
		virtual Amf0Any* copy();
	private:
		friend class Amf0Any;
		Amf0Boolean(bool value);
	private:
		bool m_value;
	};

	class Amf0Number : public Amf0Any {
	public:
		virtual ~Amf0Number();
	public:
		virtual int totalSize();
		virtual bool read(CByteStream* stream);
		virtual bool write(CByteStream* stream);
		virtual Amf0Any* copy();
	private:
		friend class Amf0Any;
		Amf0Number(double value);
	private:
		double m_value;
	};

	class Amf0Date : public Amf0Any {
	public:
		virtual ~Amf0Date();
	public:
		virtual int64_t date() const { return m_date; }
		virtual int16_t time_zone() const { return m_timezone; }
	public:
		virtual int totalSize();
		virtual bool read(CByteStream* stream);
		virtual bool write(CByteStream* stream);
		virtual Amf0Any* copy();
	private:
		friend class Amf0Any;
		Amf0Date(int64_t value);
	private:
		int64_t m_date;
		int16_t m_timezone;
	};

	class Amf0Null : public Amf0Any {
	public:
		virtual ~Amf0Null();
	public:
		virtual int totalSize();
		virtual bool read(CByteStream* stream);
		virtual bool write(CByteStream* stream);
		virtual Amf0Any* copy();
	private:
		friend class Amf0Any;
		Amf0Null();
	};

	class Amf0Undefined : public Amf0Any {
	public:
		virtual ~Amf0Undefined();
	public:
		virtual int totalSize();
		virtual bool read(CByteStream* stream);
		virtual bool write(CByteStream* stream);
		virtual Amf0Any* copy();
	private:
		friend class Amf0Any;
		Amf0Undefined();
	};

	class Amf0Object : public Amf0Any {
	public:
		virtual ~Amf0Object();

	public:
		virtual int totalSize();
		virtual bool read(CByteStream* stream);
		virtual bool write(CByteStream* stream);
		virtual Amf0Any* copy();

		virtual void clear();
		virtual int count();
		virtual std::string keyAt(int index);
		virtual const char* keyRawAt(int index);
		virtual Amf0Any* valueAt(int index);

		virtual void set(std::string key, Amf0Any* value);
		virtual Amf0Any* getProperty(std::string name);
		virtual Amf0Any* ensurePropertyString(std::string name);
		virtual Amf0Any* ensurePropertyNumber(std::string name);
		virtual void remove(std::string name);

	private:
		friend class Amf0Any;
		Amf0Object();
	private:
		UnSortedHashtable* properties;
		Amf0ObjectEOF* eof;
	};

	class Amf0EcmaArray : public Amf0Any {
	public:
		virtual ~Amf0EcmaArray();
	public:
		virtual int totalSize();
		virtual bool read(CByteStream* stream);
		virtual bool write(CByteStream* stream);
		virtual Amf0Any* copy();

		virtual void clear();
		virtual int count();
		virtual std::string keyAt(int index);
		virtual const char* keyRawAt(int index);
		virtual Amf0Any* valueAt(int index);

		virtual void set(std::string key, Amf0Any* value);
		virtual Amf0Any* getProperty(std::string name);
		virtual Amf0Any* ensurePropertyString(std::string name);
		virtual Amf0Any* ensurePropertyNumber(std::string name);

	private:
		friend class Amf0Any;
		Amf0EcmaArray();
	private:
		UnSortedHashtable* properties;
		Amf0ObjectEOF* eof;
		int32_t _count;
	};

	class Amf0StrictArray : public Amf0Any {
	public:
		virtual ~Amf0StrictArray();
	public:
		virtual int totalSize();
		virtual bool read(CByteStream* stream);
		virtual bool write(CByteStream* stream);
		virtual Amf0Any* copy();

		virtual void clear();
		virtual int count();
		virtual Amf0Any* at(int index);

		virtual void append(Amf0Any* any);

	private:
		friend class Amf0Any;
		Amf0StrictArray();
	private:
		std::vector<Amf0Any*> properties;
		int32_t _count;
	};

	class Amf0Size {
	public:
		static int utf8(std::string value);
		static int string(std::string value);
		static int number();
		static int date();
		static int null();
		static int undefined();
		static int boolean();
		static int object(Amf0Object* obj);
		static int objectEof();
		static int ecmaArray(Amf0EcmaArray* arr);
		static int strictArray(Amf0StrictArray* arr);
		static int any(Amf0Any* o);

		static int size(std::string value) { return string(value); }
		static int size(double value) { return number(); }
		static int size(bool value) { return boolean(); }
		static int size(Amf0Any* value) { return any(value); }
	};
	
	class UnSortedHashtable {
	public:
		UnSortedHashtable();
		virtual ~UnSortedHashtable();
	public:
		virtual int count();
		virtual void clear();
		virtual std::string keyAt(int index);
		virtual const char* keyRawAt(int index);
		virtual Amf0Any* valueAt(int index);
		virtual void set(std::string key, Amf0Any* value);

		virtual Amf0Any* getProperty(std::string name);
		virtual Amf0Any* ensurePropertyString(std::string name);
		virtual Amf0Any* ensurePropertyNumber(std::string name);
		virtual void remove(std::string name);

		virtual void copy(UnSortedHashtable* src);

	private:
		typedef std::pair<std::string, Amf0Any*> Amf0ObjectPropertyType;
		std::vector<Amf0ObjectPropertyType> properties;
	};

	class Amf0ObjectEOF : public Amf0Any {
	public:
		Amf0ObjectEOF();
		virtual ~Amf0ObjectEOF();

	public:
		virtual int totalSize();
		virtual bool read(CByteStream* stream);
		virtual bool write(CByteStream* stream);
		virtual Amf0Any* copy();
	};
};

