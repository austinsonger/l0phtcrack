#ifndef __INC_SLOW_DES_H
#define __INC_SLOW_DES_H

void slowdes_init(void);

struct SLOWDES_CTX
{
	quint8 kn[16][8];              // 8 6-bit subkeys for each of 16 rounds,  initialized by setkey()
};

void slowdes_round(SLOWDES_CTX *fdctx, quint32 num, quint32 *block);
void slowdes_endes(SLOWDES_CTX *ctx, quint8 block[8]);
void slowdes_dedes(SLOWDES_CTX *ctx, quint8 block[8]);
void slowdes_setkey(SLOWDES_CTX *ctx, quint8 *key);

void slowdes_permute(const quint8 inblock[8], const quint8 perm[16][16][8], quint8 outblock[8]);
void slowdes_str_to_key(const char *str, quint8 key[8]);

#endif