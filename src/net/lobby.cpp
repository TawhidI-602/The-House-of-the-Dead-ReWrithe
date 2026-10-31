#include "net/lobby.h"

#include "net/session.h"
#include "util/utf8.h"

#include <algorithm>
#include <cctype>
#include <cstring>

namespace hotd::net {

namespace {
constexpr uint8_t kMagic[4] = {'H', 'N', 'L', '1'};
constexpr size_t kFixed = 4 + 2 + 2 + 1 + 1 + 1 + 1 + 1 + 8 + 4 + 1;   // magic proto port players max chapter flags state build public len
}

uint64_t buildId()
{
    static const uint64_t id = [] {
        const char* s = __DATE__ " " __TIME__;
        uint64_t h = 1469598103934665603ull ^ kProtocol;
        for (; *s; ++s)
            h = (h ^ uint8_t(*s)) * 1099511628211ull;
        return h;
    }();
    return id;
}

std::vector<uint8_t> encodeAnnouncement(const LobbyGame& g)
{
    std::vector<uint8_t> v(kMagic, kMagic + 4);
    v.push_back(uint8_t(kProtocol)), v.push_back(uint8_t(kProtocol >> 8));
    v.push_back(uint8_t(g.addr.port)), v.push_back(uint8_t(g.addr.port >> 8));
    v.push_back(g.players), v.push_back(g.maxPlayers), v.push_back(g.chapter);
    v.push_back(uint8_t(g.needsCode ? 1 : 0));
    v.push_back(uint8_t(g.inGame ? 1 : 0));
    for (int i = 0; i < 8; ++i) v.push_back(uint8_t(g.build >> (8 * i)));
    for (int i = 0; i < 4; ++i) v.push_back(uint8_t(g.publicIp >> (8 * i)));
    const std::string name = util::utf8Head(g.name, 32);
    v.push_back(uint8_t(name.size()));
    v.insert(v.end(), name.begin(), name.end());
    // a game open on the LAN (needsCode 0, 2026-10-30): its password follows (a length, 4-32 letters / digits), so the
    // LAN list joins with one click; never in a game that needs the code
    const std::string code = g.needsCode ? std::string() : normaliseCode(g.code);
    if (!g.needsCode) {
        v.push_back(uint8_t(code.size()));
        v.insert(v.end(), code.begin(), code.end());
    }
    return v;
}

bool decodeAnnouncement(const uint8_t* p, size_t n, LobbyGame& g)
{
    if (n < kFixed || n > kFixed + 32 + 1 + 32 || std::memcmp(p, kMagic, 4) != 0)
        return false;
    LobbyGame t;
    if (uint16_t(p[4] | p[5] << 8) != kProtocol)
        return false;
    t.addr.port = uint16_t(p[6] | p[7] << 8);
    t.players = p[8], t.maxPlayers = p[9], t.chapter = p[10];
    if (t.addr.port == 0 || t.maxPlayers < 1 || t.maxPlayers > 4 || t.players > t.maxPlayers || t.chapter > 3 || p[11] > 1 || p[12] > 1)
        return false;
    t.needsCode = p[11] != 0;
    t.inGame = p[12] != 0;
    t.build = 0;
    for (int i = 0; i < 8; ++i) t.build |= uint64_t(p[13 + i]) << (8 * i);
    t.publicIp = uint32_t(p[21]) | uint32_t(p[22]) << 8 | uint32_t(p[23]) << 16 | uint32_t(p[24]) << 24;
    const size_t len = p[25];
    if (len > 32)
        return false;
    if (t.needsCode ? n != kFixed + len : n < kFixed + len + 1 || n != kFixed + len + 1 + p[kFixed + len])
        return false;
    if (!t.needsCode) {   // (the open game's password: letters / digits only, as normaliseCode keeps them)
        t.code.assign(reinterpret_cast<const char*>(p + kFixed + len + 1), p[kFixed + len]);
        if (t.code.empty() || normaliseCode(t.code) != t.code)
            return false;
    }
    for (size_t i = 0; i < len; ++i) {   // printable ASCII only (shown in the menu)
        const char c = char(p[kFixed + i]);
        t.name += c >= 32 && c < 127 ? c : '?';
    }
    if (t.name.empty())
        t.name = "HOTD game";
    g = t;
    return true;
}

namespace {
const char kB32[] = "0123456789ABCDEFGHJKMNPQRSTVWXYZ";   // Crockford
uint8_t check(const uint8_t* b, size_t n)
{
    uint8_t c = 0xA5;
    for (size_t i = 0; i < n; ++i)
        c = uint8_t((c << 1 | c >> 7) ^ b[i]);
    return c;
}
} // namespace

namespace {
// the short join code (docs/netplay.md "Join code"): 16 symbols of kJoin (31: no 0 O 1 I L), shown XXXX-XXXX-XXXX-XXXX.
// The number it spells: ((ip * 4 + port - 47800) * 2^40 + secret) * 37 + check, below 2^79.3 = 31^16. The secret is the
// password (8 Crockford symbols = 40 bits); the check (mod 37) refuses most typos before anything is sent.
const char kJoin[] = "ABCDEFGHJKMNPQRSTUVWXYZ23456789";
constexpr uint32_t kJoinBase = 31, kJoinCheck = 37, kJoinLen = 16;
constexpr uint16_t kJoinPort = 47800;

// a 96-bit number in three 32-bit limbs (most significant first): small multiplies / divides only
struct Big {
    uint32_t w[3] = {};
    void mulAdd(uint32_t m, uint32_t add)
    {
        uint64_t carry = add;
        for (int i = 2; i >= 0; --i) {
            const uint64_t x = uint64_t(w[i]) * m + carry;
            w[i] = uint32_t(x);
            carry = x >> 32;
        }
    }
    uint32_t divMod(uint32_t d)
    {
        uint64_t r = 0;
        for (int i = 0; i < 3; ++i) {
            const uint64_t x = r << 32 | w[i];
            w[i] = uint32_t(x / d);
            r = x % d;
        }
        return uint32_t(r);
    }
};

uint32_t joinCheck(uint32_t ip, uint32_t portOff, uint64_t secret)
{
    uint64_t h = 1469598103934665603ull;
    const uint64_t parts[3] = {ip, portOff, secret};
    for (uint64_t p : parts)
        for (int i = 0; i < 8; ++i)
            h = (h ^ uint8_t(p >> (8 * i))) * 1099511628211ull;
    return uint32_t(h % kJoinCheck);
}

// the password as 40 bits: exactly 8 Crockford symbols (what randomCode makes); otherwise false (the long form then)
bool secretOf(const std::string& code, uint64_t& v)
{
    if (code.size() != 8)
        return false;
    v = 0;
    for (char c : code) {
        const char* p = c ? std::strchr(kB32, c) : nullptr;
        if (!p) return false;
        v = v << 5 | uint64_t(p - kB32);
    }
    return true;
}
} // namespace

std::string makeJoinCode(const Address& a, const std::string& code)
{
    uint64_t secret = 0;
    if (a.ip == 0 || a.port < kJoinPort || a.port > kJoinPort + 3 || !secretOf(code, secret))
        return "";
    const uint32_t off = a.port - kJoinPort;
    Big n;
    n.w[2] = a.ip;
    n.mulAdd(4, off);
    n.mulAdd(1u << 20, 0), n.mulAdd(1u << 20, 0);
    n.w[1] |= uint32_t(secret >> 32), n.w[2] |= uint32_t(secret);   // (the low 40 bits are zero here)
    n.mulAdd(kJoinCheck, joinCheck(a.ip, off, secret));
    char s[kJoinLen];
    for (int i = int(kJoinLen) - 1; i >= 0; --i)
        s[i] = kJoin[n.divMod(kJoinBase)];
    std::string out;
    for (uint32_t i = 0; i < kJoinLen; ++i) {
        if (i && i % 4 == 0) out += '-';
        out += s[i];
    }
    return out;
}

int parseJoinCode(const std::string& text, Address& a, std::string& code)
{
    std::string t;
    for (char c : text)
        if (c != ' ' && c != '\t' && c != '\r' && c != '\n' && c != '-')
            t += char(std::toupper(static_cast<unsigned char>(c)));
    if (t.size() != kJoinLen)
        return t.size() > kJoinLen ? -3 : -1;
    Big n;
    for (char c : t) {
        const char* p = c ? std::strchr(kJoin, c) : nullptr;
        if (!p) return -2;   // 0 O 1 I L or a sign: never in a code
        n.mulAdd(kJoinBase, uint32_t(p - kJoin));
    }
    const uint32_t chk = n.divMod(kJoinCheck);
    const uint64_t secret = (uint64_t(n.w[1] & 0xFF) << 32) | n.w[2];
    n.w[2] = 0, n.w[1] &= ~0xFFu;
    n.divMod(1u << 20), n.divMod(1u << 20);
    const uint32_t off = n.divMod(4);
    if (n.w[0] || n.w[1] || joinCheck(n.w[2], off, secret) != chk || n.w[2] == 0)
        return -4;
    a.ip = n.w[2];
    a.port = uint16_t(kJoinPort + off);
    code.clear();
    for (int i = 7; i >= 0; --i)
        code += kB32[(secret >> (5 * i)) & 31];
    return 1;
}

std::string makeShareCode(const Address& a, const std::string& code)
{
    const std::string j = makeJoinCode(a, code);
    if (!j.empty())
        return j;
    uint8_t b[7] = {uint8_t(a.ip >> 24), uint8_t(a.ip >> 16), uint8_t(a.ip >> 8), uint8_t(a.ip), uint8_t(a.port >> 8), uint8_t(a.port), 0};
    b[6] = check(b, 6);
    uint64_t v = 0;
    for (uint8_t x : b) v = v << 8 | x;   // 56 bits -> 12 characters (60 bits)
    std::string s = "HOTD-";
    for (int i = 11; i >= 0; --i)
        s += kB32[(v >> (5 * i)) & 31];
    return s + "-" + code;
}

bool parseShareCode(const std::string& text, Address& a, std::string& code)
{
    if (parseJoinCode(text, a, code) > 0)
        return true;
    std::string t;
    for (char c : text)
        if (c != ' ' && c != '\t' && c != '\r' && c != '\n')
            t += char(std::toupper(static_cast<unsigned char>(c)));
    if (t.size() < 5 + 12 + 1 + 4 || t.compare(0, 5, "HOTD-") != 0 || t[17] != '-')
        return false;
    uint64_t v = 0;
    for (int i = 0; i < 12; ++i) {
        char c = t[size_t(5 + i)];
        if (c == 'O') c = '0';
        if (c == 'I' || c == 'L') c = '1';
        const char* p = std::strchr(kB32, c);
        if (!p || !c) return false;
        v = v << 5 | uint64_t(p - kB32);
    }
    if (v >> 56) return false;
    uint8_t b[7];
    for (int i = 6; i >= 0; --i, v >>= 8) b[i] = uint8_t(v);
    if (check(b, 6) != b[6]) return false;
    a.ip = uint32_t(b[0]) << 24 | uint32_t(b[1]) << 16 | uint32_t(b[2]) << 8 | b[3];
    a.port = uint16_t(b[4] << 8 | b[5]);
    code = normaliseCode(t.substr(18));
    return a.ip != 0 && a.port != 0 && !code.empty();
}

bool LobbyAnnouncer::start(const Address& to, std::string& err)
{
    to_ = to;
    Address bind;
    bind.ip = (to.ip >> 24) == 127 ? 0x7F000001 : 0;   // a loopback test stays on the loopback
    if (!sock_.open(bind, err))
        return false;
    sock_.enableBroadcast();
    return true;
}

void LobbyAnnouncer::tick(const LobbyGame& game)
{
    const auto now = std::chrono::steady_clock::now();
    if (!sock_.isOpen() || now - last_ < std::chrono::seconds(1))
        return;
    last_ = now;
    const std::vector<uint8_t> v = encodeAnnouncement(game);
    sock_.send(to_, v.data(), v.size());
    // 255.255.255.255 leaves by one adapter only (Windows: the first one, often a VPN's / Hyper-V's / VirtualBox's):
    // also each adapter's own subnet broadcast, so every network this PC is on hears it (re-read every 10 s: Wi-Fi)
    if (to_.ip != 0xFFFFFFFFu)
        return;
    if (ifsAt_ == std::chrono::steady_clock::time_point{} || now - ifsAt_ > std::chrono::seconds(10)) {
        ifs_ = localInterfaces();
        ifsAt_ = now;
    }
    for (const LocalIf& f : ifs_)
        sock_.send(Address{f.broadcast, to_.port}, v.data(), v.size());
}

bool LobbyListener::start(const Address& bind, std::string& err)
{
    return sock_.open(bind, err, true);
}

void LobbyListener::poll(int waitMs)
{
    const auto now = std::chrono::steady_clock::now();
    uint8_t buf[256];
    for (int k = 0; k < 64 && sock_.isOpen(); ++k) {
        Address from;
        const int n = sock_.recv(buf, sizeof buf, from, k == 0 ? waitMs : 0);
        if (n <= 0)
            break;
        LobbyGame g;
        if (!decodeAnnouncement(buf, size_t(n), g)) { ++dropped; continue; }
        g.addr.ip = from.ip;   // the sender's address, never one written in the packet
        g.seen = now;
        auto it = std::find_if(games_.begin(), games_.end(), [&](const LobbyGame& o) { return o.addr == g.addr; });
        if (it != games_.end())
            *it = g;
        else if (games_.size() < 32)
            games_.push_back(g);
    }
    games_.erase(std::remove_if(games_.begin(), games_.end(),
                                [&](const LobbyGame& g) { return now - g.seen > std::chrono::seconds(3); }),
                 games_.end());
}

} // namespace hotd::net
