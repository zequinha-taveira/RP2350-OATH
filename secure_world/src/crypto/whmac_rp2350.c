#include "cotp.h"
#include "sha1.h"
#include "sha256.h"
#include "whmac.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Max number of simultaneous HMAC handles in secure world
#define MAX_WHMAC_HANDLES 4

struct whmac_handle_s {
  int algo;
  size_t dlen;
  size_t blklen;
  bool in_use;

  // Union of contexts
  union {
    SHA1_CTX sha1;
    SHA256_CTX sha256;
  } ctx;

  // Key pads
  uint8_t k_opad[128]; // Max block size
};

static whmac_handle_t handle_pool[MAX_WHMAC_HANDLES];

int whmac_check(void) {
  return 0; // Success
}

whmac_handle_t *whmac_gethandle(int algo) {
  if (algo != SHA1 && algo != SHA256) {
    return NULL; // SHA512 not supported
  }

  for (int i = 0; i < MAX_WHMAC_HANDLES; i++) {
    if (!handle_pool[i].in_use) {
      whmac_handle_t *hd = &handle_pool[i];
      memset(hd, 0, sizeof(whmac_handle_t));
      hd->in_use = true;
      hd->algo = algo;
      if (algo == SHA1) {
        hd->dlen = 20;
        hd->blklen = 64;
      } else {
        hd->dlen = 32;
        hd->blklen = 64;
      }
      return hd;
    }
  }

  return NULL; // No free handles
}

void whmac_freehandle(whmac_handle_t *hd) {
  if (hd) {
    memset(hd, 0, sizeof(whmac_handle_t));
    hd->in_use = false;
  }
}

size_t whmac_getlen(whmac_handle_t *hd) { return hd->dlen; }

int whmac_setkey(whmac_handle_t *hd, const unsigned char *key, size_t keylen) {
  if (!hd)
    return -1;
  uint8_t k[128]; // Working key buffer
  memset(k, 0, sizeof(k));

  // Hash key if longer than block size
  if (keylen > hd->blklen) {
    if (hd->algo == SHA1) {
      SHA1_CTX tctx;
      SHA1Init(&tctx);
      SHA1Update(&tctx, key, keylen);
      SHA1Final(k, &tctx);
      keylen = 20;
    } else {
      SHA256_CTX tctx;
      SHA256Init(&tctx);
      SHA256Update(&tctx, key, keylen);
      SHA256Final(&tctx, k);
      keylen = 32;
    }
  } else {
    memcpy(k, key, keylen);
  }

  // Compute ipad and opad
  uint8_t k_ipad[128];
  memset(k_ipad, 0x36, hd->blklen);
  memset(hd->k_opad, 0x5C, hd->blklen);

  for (size_t i = 0; i < hd->blklen; i++) {
    k_ipad[i] ^= k[i];
    hd->k_opad[i] ^= k[i];
  }

  // Initialize Inner Hash
  if (hd->algo == SHA1) {
    SHA1Init(&hd->ctx.sha1);
    SHA1Update(&hd->ctx.sha1, k_ipad, hd->blklen);
  } else {
    SHA256Init(&hd->ctx.sha256);
    SHA256Update(&hd->ctx.sha256, k_ipad, hd->blklen);
  }

  // Clean up stack key
  memset(k, 0, sizeof(k));
  memset(k_ipad, 0, sizeof(k_ipad));

  return 0; // NO_ERROR
}

void whmac_update(whmac_handle_t *hd, const unsigned char *buffer,
                  size_t buflen) {
  if (!hd)
    return;
  if (hd->algo == SHA1) {
    SHA1Update(&hd->ctx.sha1, buffer, (uint32_t)buflen);
  } else {
    SHA256Update(&hd->ctx.sha256, buffer, buflen);
  }
}

ssize_t whmac_finalize(whmac_handle_t *hd, unsigned char *buffer,
                       size_t buflen) {
  if (!hd || buflen < hd->dlen)
    return -1;

  uint8_t inner_hash[64]; // Max digest size

  // Finish Inner Hash
  if (hd->algo == SHA1) {
    SHA1Final(inner_hash, &hd->ctx.sha1);
  } else {
    SHA256Final(&hd->ctx.sha256, inner_hash);
  }

  // Outer Hash
  if (hd->algo == SHA1) {
    SHA1_CTX ctx;
    SHA1Init(&ctx);
    SHA1Update(&ctx, hd->k_opad, hd->blklen);
    SHA1Update(&ctx, inner_hash, 20);
    SHA1Final(buffer, &ctx);
  } else {
    SHA256_CTX ctx;
    SHA256Init(&ctx);
    SHA256Update(&ctx, hd->k_opad, hd->blklen);
    SHA256Update(&ctx, inner_hash, 32);
    SHA256Final(&ctx, buffer);
  }

  return (ssize_t)hd->dlen;
}
