

#ifndef __MD5_H
#define __MD5_H


#define MD5_API


#ifdef __cplusplus
extern "C" {
#endif


// size of an MD5 digest

#define MD5_DIGESTSIZE  16



// MD5 context

typedef struct
{
  unsigned int state[4];  	// state (ABCD)
  unsigned int count[2];  	// number of bits, modulo 2^64 (lsb first)
  unsigned char buffer[64]; 	// input buffer
}
MD5CTX, *PMD5CTX;




/*
 * initialization
 * -> pointer to MD5 context
 */
void MD5_API 
    MD5_Initialize (PMD5CTX);


/*
 * resets an MD5 context
 * -> context to rest
 */
void MD5_API 
    MD5_Reset (PMD5CTX);



/*
 * block update operation, continues an MD5 message-digest operation,
 * processing another message block, and updating the context
 * -> pointer to MD5 context
 * -> pointer to input buffer, which is treated as a byte buffer
 * -> number of bytes to scramble
 */
void MD5_API 
    MD5_Update (PMD5CTX, const void*, unsigned int);



/*
 * MD5 finalization, ends an MD5 message-digest operation,
 * writing the message digest and clearing the context
 * -> buffer where to copy the digest's bytes
 * -> pointer to MD5 context
 */
void MD5_API 
    MD5_Final (unsigned char[MD5_DIGESTSIZE], PMD5CTX);


/*
 * selftest
 * <- 1: selftest succeeded / 0: error
 */
int MD5_API 
    MD5_SelfTest();


#ifdef __cplusplus
}
#endif


#endif
