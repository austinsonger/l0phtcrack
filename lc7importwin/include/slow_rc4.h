#ifndef __INC_SLOW_RC4_H
#define __INC_SLOW_RC4_H


struct rc4_state {
	quint8 perm[256];
	quint8 index1;
	quint8 index2;
};

void rc4_init(struct rc4_state *const state, const quint8 *key, size_t keylen);
void rc4_crypt(struct rc4_state *const state, const quint8 *inbuf, quint8 *outbuf, size_t buflen);

#endif