// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/common/sha1.h"

#include <string>

#include "clay/fml/hex_codec.h"

#ifdef OS_IOS
#include <openssl/sha.h>
#else
#include <cstdlib>

#include "third_party/boringssl/src/include/openssl/sha.h"
#endif
#ifndef _WIN32
#define __WEAK __attribute__((__weak__))
extern "C" {
// cSpell:disable
__WEAK void CRYPTO_init_sysrand(void) {}
__WEAK uint64_t CRYPTO_get_fork_generation(void) { return 0; }
__WEAK void *OPENSSL_malloc(size_t size) { return malloc(size); }
__WEAK void *OPENSSL_realloc(void *ptr, size_t size) {
  return realloc(ptr, size);
}
__WEAK void OPENSSL_free(void *ptr) { free(ptr); }
__WEAK void OPENSSL_cleanse(void *ptr, size_t len) {}
__WEAK void CRYPTO_once(int32_t *once, void (*init)(void)) {
  if (!*once) {
    *once = 1;
    init();
  }
}
}
#endif

namespace clay {

std::string SHA1HashBytes(const void *data, size_t size) {
  if (data == nullptr || size == 0) {
    return "";
  }

  uint8_t sha_digest[SHA_DIGEST_LENGTH];
  SHA1(static_cast<const uint8_t *>(data), size, sha_digest);

  std::string_view view(reinterpret_cast<const char *>(sha_digest),
                        SHA_DIGEST_LENGTH);
  return fml::HexEncode(view);
}

}  // namespace clay
