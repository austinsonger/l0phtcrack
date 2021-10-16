#ifdef QT_CORE_LIB
#include<QtCore/qcoreapplication.h>
#else
#include<Windows.h>
#endif

#include <iostream>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/err.h>
#include <random>
#include <time.h>
#include "PubKeyFile.h"


const char *pubkey = R"(-----BEGIN PUBLIC KEY-----
MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAp+bLujF7Hgnhhng55dMj
lYTrDYf6MxejVNf/X2qyRKGgUnoO42PtATMRJ5dz7L92dR2AB7h3m5bsar9ajIT+
/pOcbH2k7Ufq4n1TRF9H8yrcB+Hq+U6gyJP5dXlq3lkVBTpOC80eN6pLNXfpw/sN
3AdgkUIRxa65d5+imLA6mtq2y8lAFqXhpdtY9/tSgPjdE75rD1U6/5d73Dfx8Zot
0+oZ7MASt+OoJo2hCoPx8yXssXcKT1hDPZzdzIIFDLursSp+aD8Nj8xiZO9CWpsu
0EmwA63MGzOrC/FyRHAl8TEUyecQESUZij1Ws2XJ2mZqNGKwmr3EZ0u75Gtdp4j6
DQIDAQAB
-----END PUBLIC KEY-----)";
const char *privkey = R"(-----BEGIN RSA PRIVATE KEY-----
MIIEogIBAAKCAQEAp+bLujF7Hgnhhng55dMjlYTrDYf6MxejVNf/X2qyRKGgUnoO
42PtATMRJ5dz7L92dR2AB7h3m5bsar9ajIT+/pOcbH2k7Ufq4n1TRF9H8yrcB+Hq
+U6gyJP5dXlq3lkVBTpOC80eN6pLNXfpw/sN3AdgkUIRxa65d5+imLA6mtq2y8lA
FqXhpdtY9/tSgPjdE75rD1U6/5d73Dfx8Zot0+oZ7MASt+OoJo2hCoPx8yXssXcK
T1hDPZzdzIIFDLursSp+aD8Nj8xiZO9CWpsu0EmwA63MGzOrC/FyRHAl8TEUyecQ
ESUZij1Ws2XJ2mZqNGKwmr3EZ0u75Gtdp4j6DQIDAQABAoIBAGlIYMFtW51XZtKS
OwPxJKQSR+4hMPOuw91k4B5t4kEE9GojXYia1TfmVMB1yNxQprADgo/UzTOELNU3
DkmE7/ykhPbGxo9/hDrniTobYRuK6ZQLxvLzqgB8QXW3YBry0yqHB/+AbI1NPIHX
on8gwu0Qty3+tvQ1ftkTAbsq7mTA74vYWwem4VaAIH8ZRhMwItGAEcsSJQjsq9LN
IuKf8QcAx20nLYC2j+YFhkB2k6PS/rsGQq+FxL1vmT3q/6rTsmy9a+KwMX3W61E2
3t/jXgPh/YtH0sfP9HwDTIIXKX5+OTiazHa/7ZVK0BZ6+a0EXNFbDjPDoA1SpjS8
UPg1/EUCgYEA8vUHvWaP1mezWKeTTK0p4CrMyWM4PHXS0A7XDlOVkUdrraQff7sC
/1/Gv5qCdqo+csqghptqwNp05NLe+CrUxB0f5E70ustj7Wjl2StNm1pMmtIe48AJ
5E5cVDKL6SAOaTk7l6v65ZpCZj93aqwI+lROB0N4t56HzuQ/2hzI998CgYEAsOpF
/+ZtI1+WQBihDW5BIvyZOjGZQ+pSE0uzK36N1h80i4f4j7MG3fXE9DnF6i3lP7GF
HaWVr0T1tYAkYG1KUBmNZfBTKWwKXCDLsepSl2JHTeLj5Vl7ngMyudqgMheJUjXv
/nf2g9EHnr7Nf3fvTREZ5zm+PTmTnoNgxXpF+5MCgYACfM3Ce61XVaJ5cuXIMDGw
kN+d5D0sKUyqQqyjAeoTHbcVhQuphCGoubNQPOW/D0kqZba1ChxSsynlOd98jPu2
slGkRcatru1r/dn0mkPmJkuIfkRAsrOnOcIqPoY+MczT+PkKOYGNHkI1x8qSNga1
gW13F5LB1oavcJjSw96u4wKBgGiB29vxCKOOwbkXU3wVl1DNms5vojQmZVmUrECT
P8IvKOEL718jyHixyCjlP+64MeZQPWxn9mZU/Di9e7+ij/WFQqvnLZkYg34suWde
Hm/i1HBECY/gLlAHX52+ckdk8++mgSD3SJ5RiojR+Z7FAI2r/ntC9wg/zN4kibPe
KR0VAoGAD0uSzhLIrO7aRDQu3XjTDHuko1asbsa8qMq7Fsq9JUr1tvDGm0WpDBOs
hzVqBiGTMeTPfSM6Vkfsib9vIUL+nP0xZUquDGG235eToH78eQW7vB2QNQQbmrhl
cnOmfEAz3YIhlMgNPzjirgeErVdK0S9LnEEj0jR5LJbJzO+XR9I=
-----END RSA PRIVATE KEY-----)";


void *random_buffer(uint32_t n) {
	
	time_t t;
	time(&t);
	srand((unsigned int)t);
	uint8_t *out = (uint8_t *)malloc((size_t)n);
	for (uint32_t i = 0; i < n; i++) {
		out[i] = (uint8_t)rand();
	}
	return out;
}

void test_size(uint32_t n)
{
	std::cout << "  Size " << n << "\n";

	// Generate data
	void *data = random_buffer(n);
	void *data2 = malloc(n);

	// Write file
	CPubKeyFile w;
	w.SetKey(pubkey, true);
#ifdef QT_CORE_LIB
	if (!w.Open("foo.bar", true)) {
#else
	if (!w.Open(L"foo.bar", true, NULL)) {
#endif
		std::cout << "failed to open write file\n";
		return;
	}
	U32 written;
	if (!w.Write(data, n, &written)) {
		std::cout << "failed to write file\n";
		return;
	}
	if (written != n) {
		std::cout << "failed to write entire file\n";
		return;
	}
	if (!w.Close()) {
		std::cout << "failed to close write file\n";
		return;
	}

	// Read file
	CPubKeyFile r;
	r.SetKey(privkey, false);
#ifdef QT_CORE_LIB
	if (!r.Open("foo.bar", false)) {
#else
	if (!r.Open(L"foo.bar", false, NULL)) {
#endif
		std::cout << "failed to open read file\n";
		return;
	}
	U32 read;
	if (!r.Read(data2, n, &read)) {
		std::cout << "failed to read file\n";
		return;
	}
	if (read != n) {
		std::cout << "failed to read entire file\n";
		return;
	}
	if (!r.Close()) {
		std::cout << "failed to close read file\n";
		return;
	}

	// Compare data
	if (memcmp(data, data2, n) != 0) {
		std::cout << "data was not the same\n";
		return;
	}
	free(data);
	free(data2);

	// Delete file
	_wunlink(L"foo.bar");

}

int main(int argc, char *argv[])
{
#ifdef QT_CORE_LIB
	QCoreApplication app(argc, argv);
#endif

	OpenSSL_add_all_ciphers();
	ERR_load_crypto_strings();

    std::cout << "Starting testing PubKeyFile\n";
	test_size(0);
	test_size(1);
	test_size(128);
	test_size(65534);
	test_size(65535);
	test_size(65536);
	test_size(150000);
	test_size(2000000);
	std::cout << "End testing PubKeyFile\n";
}
