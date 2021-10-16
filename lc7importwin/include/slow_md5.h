#ifndef __INC_SLOW_MD5_H
#define __INC_SLOW_MD5_H

/* Any 32-bit or wider unsigned integer data type will do */
typedef quint32 MD5_u32plus;
 
typedef struct 
{
	MD5_u32plus lo, hi;
	MD5_u32plus a, b, c, d;
	unsigned char buffer[64];
	MD5_u32plus block[16];
} MD5_CTX;
 
extern void MD5_Init(MD5_CTX *ctx);
extern void MD5_Update(MD5_CTX *ctx, const void *data, size_t size);
extern void MD5_Final(unsigned char *result, MD5_CTX *ctx);

#endif // __INC_SLOW_MD5_H
