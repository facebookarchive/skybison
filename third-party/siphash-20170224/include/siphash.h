#ifndef SIPHASH_H
#define SIPHASH_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

int halfsiphash(const uint8_t *in, size_t inlen, const uint8_t *k,
                uint8_t *out, size_t outlen);

int siphash(const uint8_t *in, size_t inlen, const uint8_t *k,
            uint8_t *out, size_t outlen);

#ifdef __cplusplus
};
#endif

#endif /* SIPHASH_H */
