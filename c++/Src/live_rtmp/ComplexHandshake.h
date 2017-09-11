#pragma once
#include "HandshakeBytes.h"
#include "openssl/dh.h"
#include "ByteStream.h"
#include "SmartArr.h"
#include "SmartPtr.h"
#include "os.h"

namespace rtmp_handshake {

	extern uint8_t SrsGenuineFMSKey[];
	extern uint8_t SrsGenuineFPKey[];
	bool openssl_HMACsha256(const void* key, int key_size, const void* data, int data_size, void* digest);
	bool openssl_generate_key(char* public_key, int32_t size);

	class SrsDH {
	public:
		SrsDH();
		virtual ~SrsDH();
	public:
		virtual bool initialize(bool ensure_128bytes_public_key = false);
		virtual bool copy_public_key(char* pkey, int32_t& pkey_size);
		virtual bool copy_shared_key(const char* ppkey, int32_t ppkey_size, char* skey, int32_t& skey_size);
	private:
		virtual bool do_initialize();
		virtual void close();
	private:
		DH* pdh;
	};

	enum srs_schema_type {
		srs_schema_invalid = 2,
		srs_schema0 = 0,
		srs_schema1 = 1,
	};

	class key_block {
	public:
		key_block();
		virtual ~key_block();
	public:
		// parse key block from c1s1.
		// if created, user must free it by srs_key_block_free
		// @stream contains c1s1_key_bytes the key start bytes
		bool parse(CByteStream* stream);
	private:
		// calc the offset of key,
		// the key->offset cannot be used as the offset of key.
		int calc_valid_offset();
	public:
		CSmartArr<char> random0;
		int random0_size;
		char key[128];
		// (764-offset-128-4)bytes
		CSmartArr<char> random1;
		int random1_size;
		// 4bytes
		int32_t offset;
	};

	/**
	* 764bytes digest structure
	*     offset: 4bytes
	*     random-data: (offset)bytes
	*     digest-data: 32bytes
	*     random-data: (764-4-offset-32)bytes
	* @see also: http://blog.csdn.net/win_lin/article/details/13006803
	*/
	class digest_block	{
	public:
		digest_block();
		virtual ~digest_block();
	public:
		// parse digest block from c1s1.
		// if created, user must free it by srs_digest_block_free
		// @stream contains c1s1_digest_bytes the digest start bytes
		bool parse(CByteStream* stream);
	private:
		// calc the offset of digest,
		// the key->offset cannot be used as the offset of digest.
		int calc_valid_offset();
	public:
		// 4bytes
		int32_t offset;
		// (offset)bytes
		CSmartArr<char> random0;
		int random0_size;
		// 32bytes
		char digest[32];
		// (764-4-offset-32)bytes
		CSmartArr<char> random1;
		int random1_size;
	};

	class c1s1;

	/**
	* the c1s1 strategy, use schema0 or schema1.
	* the template method class to defines common behaviors,
	* while the concrete class to implements in schema0 or schema1.
	*/
	class c1s1_strategy {
	protected:
		key_block key;
		digest_block digest;
	public:
		c1s1_strategy();
		virtual ~c1s1_strategy();
	public:
		virtual srs_schema_type schema() = 0;
		virtual char* get_digest();
		virtual char* get_key();
		virtual bool dump(c1s1* owner, char* _c1s1, int size);
		/*server: parse the c1s1, discovery the key and digest by schema.
		* use the c1_validate_digest() to valid the digest of c1.*/
		virtual bool parse(char* _c1s1, int size) = 0;
	public:
		/**
		* client: create and sign c1 by schema.
		* sign the c1, generate the digest.
		*         calc_c1_digest(c1, schema) {
		*            get c1s1-joined from c1 by specified schema
		*            digest-data = HMACsha256(c1s1-joined, FPKey, 30)
		*            return digest-data;
		*        }
		*        random fill 1536bytes c1 // also fill the c1-128bytes-key
		*        time = time() // c1[0-3]
		*        version = [0x80, 0x00, 0x07, 0x02] // c1[4-7]
		*        schema = choose schema0 or schema1
		*        digest-data = calc_c1_digest(c1, schema)
		*        copy digest-data to c1
		*/
		virtual bool c1_create(c1s1* owner);
		/**
		* server: validate the parsed c1 schema
		*/
		virtual bool c1_validate_digest(c1s1* owner, bool& is_valid);
		/**
		* server: create and sign the s1 from c1.
		*       // decode c1 try schema0 then schema1
		*       c1-digest-data = get-c1-digest-data(schema0)
		*       if c1-digest-data equals to calc_c1_digest(c1, schema0) {
		*           c1-key-data = get-c1-key-data(schema0)
		*           schema = schema0
		*       } else {
		*           c1-digest-data = get-c1-digest-data(schema1)
		*           if c1-digest-data not equals to calc_c1_digest(c1, schema1) {
		*               switch to simple handshake.
		*               return
		*           }
		*           c1-key-data = get-c1-key-data(schema1)
		*           schema = schema1
		*       }
		*
		*       // generate s1
		*       random fill 1536bytes s1
		*       time = time() // c1[0-3]
		*       version = [0x04, 0x05, 0x00, 0x01] // s1[4-7]
		*       s1-key-data=shared_key=DH_compute_key(peer_pub_key=c1-key-data)
		*       get c1s1-joined by specified schema
		*       s1-digest-data = HMACsha256(c1s1-joined, FMSKey, 36)
		*       copy s1-digest-data and s1-key-data to s1.
		* @param c1, to get the peer_pub_key of client.
		*/
		virtual bool s1_create(c1s1* owner, c1s1* c1);
		/**
		* server: validate the parsed s1 schema
		*/
		virtual bool s1_validate_digest(c1s1* owner, bool& is_valid);
	public:
		/**
		* calc the digest for c1
		*/
		virtual bool calc_c1_digest(c1s1* owner, char*& c1_digest);
		/**
		* calc the digest for s1
		*/
		virtual bool calc_s1_digest(c1s1* owner, char*& s1_digest);
		/**
		* copy whole c1s1 to bytes.
		* @param size must always be 1536 with digest, and 1504 without digest.
		*/
		virtual bool copy_to(c1s1* owner, char* bytes, int size, bool with_digest) = 0;
		/**
		* copy time and version to stream.
		*/
		virtual void copy_time_version(CByteStream* stream, c1s1* owner);
		/**
		* copy key to stream.
		*/
		virtual void copy_key(CByteStream* stream);
		/**
		* copy digest to stream.
		*/
		virtual void copy_digest(CByteStream* stream, bool with_digest);
	};

	/**
	* c1s1 schema0
	*     key: 764bytes
	*     digest: 764bytes
	*/
	class c1s1_strategy_schema0 : public c1s1_strategy {
	public:
		c1s1_strategy_schema0();
		virtual ~c1s1_strategy_schema0();
	public:
		virtual srs_schema_type schema();
		virtual bool parse(char* _c1s1, int size);
	public:
		virtual bool copy_to(c1s1* owner, char* bytes, int size, bool with_digest);
	};

	/**
	* c1s1 schema1
	*     digest: 764bytes
	*     key: 764bytes
	*/
	class c1s1_strategy_schema1 : public c1s1_strategy {
	public:
		c1s1_strategy_schema1();
		virtual ~c1s1_strategy_schema1();
	public:
		virtual srs_schema_type schema();
		virtual bool parse(char* _c1s1, int size);
	public:
		virtual bool copy_to(c1s1* owner, char* bytes, int size, bool with_digest);
	};

	/**
	* c1s1 schema0
	*     time: 4bytes
	*     version: 4bytes
	*     key: 764bytes
	*     digest: 764bytes
	* c1s1 schema1
	*     time: 4bytes
	*     version: 4bytes
	*     digest: 764bytes
	*     key: 764bytes
	* @see also: http://blog.csdn.net/win_lin/article/details/13006803
	*/
	class c1s1 {
	public:
		// 4bytes
		int32_t time;
		// 4bytes
		int32_t version;
		// 764bytes+764bytes
		CSmartPtr<c1s1_strategy> payload;
	public:
		c1s1();
		virtual ~c1s1();
	public:
		virtual srs_schema_type schema();
		virtual char* get_digest();
		virtual char* get_key();
	public:
		/**
		* copy to bytes.
		* @param size, must always be 1536.
		*/
		virtual bool dump(char* _c1s1, int size);
		/**
		* server: parse the c1s1, discovery the key and digest by schema.
		* @param size, must always be 1536.
		* use the c1_validate_digest() to valid the digest of c1.
		* use the s1_validate_digest() to valid the digest of s1.
		*/
		virtual bool parse(char* _c1s1, int size, srs_schema_type _schema);
	public:
		/**
		* client: create and sign c1 by schema.
		* sign the c1, generate the digest.
		*         calc_c1_digest(c1, schema) {
		*            get c1s1-joined from c1 by specified schema
		*            digest-data = HMACsha256(c1s1-joined, FPKey, 30)
		*            return digest-data;
		*        }
		*        random fill 1536bytes c1 // also fill the c1-128bytes-key
		*        time = time() // c1[0-3]
		*        version = [0x80, 0x00, 0x07, 0x02] // c1[4-7]
		*        schema = choose schema0 or schema1
		*        digest-data = calc_c1_digest(c1, schema)
		*        copy digest-data to c1
		*/
		virtual bool c1_create(srs_schema_type _schema);
		/**
		* server: validate the parsed c1 schema
		*/
		virtual bool c1_validate_digest(bool& is_valid);
	public:
		/**
		* server: create and sign the s1 from c1.
		*       // decode c1 try schema0 then schema1
		*       c1-digest-data = get-c1-digest-data(schema0)
		*       if c1-digest-data equals to calc_c1_digest(c1, schema0) {
		*           c1-key-data = get-c1-key-data(schema0)
		*           schema = schema0
		*       } else {
		*           c1-digest-data = get-c1-digest-data(schema1)
		*           if c1-digest-data not equals to calc_c1_digest(c1, schema1) {
		*               switch to simple handshake.
		*               return
		*           }
		*           c1-key-data = get-c1-key-data(schema1)
		*           schema = schema1
		*       }
		*
		*       // generate s1
		*       random fill 1536bytes s1
		*       time = time() // c1[0-3]
		*       version = [0x04, 0x05, 0x00, 0x01] // s1[4-7]
		*       s1-key-data=shared_key=DH_compute_key(peer_pub_key=c1-key-data)
		*       get c1s1-joined by specified schema
		*       s1-digest-data = HMACsha256(c1s1-joined, FMSKey, 36)
		*       copy s1-digest-data and s1-key-data to s1.
		*/
		virtual bool s1_create(c1s1* c1);
		/**
		* server: validate the parsed s1 schema
		*/
		virtual bool s1_validate_digest(bool& is_valid);
	};

	/**
	* the c2s2 complex handshake structure.
	* random-data: 1504bytes
	* digest-data: 32bytes
	* @see also: http://blog.csdn.net/win_lin/article/details/13006803
	*/
	class c2s2 {
	public:
		char random[1504];
		char digest[32];
	public:
		c2s2();
		virtual ~c2s2();
	public:
		/**
		* copy to bytes.
		* @param size, must always be 1536.
		*/
		virtual bool dump(char* _c2s2, int size);
		/**
		* parse the c2s2
		* @param size, must always be 1536.
		*/
		virtual bool parse(char* _c2s2, int size);
	public:
		/**
		* create c2.
		* random fill c2s2 1536 bytes
		*
		* // client generate C2, or server valid C2
		* temp-key = HMACsha256(s1-digest, FPKey, 62)
		* c2-digest-data = HMACsha256(c2-random-data, temp-key, 32)
		*/
		virtual bool c2_create(c1s1* s1);

		/**
		* validate the c2 from client.
		*/
		virtual bool c2_validate(c1s1* s1, bool& is_valid);
	public:
		/**
		* create s2.
		* random fill c2s2 1536 bytes
		*
		* // server generate S2, or client valid S2
		* temp-key = HMACsha256(c1-digest, FMSKey, 68)
		* s2-digest-data = HMACsha256(s2-random-data, temp-key, 32)
		*/
		virtual bool s2_create(c1s1* c1);

		/**
		* validate the s2 from server.
		*/
		virtual bool s2_validate(c1s1* c1, bool& is_valid);
	};
}

