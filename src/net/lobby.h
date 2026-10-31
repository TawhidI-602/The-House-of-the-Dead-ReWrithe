// The LAN lobby (docs/netplay.md "LAN lobby"): a host announces its game on the local network (UDP broadcast, once a
// second, on kLobbyPort); Join lists the games heard. An announcement holds no secrets: the game's name, players,
// chapter, the build, whether a code is needed, the game's UDP port. Everything received is checked and sanitised.
#pragma once

#include "net/socket.h"

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

namespace hotd::net {

constexpr uint16_t kLobbyPort = 47777;

struct LobbyGame {
    Address addr;              // the host's game port (from the sender's address + the announced port)
    std::string name;          // printable ASCII, at most 32
    uint8_t players = 1, maxPlayers = 2, chapter = 0;
    bool needsCode = true, inGame = false;
    uint64_t build = 0;        // the announcing build (another build: listed, marked, not joinable)
    uint32_t publicIp = 0;     // the host's internet address (UPnP; 0 unknown): a join code with it is this game here
                               // (the joiner on the same LAN connects to addr instead: protocol 13)
    // needsCode false (2026-10-30, the default for a host's LAN): the game's password, sent in the announcement, so a
    // click in the LAN list joins without typing the code (only the local network hears a broadcast; joins over the
    // internet still need the code). --lan-code: the code is needed on the LAN too (nothing secret announced)
    std::string code;
    std::chrono::steady_clock::time_point seen;
};

uint64_t buildId();            // this build's id (compile date / time, protocol)

class LobbyAnnouncer {
public:
    // to: the broadcast address (255.255.255.255:kLobbyPort) or, for tests, 127.0.0.1:<port>
    bool start(const Address& to, std::string& err);
    void stop() { sock_.close(); }
    // sends at most once a second
    void tick(const LobbyGame& game);
private:
    UdpSocket sock_;
    Address to_;
    std::chrono::steady_clock::time_point last_{}, ifsAt_{};
    std::vector<LocalIf> ifs_;   // (to 255.255.255.255: each adapter's subnet broadcast too)
};

class LobbyListener {
public:
    // bind: 0.0.0.0:kLobbyPort for a LAN (Windows Firewall asks once), 127.0.0.1:<port> for tests
    bool start(const Address& bind, std::string& err);
    void stop() { sock_.close(); }
    void poll(int waitMs = 0);                 // receives the announcements; drops games silent for 3 s
    const std::vector<LobbyGame>& games() const { return games_; }
    uint64_t dropped = 0;
private:
    UdpSocket sock_;
    std::vector<LobbyGame> games_;
};

// the join code (docs/netplay.md "Join code"): the host's address, port (47800-47803) and password (8 Crockford symbols
// = 40 bits, what randomCode makes) in 16 symbols, "XXXX-XXXX-XXXX-XXXX": no 0 O 1 I L, any case, dashes / spaces
// optional, a check symbol against typos. "" when the address / password does not fit (the long share code then)
std::string makeJoinCode(const Address& a, const std::string& code);
// 1 = a join code (a, code set); -1 too short, -2 a letter never used (0 O 1 I L ...), -3 too long, -4 a typo (the check)
int parseJoinCode(const std::string& text, Address& a, std::string& code);
// the share code: the join code when it fits, else the long form "HOTD-XXXXXXXXXXXX-CODE" (the address in Crockford
// base32 with a check byte; a custom password). parseShareCode takes either
std::string makeShareCode(const Address& a, const std::string& code);
bool parseShareCode(const std::string& text, Address& a, std::string& code);

// the wire form (exposed for the fuzz test)
std::vector<uint8_t> encodeAnnouncement(const LobbyGame& g);
bool decodeAnnouncement(const uint8_t* p, size_t n, LobbyGame& g);

} // namespace hotd::net
