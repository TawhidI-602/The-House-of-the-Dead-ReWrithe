// Netplay crypto on Monocypher (every system: cross-play), docs/netplay.md "Security". Random bytes from the OS.
#include "net/crypto.h"

#include "third_party/monocypher/monocypher.h"

#include <cstring>
#include <string>
#include <vector>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <bcrypt.h>
#pragma comment(lib, "bcrypt.lib")
#elif defined(__APPLE__)
#include <stdlib.h>   // arc4random_buf
#elif defined(__linux__)
#include <cerrno>
#include <sys/random.h>
#else
#include <unistd.h>   // getentropy
#endif

namespace hotd::net::crypto {

namespace {
// the shared secret's KDF label (in the BLAKE2b input before K and the transcript)
constexpr char kKeyLabel[] = "HOTD ReWrithe netplay v10 keys";

class MonoCpace final : public Cpace {
public:
    uint8_t secret[32] = {};
    uint8_t pub[kPubSize] = {};
    ~MonoCpace() override { crypto_wipe(secret, sizeof secret); }
    const uint8_t* publicKey() const override { return pub; }
    bool derive(const uint8_t peer[kPubSize], const void* transcript, size_t n, uint8_t out[64]) override
    {
        uint8_t k[32];
        crypto_x25519(k, secret, peer);
        static const uint8_t kZero[32] = {};
        if (crypto_verify32(k, kZero) == 0) {   // a low-order point from the peer: refuse (contributory behaviour)
            crypto_wipe(k, sizeof k);
            return false;
        }
        crypto_blake2b_ctx ctx;
        crypto_blake2b_init(&ctx, 64);
        crypto_blake2b_update(&ctx, reinterpret_cast<const uint8_t*>(kKeyLabel), sizeof kKeyLabel - 1);
        crypto_blake2b_update(&ctx, k, sizeof k);
        crypto_blake2b_update(&ctx, static_cast<const uint8_t*>(transcript), n);
        crypto_blake2b_final(&ctx, out);   // (wipes ctx)
        crypto_wipe(k, sizeof k);
        return true;
    }
};

class MonoAead final : public Aead {
public:
    uint8_t key[kKeySize] = {};
    ~MonoAead() override { crypto_wipe(key, sizeof key); }
    bool seal(const uint8_t nonce[kNonceSize], const void* aad, size_t aadLen, const void* plain, size_t n, uint8_t* out) override
    {
        // crypto_aead_lock (cipher, mac, key, nonce, ad, ad size, plain, size): our layout is cipher || mac
        crypto_aead_lock(out, out + n, key, nonce, static_cast<const uint8_t*>(aad), aadLen, static_cast<const uint8_t*>(plain), n);
        return true;
    }
    bool open(const uint8_t nonce[kNonceSize], const void* aad, size_t aadLen, const void* in, size_t n, uint8_t* out) override
    {
        if (n < kTagSize)
            return false;
        const size_t m = n - kTagSize;
        const uint8_t* c = static_cast<const uint8_t*>(in);
        // the tag is checked before anything is decrypted (crypto_aead_unlock: -1 = forged, out untouched)
        return crypto_aead_unlock(out, c + m, key, nonce, static_cast<const uint8_t*>(aad), aadLen, c, m) == 0;
    }
};

// lv: a length (2 bytes little-endian) then the bytes (the CPace draft's lv_cat: no ambiguity between fields)
void lv(std::vector<uint8_t>& v, const void* p, size_t n)
{
    v.push_back(uint8_t(n)), v.push_back(uint8_t(n >> 8));
    v.insert(v.end(), static_cast<const uint8_t*>(p), static_cast<const uint8_t*>(p) + n);
}
} // namespace

bool available() { return true; }

bool random(void* out, size_t n)
{
    uint8_t* p = static_cast<uint8_t*>(out);
#if defined(_WIN32)
    return BCryptGenRandom(nullptr, p, ULONG(n), BCRYPT_USE_SYSTEM_PREFERRED_RNG) >= 0;
#elif defined(__APPLE__)
    arc4random_buf(p, n);
    return true;
#elif defined(__linux__)
    while (n > 0) {
        const ssize_t got = getrandom(p, n, 0);
        if (got < 0) {
            if (errno == EINTR) continue;
            return false;
        }
        p += got, n -= size_t(got);
    }
    return true;
#else
    while (n > 0) {
        const size_t k = n < 256 ? n : 256;
        if (getentropy(p, k) != 0) return false;
        p += k, n -= k;
    }
    return true;
#endif
}

void blake2b(uint8_t* out, size_t outLen, const void* key, size_t keyLen, const void* data, size_t n)
{
    crypto_blake2b_keyed(out, outLen, static_cast<const uint8_t*>(key), keyLen, static_cast<const uint8_t*>(data), n);
}

void wipe(void* p, size_t n) { crypto_wipe(p, n); }

std::unique_ptr<Cpace> makeCpace(const void* password, size_t passwordLen, const char* channel)
{
    // the generator: Elligator 2 of BLAKE2b-256 (lv("CPace255") || lv(password) || lv(channel)); X25519's clamping
    // multiplies by the cofactor, so the public key lies in the prime-order group
    std::vector<uint8_t> g;
    lv(g, "CPace255", 8);
    lv(g, password, passwordLen);
    lv(g, channel, std::strlen(channel));
    uint8_t h[32], gen[32];
    crypto_blake2b(h, 32, g.data(), g.size());
    crypto_wipe(g.data(), g.size());
    crypto_elligator_map(gen, h);
    crypto_wipe(h, sizeof h);
    auto c = std::make_unique<MonoCpace>();
    if (!random(c->secret, sizeof c->secret)) {
        crypto_wipe(gen, sizeof gen);
        return nullptr;
    }
    crypto_x25519(c->pub, c->secret, gen);
    crypto_wipe(gen, sizeof gen);   // (derived from the password)
    static const uint8_t kZero[32] = {};
    if (crypto_verify32(c->pub, kZero) == 0)   // a low-order generator (never for a real hash)
        return nullptr;
    return c;
}

std::unique_ptr<Aead> makeAead(const uint8_t key[kKeySize])
{
    auto a = std::make_unique<MonoAead>();
    std::memcpy(a->key, key, kKeySize);
    return a;
}

// ---------------------------------------------------------------------------------------------------------------------
// known-answer tests
namespace {
#include "net/crypto_kat.inc"

std::vector<uint8_t> unhex(const char* s)
{
    std::vector<uint8_t> v;
    auto d = [](char c) { return c <= '9' ? c - '0' : c - 'a' + 10; };
    for (size_t i = 0; s[i] && s[i + 1]; i += 2)
        v.push_back(uint8_t(d(s[i]) << 4 | d(s[i + 1])));
    return v;
}
} // namespace

int selfTest(void (*report)(bool ok, const char* what))
{
    int failed = 0;
    auto check = [&](bool ok, const std::string& what) {
        if (!ok) ++failed;
        if (report) report(ok, what.c_str());
    };
    {   // X25519
        int bad = 0, n = 0;
        for (size_t i = 0; i + 2 < std::size(kX25519); i += 3, ++n) {
            const auto s = unhex(kX25519[i]), p = unhex(kX25519[i + 1]), want = unhex(kX25519[i + 2]);
            uint8_t got[32];
            crypto_x25519(got, s.data(), p.data());
            if (want.size() != 32 || std::memcmp(got, want.data(), 32) != 0) ++bad;
        }
        check(bad == 0, "X25519: " + std::to_string(n) + " known answers (RFC 7748 and Monocypher's vectors)");
    }
    {   // Elligator 2 map
        int bad = 0, n = 0;
        for (size_t i = 0; i + 1 < std::size(kElligatorMap); i += 2, ++n) {
            const auto r = unhex(kElligatorMap[i]), want = unhex(kElligatorMap[i + 1]);
            uint8_t got[32];
            crypto_elligator_map(got, r.data());
            if (want.size() != 32 || std::memcmp(got, want.data(), 32) != 0) ++bad;
        }
        check(bad == 0, "Elligator 2 (crypto_elligator_map): " + std::to_string(n) + " known answers");
    }
    {   // BLAKE2b (keyed and unkeyed, 32- and 64-byte hashes)
        int bad = 0, n = 0;
        for (size_t i = 0; i + 2 < std::size(kBlake2b); i += 3, ++n) {
            const auto m = unhex(kBlake2b[i]), k = unhex(kBlake2b[i + 1]), want = unhex(kBlake2b[i + 2]);
            uint8_t got[64];
            blake2b(got, want.size(), k.data(), k.size(), m.data(), m.size());
            if (want.empty() || std::memcmp(got, want.data(), want.size()) != 0) ++bad;
        }
        check(bad == 0, "BLAKE2b: " + std::to_string(n) + " known answers");
    }
    {   // XChaCha20-Poly1305 through our Aead: seal gives the vector's cipher text and tag, open takes them back,
        // a flipped bit anywhere (cipher, tag, associated data) is refused
        int bad = 0, n = 0;
        for (size_t i = 0; i + 4 < std::size(kAead); i += 5, ++n) {
            const auto key = unhex(kAead[i]), nonce = unhex(kAead[i + 1]), ad = unhex(kAead[i + 2]), pt = unhex(kAead[i + 3]),
                       want = unhex(kAead[i + 4]);   // tag || cipher text
            if (key.size() != 32 || nonce.size() != 24 || want.size() != pt.size() + 16) { ++bad; continue; }
            auto a = makeAead(key.data());
            std::vector<uint8_t> sealed(pt.size() + 16), back(pt.size() + 1);
            a->seal(nonce.data(), ad.data(), ad.size(), pt.data(), pt.size(), sealed.data());
            if (std::memcmp(sealed.data(), want.data() + 16, pt.size()) != 0 || std::memcmp(sealed.data() + pt.size(), want.data(), 16) != 0)
                ++bad;
            if (!a->open(nonce.data(), ad.data(), ad.size(), sealed.data(), sealed.size(), back.data()) ||
                std::memcmp(back.data(), pt.data(), pt.size()) != 0)
                ++bad;
            for (size_t b = 0; b < sealed.size(); b += 7) {
                sealed[b] ^= 0x10;
                if (a->open(nonce.data(), ad.data(), ad.size(), sealed.data(), sealed.size(), back.data())) ++bad;
                sealed[b] ^= 0x10;
            }
            if (!ad.empty()) {
                auto ad2 = ad;
                ad2[0] ^= 1;
                if (a->open(nonce.data(), ad2.data(), ad2.size(), sealed.data(), sealed.size(), back.data())) ++bad;
            }
        }
        check(bad == 0, "XChaCha20-Poly1305: " + std::to_string(n) + " known answers, round trips, every tampering refused");
    }
    {   // CPace: the same password agrees, another one does not, a low-order key (zero) is refused
        auto a = makeCpace("ABCDEFGH", 8, "test"), b = makeCpace("ABCDEFGH", 8, "test"), c = makeCpace("ABCDEFGX", 8, "test"),
             d = makeCpace("ABCDEFGH", 8, "other");
        uint8_t ka[64], kb[64], kc[64], kd[64];
        const bool same = a && b && a->derive(b->publicKey(), "t", 1, ka) && b->derive(a->publicKey(), "t", 1, kb) && std::memcmp(ka, kb, 64) == 0;
        check(same, "CPace: the same password gives both sides the same keys");
        const bool other = a && c && a->derive(c->publicKey(), "t", 1, ka) && c->derive(a->publicKey(), "t", 1, kc) && std::memcmp(ka, kc, 64) != 0;
        const bool chan = a && d && a->derive(d->publicKey(), "t", 1, ka) && d->derive(a->publicKey(), "t", 1, kd) && std::memcmp(ka, kd, 64) != 0;
        check(other && chan, "CPace: another password (or channel) gives other keys");
        const bool tr = a && b && a->derive(b->publicKey(), "t", 1, ka) && b->derive(a->publicKey(), "u", 1, kb) && std::memcmp(ka, kb, 64) != 0;
        check(tr, "CPace: another transcript gives other keys");
        uint8_t zero[32] = {}, one[32] = {1};
        check(a && !a->derive(zero, "t", 1, ka) && !a->derive(one, "t", 1, ka), "CPace: a low-order public key (0, 1) is refused");
    }
    return failed;
}

} // namespace hotd::net::crypto
