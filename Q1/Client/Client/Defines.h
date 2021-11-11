#pragma once

#include <stdint.h>

static constexpr uint8_t CLIENT_VERSION = 1;

static constexpr size_t PUBLIC_KEY_LENGTH = 160;
static constexpr size_t SYM_KEY_LENGTH = 16;

// The current RSA encryption uses modulu with 1024 bits,
// thus the output should be 128 bytes.
static constexpr size_t ENCRYPTED_SYM_KEY_LENGTH = 128;

typedef uint8_t publicKey_t[PUBLIC_KEY_LENGTH];
