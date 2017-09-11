#include "ProtocolAmf0.h"
#include "Writelog.h"
#include "xstring.h"
#include <sstream>

namespace amf0 {
	bool amf0_read_string(CByteStream* stream, std::string& value) {
		if (!stream->require(1)) {
			return wlet(false, _T("amf0 read string marker failed."));
		}
		char marker = stream->read_1bytes();
		if (marker != RTMP_AMF0_String) {
			return wlet(false, _T("amf0 check string marker failed.  marker=%#x, required=%#x"), marker, RTMP_AMF0_String);
		}
		return amf0_read_utf8(stream, value);
	}
	bool amf0_write_string(CByteStream* stream, std::string value) {
		if (!stream->require(1)) {
			return wlet(false, _T("amf0 write string marker failed."));
		}
		stream->write_1bytes(RTMP_AMF0_String);
		return amf0_write_utf8(stream, value);
	}
	bool amf0_read_boolean(CByteStream* stream, bool& value) {
		if (!stream->require(1)) {
			return wlet(false, _T("amf0 read bool marker failed."));
		}
		char marker = stream->read_1bytes();
		if (marker != RTMP_AMF0_Boolean) {
			return wlet(false, _T("amf0 check bool marker failed. marker=%#x, required=%#x"), marker, RTMP_AMF0_Boolean);
		}
		if (!stream->require(1)) {
			return wlet(false, _T("amf0 read bool value failed."));
		}
		value = (stream->read_1bytes() != 0);
		return true;
	}
	bool amf0_write_boolean(CByteStream* stream, bool value) {
		if (!stream->require(1)) {
			return wlet(false, _T("amf0 write bool marker failed."));
		}
		stream->write_1bytes(RTMP_AMF0_Boolean);
		if (!stream->require(1)) {
			return wlet(false, _T("amf0 write bool value failed."));
		}
		if (value) {
			stream->write_1bytes(0x01);
		} else {
			stream->write_1bytes(0x00);
		}
		return true;
	}
	bool amf0_read_number(CByteStream* stream, double& value) {
		if (!stream->require(1)) {
			return wlet(false, _T("amf0 read number marker failed."));
		}
		char marker = stream->read_1bytes();
		if (marker != RTMP_AMF0_Number) {
			return wlet(false, _T("amf0 check number marker failed. marker=%#x, required=%#x"), marker, RTMP_AMF0_Number);
		}
		if (!stream->require(8)) {
			return wlet(false, _T("amf0 read number value failed."));
		}
		int64_t temp = stream->read_8bytes();
		memcpy(&value, &temp, 8);
		return true;
	}
	bool amf0_write_number(CByteStream* stream, double value) {
		if (!stream->require(1)) {
			return wlet(false, _T("amf0 write number marker failed."));
		}
		stream->write_1bytes(RTMP_AMF0_Number);
		if (!stream->require(8)) {
			return wlet(false, _T("amf0 write number value failed."));
		}
		int64_t temp = 0x00;
		memcpy(&temp, &value, 8);
		stream->write_8bytes(temp);
		return true;
	}
	bool amf0_read_null(CByteStream* stream) {
		if (!stream->require(1)) {
			return wlet(false, _T("amf0 read null marker failed."));
		}
		char marker = stream->read_1bytes();
		if (marker != RTMP_AMF0_Null) {
			return wlet(false, _T("amf0 check null marker failed. marker=%#x, required=%#x"), marker, RTMP_AMF0_Null);
		}
		return true;
	}
	bool amf0_write_null(CByteStream* stream) {
		if (!stream->require(1)) {
			return wlet(false, _T("amf0 write null marker failed."));
		}
		stream->write_1bytes(RTMP_AMF0_Null);
		return true;
	}
	bool amf0_read_undefined(CByteStream* stream) {
		if (!stream->require(1)) {
			return wlet(false, _T("amf0 read undefined marker failed."));
		}
		char marker = stream->read_1bytes();
		if (marker != RTMP_AMF0_Undefined) {
			return wlet(false, _T("amf0 check undefined marker failed. marker=%#x, required=%#x"), marker, RTMP_AMF0_Undefined);
		}
		return true;
	}
	bool amf0_write_undefined(CByteStream* stream) {
		if (!stream->require(1)) {
			return wlet(false, _T("amf0 write undefined marker failed."));
		}
		stream->write_1bytes(RTMP_AMF0_Undefined);
		return true;
	}
	bool amf0_read_utf8(CByteStream* stream, std::string& value) {
		if (!stream->require(2)) {
			return wlet(false, _T("amf0 read string length failed."));
		}
		int16_t len(stream->read_2bytes());
		if (len <= 0) {
			return wlwt(true, _T("amf0 read empty string."));
		}
		if (!stream->require(len)) {
			return wlet(false, _T("amf0 read string data failed."));
		}
		std::string str = stream->read_string(len);
		value = str;
		return true;
	}
	bool amf0_write_utf8(CByteStream* stream, std::string value) {
		if (!stream->require(2)) {
			return wlet(false, _T("amf0 write string length failed."));
		}
		stream->write_2bytes((int16_t)value.length());
		if (value.length() <= 0) {
			return wlwt(true, _T("amf0 write empty string."));
		}
		if (!stream->require((int)value.length())) {
			return wlet(false, _T("amf0 write string data failed."));
		}
		stream->write_string(value);
		return true;
	}
	bool amf0_is_object_eof(CByteStream* stream) {
		if (stream->require(3)) {
			int32_t flag = stream->read_3bytes();
			stream->skip(-3);
			return 0x09 == flag;
		}
		return false;
	}
	bool amf0_write_object_eof(CByteStream* stream, Amf0ObjectEOF* value) {
		assert(value != NULL);
		if (!stream->require(2)) {
			return wlet(false, _T("amf0 write object eof value failed."));
		}
		stream->write_2bytes(0x00);
		if (!stream->require(1)) {
			return wlet(false, _T("amf0 write object eof marker failed."));
		}
		stream->write_1bytes(RTMP_AMF0_ObjectEnd);
		return true;
	}

	bool amf0_read_any(CByteStream* stream, Amf0Any** ppvalue) {
		if (!Amf0Any::discovery(stream, ppvalue)) {
			return wlet(false, _T("amf0 discovery any elem failed."));
		}
		assert(*ppvalue);
		if (!(*ppvalue)->read(stream)) {
			delete *ppvalue;
			*ppvalue = nullptr;
			return wlet(false, _T("amf0 parse elem failed. "));
		}
		return true;
	}
	bool amf0_write_any(CByteStream* stream, Amf0Any* value) {
		if (value == nullptr) return true;
		assert(value != NULL);
		return value->write(stream);
	}

	bool amf0_read(CByteStream* stream, std::string& value) {
		if (stream->empty()) return true;
		return amf0_read_string(stream, value);
	}
	bool amf0_read(CByteStream* stream, bool& value) {
		if (stream->empty()) return true;
		return amf0_read_boolean(stream, value);
	}
	bool amf0_read(CByteStream* stream, double& value) {
		if (stream->empty()) return true;
		return amf0_read_number(stream, value);
	}
	bool amf0_read(CByteStream* stream, Amf0Any*& ppvalue) {
		if (stream->empty()) return true;
		return amf0_read_any(stream, &ppvalue);
	}
	
	bool amf0_write(CByteStream* stream, std::string value) {
		return amf0_write_string(stream, value); 
	}
	bool amf0_write(CByteStream* stream, bool value) { 
		return amf0_write_boolean(stream, value);
	}
	bool amf0_write(CByteStream* stream, double value) {
		return amf0_write_number(stream, value); 
	}
	bool amf0_write(CByteStream* stream, Amf0Any* value) { 
		return amf0_write_any(stream, value);
	}


	//////////////////////////////////////////////////////////////////////////
	//Amf0Any
	Amf0Any::Amf0Any() {
		m_marker = RTMP_AMF0_Invalid;
	}
	Amf0Any::~Amf0Any()	{
	}
	bool Amf0Any::isString() {
		return m_marker == RTMP_AMF0_String;
	}
	bool Amf0Any::isBoolean() {
		return m_marker == RTMP_AMF0_Boolean;
	}
	bool Amf0Any::isNumber() {
		return m_marker == RTMP_AMF0_Number;
	}
	bool Amf0Any::isNull()	{
		return m_marker == RTMP_AMF0_Null;
	}
	bool Amf0Any::isUndefined() {
		return m_marker == RTMP_AMF0_Undefined;
	}
	bool Amf0Any::isObject() {
		return m_marker == RTMP_AMF0_Object;
	}
	bool Amf0Any::isEcmaArray()	{
		return m_marker == RTMP_AMF0_EcmaArray;
	}
	bool Amf0Any::isStrictArray() {
		return m_marker == RTMP_AMF0_StrictArray;
	}
	bool Amf0Any::isDate() {
		return m_marker == RTMP_AMF0_Date;
	}
	bool Amf0Any::isComplexObject()	{
		return isObject() || isObjectEof() || isEcmaArray() || isStrictArray();
	}

	std::string Amf0Any::toString()	{
		Amf0String* p = dynamic_cast<Amf0String*>(this);
		return p->m_value;
	}
	const char* Amf0Any::toStringRaw() {
		Amf0String* p = dynamic_cast<Amf0String*>(this);
		return p->m_value.data();
	}
	bool Amf0Any::toBoolean() {
		Amf0Boolean* p = dynamic_cast<Amf0Boolean*>(this);
		return p->m_value;
	}
	double Amf0Any::toNumber() {
		Amf0Number* p = dynamic_cast<Amf0Number*>(this);
		return p->m_value;
	}
	int64_t Amf0Any::toDate() {
		Amf0Date* p = dynamic_cast<Amf0Date*>(this);
		return p->date();
	}
	int16_t Amf0Any::toDateTimeZone() {
		Amf0Date* p = dynamic_cast<Amf0Date*>(this);
		return p->time_zone();
	}
	Amf0Object* Amf0Any::toObject() {
		Amf0Object* p = dynamic_cast<Amf0Object*>(this);
		return p;
	}
	Amf0EcmaArray* Amf0Any::toEcmaArray() {
		Amf0EcmaArray* p = dynamic_cast<Amf0EcmaArray*>(this);
		return p;
	}
	Amf0StrictArray* Amf0Any::toStrictArray() {
		Amf0StrictArray* p = dynamic_cast<Amf0StrictArray*>(this);
		return p;
	}

	void Amf0Any::setNumber(double value) {
		Amf0Number* p = dynamic_cast<Amf0Number*>(this);
		p->m_value = value;
	}
	bool Amf0Any::isObjectEof()	{
		return m_marker == RTMP_AMF0_ObjectEnd;
	}

	void fillLevelSpaces(std::stringstream& ss, int level) {
		for (int i = 0; i < level; i++) {
			ss << "    ";
		}
	}
	void amf0DoPrint(Amf0Any* any, std::stringstream& ss, int level)	{
		if (any->isBoolean()) {
			ss << "Boolean " << (any->toBoolean() ? "true" : "false") << std::endl;
		} else if (any->isNumber()) {
			ss << "Number " << std::fixed << any->toNumber() << std::endl;
		} else if (any->isString()) {
			ss << "String " << any->toString() << std::endl;
		} else if (any->isDate()) {
			ss << "Date " << std::hex << any->toDate() << "/" << std::hex << any->toDateTimeZone() << std::endl;
		} else if (any->isNull()) {
			ss << "Null" << std::endl;
		} else if (any->isEcmaArray()) {
			Amf0EcmaArray* obj(any->toEcmaArray());
			ss << "EcmaArray " << "(" << obj->count() << " items)" << std::endl;
			for (int i = 0; i < obj->count(); i++) {
				fillLevelSpaces(ss, level + 1);
				ss << "Elem '" << obj->keyAt(i) << "' ";
				if (obj->valueAt(i)->isComplexObject()) {
					amf0DoPrint(obj->valueAt(i), ss, level + 1);
				} else {
					amf0DoPrint(obj->valueAt(i), ss, 0);
				}
			}
		} else if (any->isStrictArray()) {
			Amf0StrictArray* obj(any->toStrictArray());
			ss << "StrictArray " << "(" << obj->count() << " items)" << std::endl;
			for (int i = 0; i < obj->count(); i++) {
				fillLevelSpaces(ss, level + 1);
				ss << "Elem ";
				if (obj->at(i)->isComplexObject()) {
					amf0DoPrint(obj->at(i), ss, level + 1);
				} else {
					amf0DoPrint(obj->at(i), ss, 0);
				}
			}
		} else if (any->isObject()) {
			Amf0Object* obj(any->toObject());
			ss << "Object " << "(" << obj->count() << " items)" << std::endl;
			for (int i = 0; i < obj->count(); i++) {
				fillLevelSpaces(ss, level + 1);
				ss << "Property '" << obj->keyAt(i) << "' ";
				if (obj->valueAt(i)->isComplexObject()) {
					amf0DoPrint(obj->valueAt(i), ss, level + 1);
				} else {
					amf0DoPrint(obj->valueAt(i), ss, 0);
				}
			}
		} else {
			ss << "Unknown" << std::endl;
		}
	}
	char* Amf0Any::humanPrint(char** pdata, int* psize) {
		std::stringstream ss;
		ss.precision(1);
		amf0DoPrint(this, ss, 0);
		std::string str = ss.str();
		if (str.empty()) {
			return NULL;
		}
		char* data = new char[str.length() + 1];
		memcpy(data, str.data(), str.length());
		data[str.length()] = 0;
		if (pdata) {
			*pdata = data;
		}
		if (psize) {
			*psize = (int)str.length();
		}
		return data;
	}
	
	Amf0Any* Amf0Any::string(const char* value) {
		return new Amf0String(value);
	}
	Amf0Any* Amf0Any::boolean(bool value) {
		return new Amf0Boolean(value);
	}
	Amf0Any* Amf0Any::number(double value) {
		return new Amf0Number(value);
	}
	Amf0Any* Amf0Any::null() {
		return new Amf0Null();
	}
	Amf0Any* Amf0Any::undefined() {
		return new Amf0Undefined();
	}
	Amf0Object* Amf0Any::object() {
		return new Amf0Object();
	}
	Amf0Any* Amf0Any::objectEof() {
		return new Amf0ObjectEOF();
	}
	Amf0EcmaArray* Amf0Any::ecmaArray()	{
		return new Amf0EcmaArray();
	}
	Amf0StrictArray* Amf0Any::strictArray() {
		return new Amf0StrictArray();
	}
	Amf0Any* Amf0Any::date(int64_t value) {
		return new Amf0Date(value);
	}

	bool Amf0Any::discovery(CByteStream* stream, Amf0Any** ppvalue) {
		if (amf0_is_object_eof(stream)) {
			*ppvalue = new Amf0ObjectEOF();
			return true;
		}
		if (!stream->require(1)) {
			return false;
		}
		char marker = stream->read_1bytes();
		stream->skip(-1);
		switch (marker) {
		case RTMP_AMF0_String: {
			*ppvalue = Amf0Any::string();
			return true;
		}
		case RTMP_AMF0_Boolean: {
			*ppvalue = Amf0Any::boolean();
			return true;
		}
		case RTMP_AMF0_Number: {
			*ppvalue = Amf0Any::number();
			return true;
		}
		case RTMP_AMF0_Null: {
			*ppvalue = Amf0Any::null();
			return true;
		}
		case RTMP_AMF0_Undefined: {
			*ppvalue = Amf0Any::undefined();
			return true;
		}
		case RTMP_AMF0_Object: {
			*ppvalue = Amf0Any::object();
			return true;
		}
		case RTMP_AMF0_EcmaArray: {
			*ppvalue = Amf0Any::ecmaArray();
			return true;
		}
		case RTMP_AMF0_StrictArray: {
			*ppvalue = Amf0Any::strictArray();
			return true;
		}
		case RTMP_AMF0_Date: {
			*ppvalue = Amf0Any::date();
			return true;
		}
		case RTMP_AMF0_Invalid:
		default: {
			return false;
		}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	//UnSortedHashtable
	UnSortedHashtable::UnSortedHashtable() {
	}
	UnSortedHashtable::~UnSortedHashtable() {
		clear();
	}
	int UnSortedHashtable::count() {
		return (int)properties.size();
	}
	void UnSortedHashtable::clear() {
		std::vector<Amf0ObjectPropertyType>::iterator it;
		for (it = properties.begin(); it != properties.end(); ++it) {
			Amf0ObjectPropertyType& elem = *it;
			Amf0Any* any = elem.second;
			delete any;
		}
		properties.clear();
	}
	std::string UnSortedHashtable::keyAt(int index)	{
		assert(index < count());
		Amf0ObjectPropertyType& elem = properties[index];
		return elem.first;
	}
	const char* UnSortedHashtable::keyRawAt(int index) {
		assert(index < count());
		Amf0ObjectPropertyType& elem = properties[index];
		return elem.first.data();
	}
	Amf0Any* UnSortedHashtable::valueAt(int index) {
		assert(index < count());
		Amf0ObjectPropertyType& elem = properties[index];
		return elem.second;
	}
	void UnSortedHashtable::set(std::string key, Amf0Any* value) {
		std::vector<Amf0ObjectPropertyType>::iterator it;
		for (it = properties.begin(); it != properties.end(); ++it) {
			Amf0ObjectPropertyType& elem = *it;
			std::string name = elem.first;
			Amf0Any* any = elem.second;
			if (key == name) {
				delete any;
				properties.erase(it);
				break;
			}
		}
		if (value) {
			properties.push_back(std::make_pair(key, value));
		}
	}
	Amf0Any* UnSortedHashtable::getProperty(std::string name) {
		std::vector<Amf0ObjectPropertyType>::iterator it;
		for (it = properties.begin(); it != properties.end(); ++it) {
			Amf0ObjectPropertyType& elem = *it;
			std::string key = elem.first;
			Amf0Any* any = elem.second;
			if (key == name) {
				return any;
			}
		}
		return NULL;
	}
	Amf0Any* UnSortedHashtable::ensurePropertyString(std::string name) {
		Amf0Any* prop(getProperty(name));
		if (!prop) {
			return NULL;
		}
		if (!prop->isString()) {
			return NULL;
		}
		return prop;
	}
	Amf0Any* UnSortedHashtable::ensurePropertyNumber(std::string name) {
		Amf0Any* prop(getProperty(name));
		if (!prop) {
			return NULL;
		}
		if (!prop->isNumber()) {
			return NULL;
		}
		return prop;
	}
	void UnSortedHashtable::remove(std::string name) {
		std::vector<Amf0ObjectPropertyType>::iterator it;
		for (it = properties.begin(); it != properties.end();) {
			std::string key = it->first;
			Amf0Any* any = it->second;
			if (key == name) {
				delete any;
				it = properties.erase(it);
			} else {
				++it;
			}
		}
	}
	void UnSortedHashtable::copy(UnSortedHashtable* src) {
		std::vector<Amf0ObjectPropertyType>::iterator it;
		for (it = src->properties.begin(); it != src->properties.end(); ++it) {
			Amf0ObjectPropertyType& elem = *it;
			std::string key = elem.first;
			Amf0Any* any = elem.second;
			set(key, any->copy());
		}
	}

	//////////////////////////////////////////////////////////////////////////
	//Amf0ObjectEOF
	Amf0ObjectEOF::Amf0ObjectEOF(){
		m_marker = RTMP_AMF0_ObjectEnd;
	}
	Amf0ObjectEOF::~Amf0ObjectEOF() {
	}
	int Amf0ObjectEOF::totalSize() {
		return Amf0Size::objectEof();
	}
	bool Amf0ObjectEOF::read(CByteStream* stream) {
		if (!stream->require(2)) {
			return wlet(false, _T("amf0 read object eof value failed. "));
		}
		int16_t temp = stream->read_2bytes();
		if (temp != 0x00) {
			return wlet(false, _T("amf0 read object eof value check failed. must be 0x00, actual is %#x"), temp);
		}
		if (!stream->require(1)) {
			return wlet(false, _T("amf0 read object eof marker failed. "));
		}
		char marker = stream->read_1bytes();
		if (marker != RTMP_AMF0_ObjectEnd) {
			return wlet(false, _T("amf0 check object eof marker failed. marker=%#x, required=%#x"), marker, RTMP_AMF0_ObjectEnd);
		}
		return true;
	}
	bool Amf0ObjectEOF::write(CByteStream* stream) {
		if (!stream->require(2)) {
			return wlet(false, _T("amf0 write object eof value failed. "));
		}
		stream->write_2bytes(0x00);
		if (!stream->require(1)) {
			return wlet(false, _T("amf0 write object eof marker failed. "));
		}
		stream->write_1bytes(RTMP_AMF0_ObjectEnd);
		return true;
	}
	Amf0Any* Amf0ObjectEOF::copy() {
		return new Amf0ObjectEOF();
	}

	//////////////////////////////////////////////////////////////////////////
	//Amf0Object
	Amf0Object::Amf0Object() {
		properties = new UnSortedHashtable();
		eof = new Amf0ObjectEOF();
		m_marker = RTMP_AMF0_Object;
	}
	Amf0Object::~Amf0Object() {
		delete properties;
		delete eof;
	}
	int Amf0Object::totalSize() {
		int size = 1;
		for (int i = 0; i < properties->count(); i++) {
			std::string name = keyAt(i);
			Amf0Any* value(valueAt(i));
			size += Amf0Size::utf8(name);
			size += Amf0Size::any(value);
		}
		size += Amf0Size::objectEof();
		return size;
	}
	bool Amf0Object::read(CByteStream* stream) {
		if (!stream->require(1)) {
			return wlet(false, _T("amf0 read object marker failed. "));
		}
		char marker = stream->read_1bytes();
		if (marker != RTMP_AMF0_Object) {
			return wlet(false, _T("amf0 check object marker failed. marker=%#x, required=%#x"), marker, RTMP_AMF0_Object);
		}
		while (!stream->empty()) {
			if (amf0_is_object_eof(stream)) {
				Amf0ObjectEOF pbj_eof;
				if (!pbj_eof.read(stream)) {
					return wlet(false, _T("amf0 object read eof failed. "));
				}
				break;
			}
			std::string property_name;
			if (!amf0_read_utf8(stream, property_name)) {
				return wlet(false, _T("amf0 object read property name failed. "));
			}
			Amf0Any* property_value = NULL;
			if (!amf0_read_any(stream, &property_value)) {
				delete property_value;
				return wlet(false, _T("amf0 object read property_value failed. name=%s"), property_name.c_str());
			}
			set(property_name, property_value);
		}
		return true;
	}
	bool Amf0Object::write(CByteStream* stream) {
		if (!stream->require(1)) {
			return wlet(false, _T("amf0 write object marker failed. "));
		}
		stream->write_1bytes(RTMP_AMF0_Object);
		for (int i = 0; i < properties->count(); i++) {
			std::string name = this->keyAt(i);
			Amf0Any* any = this->valueAt(i);
			if (!amf0_write_utf8(stream, name)) {
				return wlet(false, _T("write object property name failed. "));
			}
			if (!amf0_write_any(stream, any)) {
				return wlet(false, _T("write object property value failed. "));
			}
		}
		if (!eof->write(stream)) {
			return wlet(false, _T("write object eof failed. "));
		}
		return true;
	}
	Amf0Any* Amf0Object::copy() {
		Amf0Object* copy(new Amf0Object());
		copy->properties->copy(properties);
		return copy;
	}
	void Amf0Object::clear() {
		properties->clear();
	}
	int Amf0Object::count() {
		return properties->count();
	}
	std::string Amf0Object::keyAt(int index) {
		return properties->keyAt(index);
	}
	const char* Amf0Object::keyRawAt(int index) {
		return properties->keyRawAt(index);
	}
	Amf0Any* Amf0Object::valueAt(int index) {
		return properties->valueAt(index);
	}
	void Amf0Object::set(std::string key, Amf0Any* value) {
		properties->set(key, value);
	}
	Amf0Any* Amf0Object::getProperty(std::string name) {
		return properties->getProperty(name);
	}
	Amf0Any* Amf0Object::ensurePropertyString(std::string name) {
		return properties->ensurePropertyString(name);
	}
	Amf0Any* Amf0Object::ensurePropertyNumber(std::string name) {
		return properties->ensurePropertyNumber(name);
	}
	void Amf0Object::remove(std::string name) {
		properties->remove(name);
	}

	//////////////////////////////////////////////////////////////////////////
	//Amf0EcmaArray
	Amf0EcmaArray::Amf0EcmaArray() {
		_count = 0;
		properties = new UnSortedHashtable();
		eof = new Amf0ObjectEOF();
		m_marker = RTMP_AMF0_EcmaArray;
	}
	Amf0EcmaArray::~Amf0EcmaArray() {
		delete properties;
		delete eof;
	}
	int Amf0EcmaArray::totalSize() {
		int size = 1 + 4;
		for (int i = 0; i < properties->count(); i++) {
			std::string name(keyAt(i));
			Amf0Any* value(valueAt(i));
			size += Amf0Size::utf8(name);
			size += Amf0Size::any(value);
		}
		size += Amf0Size::objectEof();
		return size;
	}
	bool Amf0EcmaArray::read(CByteStream* stream) {
		if (!stream->require(1)) {
			return wlet(false, _T("amf0 read ecma_array marker failed. "));
		}
		char marker = stream->read_1bytes();
		if (marker != RTMP_AMF0_EcmaArray) {
			return wlet(false, _T("amf0 check ecma_array marker failed. marker=%#x, required=%#x"), marker, RTMP_AMF0_EcmaArray);
		}
		if (!stream->require(4)) {
			return wlet(false, _T("amf0 read ecma_array count failed. "));
		}
		int32_t count = stream->read_4bytes();
		this->_count = count;
		while (!stream->empty()) {
			if (amf0_is_object_eof(stream)) {
				Amf0ObjectEOF pbj_eof;
				if (!pbj_eof.read(stream)) {
					return wlet(false, _T("amf0 ecma_array read eof failed. "));
				}
				break;
			}
			std::string property_name;
			if (!amf0_read_utf8(stream, property_name)) {
				return wlet(false, _T("amf0 ecma_array read property name failed. "));
			}
			Amf0Any* property_value = NULL;
			if (!amf0_read_any(stream, &property_value)) {
				return wlet(false, _T("amf0 ecma_array read property_value failed. name=%s"), property_name.c_str());
			}
			this->set(property_name, property_value);
		}
		return true;
	}
	bool Amf0EcmaArray::write(CByteStream* stream) {
		if (!stream->require(1)) {
			return wlet(false, _T("amf0 write ecma_array marker failed. "));
		}
		stream->write_1bytes(RTMP_AMF0_EcmaArray);
		if (!stream->require(4)) {
			return wlet(false, _T("amf0 write ecma_array count failed. "));
		}
		stream->write_4bytes(this->_count);
		for (int i = 0; i < properties->count(); i++) {
			std::string name(this->keyAt(i));
			Amf0Any* any(this->valueAt(i));
			if (!amf0_write_utf8(stream, name)) {
				return wlet(false, _T("write ecma_array property name failed. "));
			}
			if (!amf0_write_any(stream, any)) {
				return wlet(false, _T("write ecma_array property value failed. "));
			}
		}
		if (!eof->write(stream)) {
			return wlet(false, _T("write ecma_array eof failed. "));
		}
		return true;
	}
	Amf0Any* Amf0EcmaArray::copy() {
		Amf0EcmaArray* copy(new Amf0EcmaArray());
		copy->properties->copy(properties);
		copy->_count = _count;
		return copy;
	}
	void Amf0EcmaArray::clear() {
		properties->clear();
	}
	int Amf0EcmaArray::count() {
		return properties->count();
	}
	std::string Amf0EcmaArray::keyAt(int index) {
		return properties->keyAt(index);
	}
	const char* Amf0EcmaArray::keyRawAt(int index) {
		return properties->keyRawAt(index);
	}
	Amf0Any* Amf0EcmaArray::valueAt(int index) {
		return properties->valueAt(index);
	}
	void Amf0EcmaArray::set(std::string key, Amf0Any* value) {
		properties->set(key, value);
	}
	Amf0Any* Amf0EcmaArray::getProperty(std::string name) {
		return properties->getProperty(name);
	}
	Amf0Any* Amf0EcmaArray::ensurePropertyString(std::string name) {
		return properties->ensurePropertyString(name);
	}
	Amf0Any* Amf0EcmaArray::ensurePropertyNumber(std::string name) {
		return properties->ensurePropertyNumber(name);
	}

	//////////////////////////////////////////////////////////////////////////
	//Amf0StrictArray
	Amf0StrictArray::Amf0StrictArray() {
		m_marker = RTMP_AMF0_StrictArray;
		_count = 0;
	}
	Amf0StrictArray::~Amf0StrictArray() {
		std::vector<Amf0Any*>::iterator it;
		for (it = properties.begin(); it != properties.end(); ++it) {
			Amf0Any* any = *it;
			delete any;
		}
		properties.clear();
	}
	int Amf0StrictArray::totalSize() {
		int size = 1 + 4;
		for (int i = 0; i < (int)properties.size(); i++) {
			Amf0Any* any = properties[i];
			size += any->totalSize();
		}
		return size;
	}
	bool Amf0StrictArray::read(CByteStream* stream) {
		if (!stream->require(1)) {
			return wlet(false, _T("amf0 read strict_array marker failed. "));
		}
		char marker = stream->read_1bytes();
		if (marker != RTMP_AMF0_StrictArray) {
			return wlet(false, _T("amf0 check strict_array marker failed.  marker=%#x, required=%#x"), marker, RTMP_AMF0_StrictArray);
		}
		if (!stream->require(4)) {
			return wlet(false, _T("amf0 read strict_array count failed. "));
		}
		int32_t count = stream->read_4bytes();
		this->_count = count;
		for (int i = 0; i < count && !stream->empty(); i++) {
			Amf0Any* elem = NULL;
			if (!amf0_read_any(stream, &elem)) {
				return wlet(false, _T("amf0 strict_array read value failed. "));
			}
			properties.push_back(elem);
		}
		return true;
	}
	bool Amf0StrictArray::write(CByteStream* stream) {
		if (!stream->require(1)) {
			return wlet(false, _T("amf0 write strict_array marker failed. "));
		}
		stream->write_1bytes(RTMP_AMF0_StrictArray);
		if (!stream->require(4)) {
			return wlet(false, _T("amf0 write strict_array count failed. "));
		}
		stream->write_4bytes(this->_count);
		for (int i = 0; i < (int)properties.size(); i++) {
			Amf0Any* any = properties[i];
			if (!amf0_write_any(stream, any)) {
				return wlet(false, _T("write strict_array property value failed. "));
			}
		}
		return true;
	}
	Amf0Any* Amf0StrictArray::copy() {
		Amf0StrictArray* copy(new Amf0StrictArray());
		std::vector<Amf0Any*>::iterator it;
		for (it = properties.begin(); it != properties.end(); ++it) {
			Amf0Any* any = *it;
			copy->append(any->copy());
		}
		copy->_count = _count;
		return copy;
	}
	void Amf0StrictArray::clear() {
		properties.clear();
	}
	int Amf0StrictArray::count() {
		return (int)properties.size();
	}
	Amf0Any* Amf0StrictArray::at(int index) {
		assert(index < (int)properties.size());
		return properties.at(index);
	}
	void Amf0StrictArray::append(Amf0Any* any) {
		properties.push_back(any);
		_count = (int32_t)properties.size();
	}

	//////////////////////////////////////////////////////////////////////////
	//Amf0Size
	int Amf0Size::utf8(std::string value) {
		return 2 + (int)value.length();
	}
	int Amf0Size::string(std::string value) {
		return 1 + Amf0Size::utf8(value);
	}
	int Amf0Size::number() {
		return 1 + 8;
	}
	int Amf0Size::date() {
		return 1 + 8 + 2;
	}
	int Amf0Size::null() {
		return 1;
	}
	int Amf0Size::undefined() {
		return 1;
	}
	int Amf0Size::boolean() {
		return 1 + 1;
	}
	int Amf0Size::object(Amf0Object* obj) {
		if (!obj) {
			return 0;
		}
		return obj->totalSize();
	}
	int Amf0Size::objectEof() {
		return 2 + 1;
	}
	int Amf0Size::ecmaArray(Amf0EcmaArray* arr) {
		if (!arr) {
			return 0;
		}
		return arr->totalSize();
	}
	int Amf0Size::strictArray(Amf0StrictArray* arr) {
		if (!arr) {
			return 0;
		}
		return arr->totalSize();
	}
	int Amf0Size::any(Amf0Any* o) {
		if (!o) {
			return 0;
		}
		return o->totalSize();
	}

	//////////////////////////////////////////////////////////////////////////
	//Amf0String
	Amf0String::Amf0String(const char* value) {
		m_marker = RTMP_AMF0_String;
		if (value) {
			m_value = value;
		}
	}
	Amf0String::~Amf0String() {
	}
	int Amf0String::totalSize() {
		return Amf0Size::string(m_value);
	}
	bool Amf0String::read(CByteStream* stream) {
		return amf0_read_string(stream, m_value);
	}
	bool Amf0String::write(CByteStream* stream) {
		return amf0_write_string(stream, m_value);
	}
	Amf0Any* Amf0String::copy() {
		Amf0String* copy(new Amf0String(m_value.c_str()));
		return copy;
	}

	//////////////////////////////////////////////////////////////////////////
	//Amf0Boolean
	Amf0Boolean::Amf0Boolean(bool value) {
		m_marker = RTMP_AMF0_Boolean;
		m_value = value;
	}
	Amf0Boolean::~Amf0Boolean() {
	}
	int Amf0Boolean::totalSize() {
		return Amf0Size::boolean();
	}
	bool Amf0Boolean::read(CByteStream* stream) {
		return amf0_read_boolean(stream, m_value);
	}
	bool Amf0Boolean::write(CByteStream* stream) {
		return amf0_write_boolean(stream, m_value);
	}
	Amf0Any* Amf0Boolean::copy() {
		Amf0Boolean* copy = new Amf0Boolean(m_value);
		return copy;
	}

	//////////////////////////////////////////////////////////////////////////
	//Amf0Number
	Amf0Number::Amf0Number(double value) {
		m_marker = RTMP_AMF0_Number;
		m_value = value;
	}
	Amf0Number::~Amf0Number() {
	}
	int Amf0Number::totalSize() {
		return Amf0Size::number();
	}
	bool Amf0Number::read(CByteStream* stream) {
		return amf0_read_number(stream, m_value);
	}
	bool Amf0Number::write(CByteStream* stream) {
		return amf0_write_number(stream, m_value);
	}
	Amf0Any* Amf0Number::copy() {
		Amf0Number* copy(new Amf0Number(m_value));
		return copy;
	}

	//////////////////////////////////////////////////////////////////////////
	//Amf0Date
	Amf0Date::Amf0Date(int64_t value) {
		m_marker = RTMP_AMF0_Date;
		m_date = value;
		m_timezone = 0;
	}
	Amf0Date::~Amf0Date() {
	}
	int Amf0Date::totalSize() {
		return Amf0Size::date();
	}
	bool Amf0Date::read(CByteStream* stream) {
		if (!stream->require(1)) {
			return wlet(false, _T("amf0 read date marker failed. "));
		}
		char marker(stream->read_1bytes());
		if (marker != RTMP_AMF0_Date) {
			return wlet(false, _T("amf0 check date marker failed. marker=%#x, required=%#x"), marker, RTMP_AMF0_Date);
		}
		if (!stream->require(8)) {
			return wlet(false, _T("amf0 read date failed. "));
		}
		m_date = stream->read_8bytes();
		if (!stream->require(2)) {
			return wlet(false, _T("amf0 read time zone failed. "));
		}
		m_timezone = stream->read_2bytes();
		return true;
	}
	bool Amf0Date::write(CByteStream* stream) {
		if (!stream->require(1)) {
			return wlet(false, _T("amf0 write date marker failed. "));
		}
		stream->write_1bytes(RTMP_AMF0_Date);
		if (!stream->require(8)) {
			return wlet(false, _T("amf0 write date failed. "));
		}
		stream->write_8bytes(m_date);
		if (!stream->require(2)) {
			return wlet(false, _T("amf0 write time zone failed. "));
		}
		stream->write_2bytes(m_timezone);
		return true;
	}
	Amf0Any* Amf0Date::copy() {
		Amf0Date* copy(new Amf0Date(0));
		copy->m_date = m_date;
		copy->m_timezone = m_timezone;
		return copy;
	}

	//////////////////////////////////////////////////////////////////////////
	//Amf0Null
	Amf0Null::Amf0Null() {
		m_marker = RTMP_AMF0_Null;
	}
	Amf0Null::~Amf0Null() {
	}
	int Amf0Null::totalSize() {
		return Amf0Size::null();
	}
	bool Amf0Null::read(CByteStream* stream) {
		return amf0_read_null(stream);
	}
	bool Amf0Null::write(CByteStream* stream) {
		return amf0_write_null(stream);
	}
	Amf0Any* Amf0Null::copy() {
		Amf0Null* copy(new Amf0Null());
		return copy;
	}

	//////////////////////////////////////////////////////////////////////////
	//Amf0Undefined
	Amf0Undefined::Amf0Undefined() {
		m_marker = RTMP_AMF0_Undefined;
	}
	Amf0Undefined::~Amf0Undefined() {
	}
	int Amf0Undefined::totalSize() {
		return Amf0Size::undefined();
	}
	bool Amf0Undefined::read(CByteStream* stream) {
		return amf0_read_undefined(stream);
	}
	bool Amf0Undefined::write(CByteStream* stream) {
		return amf0_write_undefined(stream);
	}
	Amf0Any* Amf0Undefined::copy() {
		Amf0Undefined* copy(new Amf0Undefined());
		return copy;
	}
};