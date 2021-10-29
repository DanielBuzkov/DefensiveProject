#pragma once

#include <stdint.h>

static constexpr uint8_t CLIENT_VERSION = 1;

static constexpr size_t PUBLIC_KEY_LENGTH = 160;
static constexpr size_t SYM_KEY_LENGTH = 16;

typedef uint8_t publicKey_t[PUBLIC_KEY_LENGTH];

typedef uint8_t symkey_t[SYM_KEY_LENGTH];
