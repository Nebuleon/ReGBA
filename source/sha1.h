#ifndef __SHA1_H__
#define __SHA1_H__

#define SHA1_HASH_LENGTH 20
#define SHA1_BLOCK_LENGTH 64

union _buffer {
	uint8_t b[SHA1_BLOCK_LENGTH];
	uint32_t w[SHA1_BLOCK_LENGTH/4];
};

union _state {
	uint8_t b[SHA1_HASH_LENGTH];
	uint32_t w[SHA1_HASH_LENGTH/4];
};

typedef struct sha1nfo {
	union _buffer buffer;
	uint8_t bufferOffset;
	union _state state;
	uint32_t byteCount;
	uint8_t keyBuffer[SHA1_BLOCK_LENGTH];
	uint8_t innerHash[SHA1_HASH_LENGTH];
} sha1nfo;

/**
 */
void sha1_init(sha1nfo *s);
/**
 */
void sha1_writebyte(sha1nfo *s, uint8_t data);
/**
 */
void sha1_write(sha1nfo *s, const uint8_t *data, size_t len);
/**
 */
uint8_t* sha1_result(sha1nfo *s);
/**
 */
void sha1_initHmac(sha1nfo *s, const uint8_t* key, int keyLength);
/**
 */
uint8_t* sha1_resultHmac(sha1nfo *s);

#endif // !defined __SHA1_H__
