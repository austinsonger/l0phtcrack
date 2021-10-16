#include<stdafx.h>
#include"slow_rc4.h"

static void swap_bytes(quint8 *a, quint8 *b)
{
	unsigned char temp;

	temp = *a;
	*a = *b;
	*b = temp;
}

/*
 * Initialize an RC4 state buffer using the supplied key,
 * which can have arbitrary length.
 */
void rc4_init(struct rc4_state *const state, const quint8 *key, size_t keylen)
{
	quint8 j;
	int i;

	/* Initialize state with identity permutation */
	for (i = 0; i < 256; i++)
		state->perm[i] = (quint8)i;
	state->index1 = 0;
	state->index2 = 0;
  
	/* Randomize the permutation using key data */
	for (j = i = 0; i < 256; i++) 
	{
		j += state->perm[i] + key[i % keylen]; 
		swap_bytes(&state->perm[i], &state->perm[j]);
	}
}

/*
 * Encrypt some data using the supplied RC4 state buffer.
 * The input and output buffers may be the same buffer.
 * Since RC4 is a stream cypher, this function is used
 * for both encryption and decryption.
 */
void rc4_crypt(struct rc4_state *const state, const quint8 *inbuf, quint8 *outbuf, size_t buflen)
{
	int i;
	quint8 j;

	for (i = 0; i < buflen; i++) 
	{
		/* Update modification indicies */
		state->index1++;
		state->index2 += state->perm[state->index1];

		/* Modify permutation */
		swap_bytes(&state->perm[state->index1],
		    &state->perm[state->index2]);

		/* Encrypt/decrypt next byte */
		j = state->perm[state->index1] + state->perm[state->index2];
		outbuf[i] = inbuf[i] ^ state->perm[j];
	}
}

