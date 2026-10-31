// Netplay crypto behind a small interface (docs/netplay.md "Security"): Monocypher 4.0.3 (src/third_party/monocypher,
// CC0 / BSD-2-Clause) on every system, so Windows, Linux and macOS speak the same protocol (cross-play). Random bytes
// from the OS (Windows BCryptGenRandom, Linux getrandom, macOS arc4random_buf). Only the library's documented
// high-level functions: X25519, Elligator 2 (crypto_elligator_map), XChaCha20-Poly1305 (crypto_aead_lock / unlock),
// BLAKE2b, crypto_verify32, crypto_wipe.
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

namespace hotd::net::crypto {

constexpr size_t kPubSize = 32;     // an X25519 public key (a Curve25519 point's u coordinate, little-endian)
constexpr size_t kKeySize = 32;     // XChaCha20-Poly1305
constexpr size_t kNonceSize = 24;
constexpr size_t kTagSize = 16;

bool available();
bool random(void* out, size_t n);
// BLAKE2b, outLen 1..64; key: 0..64 bytes (keyLen 0: unkeyed)
void blake2b(uint8_t* out, size_t outLen, const void* key, size_t keyLen, const void* data, size_t n);
// wipe a secret (crypto_wipe: not optimised away)
void wipe(void* p, size_t n);

// CPace (a PAKE: the CFRG draft's X25519 variant) on an ephemeral X25519 key pair with the generator
// G = Elligator 2 (BLAKE2b-256 ("CPace255" || channel || password)): public = X25519 (s, G); the shared secret
// K = X25519 (s, the peer's public). A password guess can only be tested in a live handshake (one guess each); an
// eavesdropper or a fake host gets nothing to test guesses against offline. The secret scalar is wiped on destruction.
class Cpace {
public:
    virtual ~Cpace() = default;
    virtual const uint8_t* publicKey() const = 0;   // kPubSize bytes
    // the session keys: BLAKE2b-512 (label || K || transcript): out[0..31] client to host, out[32..63] host to
    // client. False: K is all zero (a low-order public key from the peer: refused, the draft's identity check)
    virtual bool derive(const uint8_t peer[kPubSize], const void* transcript, size_t n, uint8_t out[64]) = 0;
};
// password: the normalised session code; channel: a label both sides must share (the protocol's). Null: no OS
// randomness
std::unique_ptr<Cpace> makeCpace(const void* password, size_t passwordLen, const char* channel);

// XChaCha20-Poly1305 with a 24-byte nonce and a 16-byte tag; the key is wiped on destruction
class Aead {
public:
    virtual ~Aead() = default;
    // out: n + kTagSize bytes (the ciphertext, then the tag)
    virtual bool seal(const uint8_t nonce[kNonceSize], const void* aad, size_t aadLen, const void* plain, size_t n, uint8_t* out) = 0;
    // in: n bytes (ciphertext + tag, n >= kTagSize); out: n - kTagSize bytes. False: not authentic
    virtual bool open(const uint8_t nonce[kNonceSize], const void* aad, size_t aadLen, const void* in, size_t n, uint8_t* out) = 0;
};
std::unique_ptr<Aead> makeAead(const uint8_t key[kKeySize]);

// the known-answer tests: Monocypher's own test vectors (crypto_kat.inc) for X25519, Elligator 2, BLAKE2b and
// XChaCha20-Poly1305, each checked both ways where it applies. Returns the number of failures
int selfTest(void (*report)(bool ok, const char* what));

} // namespace hotd::net::crypto
