#ifndef _MBEDTLS_BASE64_H_
#define _MBEDTLS_BASE64_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int mbedtls_base64_encode(unsigned char *dst, size_t dlen, size_t *olen,
                         const unsigned char *src, size_t slen);

int mbedtls_base64_decode(unsigned char *dst, size_t dlen, size_t *olen,
                         const unsigned char *src, size_t slen);

#ifdef __cplusplus
}
#endif

#endif // _MBEDTLS_BASE64_H_
