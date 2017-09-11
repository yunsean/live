#include "ComplexHandshake.h"
#include "openssl/hmac.h"
#include "Writelog.h"
#include "SmartHdr.h"
#include "xutils.h"
#include "xstring.h"

#define SRS_OpensslHashSize		512
#define RTMP_SIG_SRS_HANDSHAKE	"RTMP"

namespace rtmp_handshake {

	// 68bytes FMS key which is used to sign the sever packet.
	uint8_t SrsGenuineFMSKey[] = {
		0x47, 0x65, 0x6e, 0x75, 0x69, 0x6e, 0x65, 0x20,
		0x41, 0x64, 0x6f, 0x62, 0x65, 0x20, 0x46, 0x6c,
		0x61, 0x73, 0x68, 0x20, 0x4d, 0x65, 0x64, 0x69,
		0x61, 0x20, 0x53, 0x65, 0x72, 0x76, 0x65, 0x72,
		0x20, 0x30, 0x30, 0x31, // Genuine Adobe Flash Media Server 001
		0xf0, 0xee, 0xc2, 0x4a, 0x80, 0x68, 0xbe, 0xe8,
		0x2e, 0x00, 0xd0, 0xd1, 0x02, 0x9e, 0x7e, 0x57,
		0x6e, 0xec, 0x5d, 0x2d, 0x29, 0x80, 0x6f, 0xab,
		0x93, 0xb8, 0xe6, 0x36, 0xcf, 0xeb, 0x31, 0xae
	}; // 68
	// 62bytes FP key which is used to sign the client packet.
	uint8_t SrsGenuineFPKey[] = {
		0x47, 0x65, 0x6E, 0x75, 0x69, 0x6E, 0x65, 0x20,
		0x41, 0x64, 0x6F, 0x62, 0x65, 0x20, 0x46, 0x6C,
		0x61, 0x73, 0x68, 0x20, 0x50, 0x6C, 0x61, 0x79,
		0x65, 0x72, 0x20, 0x30, 0x30, 0x31, // Genuine Adobe Flash Player 001
		0xF0, 0xEE, 0xC2, 0x4A, 0x80, 0x68, 0xBE, 0xE8,
		0x2E, 0x00, 0xD0, 0xD1, 0x02, 0x9E, 0x7E, 0x57,
		0x6E, 0xEC, 0x5D, 0x2D, 0x29, 0x80, 0x6F, 0xAB,
		0x93, 0xB8, 0xE6, 0x36, 0xCF, 0xEB, 0x31, 0xAE
	}; // 62


	bool do_openssl_HMACsha256(HMAC_CTX* ctx, const void* data, int data_size, void* digest, unsigned int* digest_size) {
		if (HMAC_Update(ctx, (unsigned char *)data, data_size) < 0) {
			return wlet(false, _T("Openssl sha256 update error"));
		}
		if (HMAC_Final(ctx, (unsigned char *)digest, digest_size) < 0) {
			return wlet(false, _T("Openssl sha256 final error"));
		}
		return true;
	}
	/**
	* sha256 digest algorithm.
	* @param key the sha256 key, NULL to use EVP_Digest, for instance, hashlib.sha256(data).digest().
	*/
	bool openssl_HMACsha256(const void* key, int key_size, const void* data, int data_size, void* digest) {
		unsigned int digest_size = 0;
		unsigned char* temp_key = (unsigned char*)key;
		unsigned char* temp_digest = (unsigned char*)digest;
		if (key == NULL) {
			// use data to digest.
			// @see ./crypto/sha/sha256t.c
			// @see ./crypto/evp/digest.c
			if (EVP_Digest(data, data_size, temp_digest, &digest_size, EVP_sha256(), NULL) < 0) {
				return wlet(false, _T("Openssl sha256 evp digest error"));
			}
		} else {
			// use key-data to digest.
			HMAC_CTX ctx;
			// @remark, if no key, use EVP_Digest to digest,
			// for instance, in python, hashlib.sha256(data).digest().
			HMAC_CTX_init(&ctx);
			if (HMAC_Init_ex(&ctx, temp_key, key_size, EVP_sha256(), NULL) < 0) {
				return wlet(false, _T("Openssl sha256 init error"));
			}
			bool ret(do_openssl_HMACsha256(&ctx, data, data_size, temp_digest, &digest_size));
			HMAC_CTX_cleanup(&ctx);
			if (!ret) return false;
		}
		if (digest_size != 32) {
			return wlet(false, _T("Openssl sha256 digest size error"));
		}
		return true;
	}

#define RFC2409_PRIME_1024 \
            "FFFFFFFFFFFFFFFFC90FDAA22168C234C4C6628B80DC1CD1" \
            "29024E088A67CC74020BBEA63B139B22514A08798E3404DD" \
            "EF9519B3CD3A431B302B0A6DF25F14374FE1356D6D51C245" \
            "E485B576625E7EC6F44C42E9A637ED6B0BFF5CB6F406B7ED" \
            "EE386BFB5A899FA5AE9F24117C4B1FE649286651ECE65381" \
            "FFFFFFFFFFFFFFFF"
	SrsDH::SrsDH() {
		pdh = NULL;
	}
	SrsDH::~SrsDH() {
		close();
	}
	bool SrsDH::do_initialize() {
		int32_t bits_count = 1024;
		close();
		if ((pdh = DH_new()) == NULL) {
			return wlet(false, _T("Openssl create dh error"));
		}
		if ((pdh->p = BN_new()) == NULL) {
			return wlet(false, _T("Openssl create p error"));
		}
		if ((pdh->g = BN_new()) == NULL) {
			return wlet(false, _T("Openssl create g error"));
		}
		if (!BN_hex2bn(&pdh->p, RFC2409_PRIME_1024)) {
			return wlet(false, _T("Openssl parse prime 1024 error"));
		}
		if (!BN_set_word(pdh->g, 2)) {
			return wlet(false, _T("Openssl set word error"));
		}
		pdh->length = bits_count;
		if (!DH_generate_key(pdh)) {
			return wlet(false, _T("Openssl generate dh keys error"));
		}
		return true;
	}
	void SrsDH::close() {
		if (pdh != NULL) {
			if (pdh->p != NULL) {
				BN_free(pdh->p);
				pdh->p = NULL;
			}
			if (pdh->g != NULL) {
				BN_free(pdh->g);
				pdh->g = NULL;
			}
			DH_free(pdh);
			pdh = NULL;
		}
	}
	bool SrsDH::initialize(bool ensure_128bytes_public_key) {
		for (;;) {
			if (!do_initialize()) {
				return false;
			}
			if (ensure_128bytes_public_key) {
				int32_t key_size = BN_num_bytes(pdh->pub_key);
				if (key_size != 128) {
					wlw(_T("regenerate 128B key, current=%dB"), key_size);
					continue;
				}
			}
			break;
		}
		return true;
	}
	bool SrsDH::copy_public_key(char* pkey, int32_t& pkey_size) {
		int32_t key_size = BN_num_bytes(pdh->pub_key);
		assert(key_size > 0);
		// maybe the key_size is 127, but dh will write all 128bytes pkey,
		// so, donot need to set/initialize the pkey.
		// @see https://github.com/ossrs/srs/issues/165
		key_size = BN_bn2bin(pdh->pub_key, (unsigned char*)pkey);
		assert(key_size > 0);
		// output the size of public key.
		// @see https://github.com/ossrs/srs/issues/165
		assert(key_size <= pkey_size);
		pkey_size = key_size;
		return true;
	}
	bool SrsDH::copy_shared_key(const char* ppkey, int32_t ppkey_size, char* skey, int32_t& skey_size) {
		CSmartHdr<BIGNUM*, void> ppk(BN_free);
		if ((ppk = BN_bin2bn((const unsigned char*)ppkey, ppkey_size, 0)) == NULL) {
			return wlet(false, _T("Openssl get peer public key error"));
		}
		// if failed, donot return, do cleanup, @see ./test/dhtest.c:168
		// maybe the key_size is 127, but dh will write all 128bytes skey,
		// so, donot need to set/initialize the skey.
		// @see https://github.com/ossrs/srs/issues/165
		int32_t key_size = DH_compute_key((unsigned char*)skey, ppk, pdh);
		if (key_size < ppkey_size) {
			wlw(_T("shared key size=%d, ppk_size=%d"), key_size, ppkey_size);
		}
		if (key_size < 0 || key_size > skey_size) {
			return wlet(false, _T("Openssl compute shared key error"));
		}
		skey_size = key_size;
		return true;
	}

	key_block::key_block() {
		offset = (int32_t)rand();
		random0 = NULL;
		random1 = NULL;
		int valid_offset = calc_valid_offset();
		assert(valid_offset >= 0);
		random0_size = valid_offset;
		if (random0_size > 0) {
			random0 = new char[random0_size];
			utils::randomGenerate(random0, random0_size);
			snprintf(random0, random0_size, "%s", RTMP_SIG_SRS_HANDSHAKE);
		}
		utils::randomGenerate(key, sizeof(key));
		random1_size = 764 - valid_offset - 128 - 4;
		if (random1_size > 0) {
			random1 = new char[random1_size];
			utils::randomGenerate(random1, random1_size);
			snprintf(random1, random1_size, "%s", RTMP_SIG_SRS_HANDSHAKE);
		}
	}
	key_block::~key_block()	{
	}
	bool key_block::parse(CByteStream* stream) {
		// the key must be 764 bytes.
		assert(stream->require(764));
		// read the last offset first, 760-763
		stream->skip(764 - sizeof(int32_t));
		offset = stream->read_4bytes();
		// reset stream to read others.
		stream->skip(-764);
		int valid_offset = calc_valid_offset();
		assert(valid_offset >= 0);
		random0_size = valid_offset;
		if (random0_size > 0) {
			random0 = new char[random0_size];
			stream->read_bytes(random0, random0_size);
		}
		stream->read_bytes(key, 128);
		random1_size = 764 - valid_offset - 128 - 4;
		if (random1_size > 0) {
			random1 = new char[random1_size];
			stream->read_bytes(random1, random1_size);
		}
		return true;
	}
	int key_block::calc_valid_offset() {
		int max_offset_size = 764 - 128 - 4;
		int valid_offset = 0;
		uint8_t* pp = (uint8_t*)&offset;
		valid_offset += *pp++;
		valid_offset += *pp++;
		valid_offset += *pp++;
		valid_offset += *pp++;
		return valid_offset % max_offset_size;
	}

	digest_block::digest_block() {
		offset = (int32_t)rand();
		random0 = NULL;
		random1 = NULL;
		int valid_offset = calc_valid_offset();
		assert(valid_offset >= 0);
		random0_size = valid_offset;
		if (random0_size > 0) {
			random0 = new char[random0_size];
			utils::randomGenerate(random0, random0_size);
			snprintf(random0, random0_size, "%s", RTMP_SIG_SRS_HANDSHAKE);
		}
		utils::randomGenerate(digest, sizeof(digest));
		random1_size = 764 - 4 - valid_offset - 32;
		if (random1_size > 0) {
			random1 = new char[random1_size];
			utils::randomGenerate(random1, random1_size);
			snprintf(random1, random1_size, "%s", RTMP_SIG_SRS_HANDSHAKE);
		}
	}
	digest_block::~digest_block() {
	}
	bool digest_block::parse(CByteStream* stream) {
		// the digest must be 764 bytes.
		assert(stream->require(764));
		offset = stream->read_4bytes();
		int valid_offset = calc_valid_offset();
		assert(valid_offset >= 0);
		random0_size = valid_offset;
		if (random0_size > 0) {
			random0 = new char[random0_size];
			stream->read_bytes(random0, random0_size);
		}
		stream->read_bytes(digest, 32);
		random1_size = 764 - 4 - valid_offset - 32;
		if (random1_size > 0) {
			random1 = new char[random1_size];
			stream->read_bytes(random1, random1_size);
		}
		return true;
	}
	int digest_block::calc_valid_offset() {
		int max_offset_size = 764 - 32 - 4;
		int valid_offset = 0;
		uint8_t* pp = (uint8_t*)&offset;
		valid_offset += *pp++;
		valid_offset += *pp++;
		valid_offset += *pp++;
		valid_offset += *pp++;
		return valid_offset % max_offset_size;
	}

	c1s1_strategy::c1s1_strategy() {
	}
	c1s1_strategy::~c1s1_strategy() {
	}
	char* c1s1_strategy::get_digest() {
		return digest.digest;
	}
	char* c1s1_strategy::get_key() {
		return key.key;
	}
	bool c1s1_strategy::dump(c1s1* owner, char* _c1s1, int size) {
		assert(size == 1536);
		if (!copy_to(owner, _c1s1, size, true)) {
			return false;
		}
		return true;
	}
	bool c1s1_strategy::c1_create(c1s1* owner) {
		char* c1_digest = NULL;
		if (!calc_c1_digest(owner, c1_digest)) {
			return false;
		}
		assert(c1_digest != NULL);
		memcpy(digest.digest, c1_digest, 32);
		delete[] c1_digest;
		return true;
	}
	bool c1s1_strategy::c1_validate_digest(c1s1* owner, bool& is_valid) {
		char* c1_digest = NULL;
		if (!calc_c1_digest(owner, c1_digest)) {
			return false;
		}
		assert(c1_digest != NULL);
		is_valid = utils::isBytesEquals(digest.digest, c1_digest, 32);
		delete[] c1_digest;
		return true;
	}
	bool c1s1_strategy::s1_create(c1s1* owner, c1s1* c1) {
		SrsDH dh;
		// ensure generate 128bytes public key.
		if (!dh.initialize(true)) {
			return false;
		}
		// directly generate the public key.
		// @see: https://github.com/ossrs/srs/issues/148
		int pkey_size = 128;
		if (!dh.copy_shared_key(c1->get_key(), 128, key.key, pkey_size)) {
			return false;
		}
		// although the public key is always 128bytes, but the share key maybe not.
		// we just ignore the actual key size, but if need to use the key, must use the actual size.
		// TODO: FIXME: use the actual key size.
		//assert(pkey_size == 128);
		wld("calc s1 key success.");
		char* s1_digest = NULL;
		if (!calc_s1_digest(owner, s1_digest)) {
			return false;
		}
		wld("calc s1 digest success.");
		assert(s1_digest != NULL);
		memcpy(digest.digest, s1_digest, 32);
		delete[] s1_digest;
		wld("copy s1 key success.");
		return true;
	}
	bool c1s1_strategy::s1_validate_digest(c1s1* owner, bool& is_valid) {
		char* s1_digest = NULL;
		if (!calc_s1_digest(owner, s1_digest)) {
			return false;
		}
		assert(s1_digest != NULL);
		is_valid = utils::isBytesEquals(digest.digest, s1_digest, 32);
		delete[] s1_digest;
		return true;
	}
	bool c1s1_strategy::calc_c1_digest(c1s1* owner, char*& c1_digest) {
		/**
		* c1s1 is splited by digest:
		*     c1s1-part1: n bytes (time, version, key and digest-part1).
		*     digest-data: 32bytes
		*     c1s1-part2: (1536-n-32)bytes (digest-part2)
		* @return a new allocated bytes, user must free it.
		*/
		CSmartArr<char> c1s1_joined_bytes(new char[1536 - 32]);
		if (!copy_to(owner, c1s1_joined_bytes, 1536 - 32, false)) {
			return false;
		}
		c1_digest = new char[SRS_OpensslHashSize];
		if (!openssl_HMACsha256(SrsGenuineFPKey, 30, c1s1_joined_bytes, 1536 - 32, c1_digest)) {
			delete[] c1_digest;
			c1_digest = NULL;
			return false;
		}
		wld("digest calculated for c1");
		return true;
	}
	bool c1s1_strategy::calc_s1_digest(c1s1* owner, char*& s1_digest) {
		/**
		* c1s1 is splited by digest:
		*     c1s1-part1: n bytes (time, version, key and digest-part1).
		*     digest-data: 32bytes
		*     c1s1-part2: (1536-n-32)bytes (digest-part2)
		* @return a new allocated bytes, user must free it.
		*/
		CSmartArr<char> c1s1_joined_bytes(new char[1536 - 32]);
		if (!copy_to(owner, c1s1_joined_bytes, 1536 - 32, false)) {
			return false;
		}
		s1_digest = new char[SRS_OpensslHashSize];
		if (!openssl_HMACsha256(SrsGenuineFMSKey, 36, c1s1_joined_bytes, 1536 - 32, s1_digest)) {
			delete s1_digest;
			s1_digest = NULL;
			return false;
		}
		wld("digest calculated for s1");
		return true;
	}
	void c1s1_strategy::copy_time_version(CByteStream* stream, c1s1* owner) {
		assert(stream->require(8));
		// 4bytes time
		stream->write_4bytes(owner->time);
		// 4bytes version
		stream->write_4bytes(owner->version);
	}
	void c1s1_strategy::copy_key(CByteStream* stream) {
		assert(key.random0_size >= 0);
		assert(key.random1_size >= 0);
		int total = key.random0_size + 128 + key.random1_size + 4;
		assert(stream->require(total));
		// 764bytes key block
		if (key.random0_size > 0) {
			stream->write_bytes(key.random0, key.random0_size);
		}
		stream->write_bytes(key.key, 128);
		if (key.random1_size > 0) {
			stream->write_bytes(key.random1, key.random1_size);
		}
		stream->write_4bytes(key.offset);
	}
	void c1s1_strategy::copy_digest(CByteStream* stream, bool with_digest) {
		assert(key.random0_size >= 0);
		assert(key.random1_size >= 0);
		int total = 4 + digest.random0_size + digest.random1_size;
		if (with_digest) {
			total += 32;
		}
		assert(stream->require(total));
		// 732bytes digest block without the 32bytes digest-data
		// nbytes digest block part1
		stream->write_4bytes(digest.offset);
		// digest random padding.
		if (digest.random0_size > 0) {
			stream->write_bytes(digest.random0, digest.random0_size);
		}
		// digest
		if (with_digest) {
			stream->write_bytes(digest.digest, 32);
		}
		// nbytes digest block part2
		if (digest.random1_size > 0) {
			stream->write_bytes(digest.random1, digest.random1_size);
		}
	}

	c1s1_strategy_schema0::c1s1_strategy_schema0() {
	}
	c1s1_strategy_schema0::~c1s1_strategy_schema0() {
	}
	srs_schema_type c1s1_strategy_schema0::schema() {
		return srs_schema0;
	}
	bool c1s1_strategy_schema0::parse(char* _c1s1, int size) {
		assert(size == 1536);
		CByteStream stream;
		if (!stream.initialize(_c1s1 + 8, 764)) {
			return false;
		}
		if (!key.parse(&stream)) {
			return false;
		}
		if (!stream.initialize(_c1s1 + 8 + 764, 764)) {
			return false;
		}
		if (!digest.parse(&stream)) {
			return false;
		}
		wld("parse c1 key-digest success");
		return true;
	}
	bool c1s1_strategy_schema0::copy_to(c1s1* owner, char* bytes, int size, bool with_digest) {
		if (with_digest) {
			assert(size == 1536);
		} else {
			assert(size == 1504);
		}
		CByteStream stream;
		if (!stream.initialize(bytes, size)) {
			return wlet(false, _T("initialize byte stream failed."));
		}
		copy_time_version(&stream, owner);
		copy_key(&stream);
		copy_digest(&stream, with_digest);
		assert(stream.empty());
		return true;
	}

	c1s1_strategy_schema1::c1s1_strategy_schema1() {
	}
	c1s1_strategy_schema1::~c1s1_strategy_schema1() {
	}
	srs_schema_type c1s1_strategy_schema1::schema() {
		return srs_schema1;
	}
	bool c1s1_strategy_schema1::parse(char* _c1s1, int size) {
		assert(size == 1536);
		CByteStream stream;
		if (!stream.initialize(_c1s1 + 8, 764)) {
			return false;
		}
		if (!digest.parse(&stream)) {
			return false;
		}
		if (!stream.initialize(_c1s1 + 8 + 764, 764)) {
			return false;
		}
		if (!key.parse(&stream)) {
			return false;
		}
		wld("parse c1 digest-key success");
		return true;
	}
	bool c1s1_strategy_schema1::copy_to(c1s1* owner, char* bytes, int size, bool with_digest) {
		if (with_digest) {
			assert(size == 1536);
		} else {
			assert(size == 1504);
		}
		CByteStream stream;
		if (!stream.initialize(bytes, size)) {
			return false;
		}
		copy_time_version(&stream, owner);
		copy_digest(&stream, with_digest);
		copy_key(&stream);
		assert(stream.empty());
		return true;
	}

	c1s1::c1s1() {
		payload = NULL;
	}
	c1s1::~c1s1() {
	}
	srs_schema_type c1s1::schema() {
		assert(payload != NULL);
		return payload->schema();
	}
	char* c1s1::get_digest() {
		assert(payload != NULL);
		return payload->get_digest();
	}
	char* c1s1::get_key() {
		assert(payload != NULL);
		return payload->get_key();
	}
	bool c1s1::dump(char* _c1s1, int size) {
		assert(size == 1536);
		assert(payload != NULL);
		return payload->dump(this, _c1s1, size);
	}
	bool c1s1::parse(char* _c1s1, int size, srs_schema_type schema) {
		assert(size == 1536);
		if (schema != srs_schema0 && schema != srs_schema1) {
			return wlet(false, _T("parse c1 failed. invalid schema=%d"), schema);
		}
		CByteStream stream;
		if (!stream.initialize(_c1s1, size)) {
			return false;
		}
		time = stream.read_4bytes();
		version = stream.read_4bytes(); // client c1 version
		if (schema == srs_schema0) {
			payload = new c1s1_strategy_schema0();
		} else {
			payload = new c1s1_strategy_schema1();
		}
		return payload->parse(_c1s1, size);
	}
	bool c1s1::c1_create(srs_schema_type schema) {
		if (schema != srs_schema0 && schema != srs_schema1) {
			return wlet(false, _T("create c1 failed. invalid schema=%d"), schema);
		}
		// client c1 time and version
		time = (int32_t)::time(NULL);
		version = 0x80000702; // client c1 version
		if (schema == srs_schema0) {
			payload = new c1s1_strategy_schema0();
		} else {
			payload = new c1s1_strategy_schema1();
		}
		return payload->c1_create(this);
	}
	bool c1s1::c1_validate_digest(bool& is_valid) {
		is_valid = false;
		assert(payload);
		return payload->c1_validate_digest(this, is_valid);
	}
	bool c1s1::s1_create(c1s1* c1) {
		if (c1->schema() != srs_schema0 && c1->schema() != srs_schema1) {
			return wlet(false, _T("create s1 failed. invalid schema=%d"), c1->schema());
		}
		time = (uint32_t)::time(NULL);
		version = 0x01000504; // server s1 version
		if (c1->schema() == srs_schema0) {
			payload = new c1s1_strategy_schema0();
		} else {
			payload = new c1s1_strategy_schema1();
		}
		return payload->s1_create(this, c1);
	}
	bool c1s1::s1_validate_digest(bool& is_valid) {
		is_valid = false;
		assert(payload);
		return payload->s1_validate_digest(this, is_valid);
	}

	c2s2::c2s2() {
		utils::randomGenerate(random, 1504);
		int size = snprintf(random, 1504, "%s", RTMP_SIG_SRS_HANDSHAKE);
		assert(++size < 1504);
		snprintf(random + 1504 - size, size, "%s", RTMP_SIG_SRS_HANDSHAKE);
		utils::randomGenerate(digest, 32);
	}
	c2s2::~c2s2() {
	}
	bool c2s2::dump(char* _c2s2, int size) {
		assert(size == 1536);
		memcpy(_c2s2, random, 1504);
		memcpy(_c2s2 + 1504, digest, 32);
		return true;
	}
	bool c2s2::parse(char* _c2s2, int size) {
		assert(size == 1536);
		memcpy(random, _c2s2, 1504);
		memcpy(digest, _c2s2 + 1504, 32);
		return true;
	}
	bool c2s2::c2_create(c1s1* s1) {
		char temp_key[SRS_OpensslHashSize];
		if (!openssl_HMACsha256(SrsGenuineFPKey, 62, s1->get_digest(), 32, temp_key)) {
			return false;
		}
		wld("generate c2 temp key success.");
		char _digest[SRS_OpensslHashSize];
		if (!openssl_HMACsha256(temp_key, 32, random, 1504, _digest)) {
			return false;
		}
		wld("generate c2 digest success.");
		memcpy(digest, _digest, 32);
		return true;
	}
	bool c2s2::c2_validate(c1s1* s1, bool& is_valid) {
		is_valid = false;
		char temp_key[SRS_OpensslHashSize];
		if (!openssl_HMACsha256(SrsGenuineFPKey, 62, s1->get_digest(), 32, temp_key)) {
			return false;
		}
		wld("generate c2 temp key success.");
		char _digest[SRS_OpensslHashSize];
		if (!openssl_HMACsha256(temp_key, 32, random, 1504, _digest)) {
			return false;
		}
		wld("generate c2 digest success.");
		is_valid = utils::isBytesEquals(digest, _digest, 32);
		return true;
	}
	bool c2s2::s2_create(c1s1* c1) {
		char temp_key[SRS_OpensslHashSize];
		if (!openssl_HMACsha256(SrsGenuineFMSKey, 68, c1->get_digest(), 32, temp_key)) {
			return false;
		}
		wld("generate s2 temp key success.");
		char _digest[SRS_OpensslHashSize];
		if (!openssl_HMACsha256(temp_key, 32, random, 1504, _digest)) {
			return false;
		}
		wld("generate s2 digest success.");
		memcpy(digest, _digest, 32);
		return true;
	}
	bool c2s2::s2_validate(c1s1* c1, bool& is_valid) {
		is_valid = false;
		char temp_key[SRS_OpensslHashSize];
		if (!openssl_HMACsha256(SrsGenuineFMSKey, 68, c1->get_digest(), 32, temp_key)) {
			return false;
		}
		wld("generate s2 temp key success.");
		char _digest[SRS_OpensslHashSize];
		if (!openssl_HMACsha256(temp_key, 32, random, 1504, _digest)) {
			return false;
		}
		wld("generate s2 digest success.");
		is_valid = utils::isBytesEquals(digest, _digest, 32);
		return true;
	}
}



