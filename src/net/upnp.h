// UPnP port mapping for the host (docs/netplay.md "Over the internet"): Windows' own UPnP (the NATUPnP COM API,
// IUPnPNAT / IStaticPortMappingCollection; no library of ours). Runs on its own thread (the router search takes
// seconds); the mapping is removed again by stop() / the destructor. Dry run (the tests): nothing is sent, nothing
// mapped, the messages say what would happen.
#pragma once

#include "net/socket.h"

#include <atomic>
#include <mutex>
#include <string>
#include <thread>

namespace hotd::net {

// this PC's LAN address (the interface the default route uses; nothing is sent), 0 = none
uint32_t localLanAddress();

class PortMapper {
public:
    enum class Mode { Off, On, DryRun };
    ~PortMapper() { stop(); }
    void start(uint16_t port, Mode mode);
    void stop();                     // removes the mapping (waits for the thread)
    enum class State { Idle, Working, Mapped, Failed };
    State state() const { return state_; }
    std::string message() const;     // for the waiting room
    uint32_t externalIp() const { return external_; }   // 0: not known
private:
    std::thread thread_;
    std::atomic<State> state_{State::Idle};
    std::atomic<uint32_t> external_{0};
    std::atomic<bool> stopping_{false};
    mutable std::mutex mutex_;
    std::string message_;
    uint16_t port_ = 0;
    Mode mode_ = Mode::Off;
    bool mapped_ = false;
    void setMessage(const std::string& m);
    void run();
};

} // namespace hotd::net
