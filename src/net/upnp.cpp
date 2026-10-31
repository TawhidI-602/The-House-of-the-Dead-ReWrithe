#include "net/upnp.h"

#include <chrono>
#include <cstdio>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <natupnp.h>
#include <oleauto.h>
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace hotd::net {

namespace {
std::string ipText(uint32_t ip)
{
    Address a;
    a.ip = ip;
    const std::string s = a.str();
    return s.substr(0, s.rfind(':'));
}
} // namespace

uint32_t localLanAddress()
{
#ifdef _WIN32
    WSADATA d;
    if (WSAStartup(MAKEWORD(2, 2), &d) != 0)
        return 0;
    SOCKET s = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s == INVALID_SOCKET)
        return 0;
#else
    int s = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s < 0)
        return 0;
#endif
    // a UDP "connect" only picks the route (no packet is sent): the interface address the default route uses
    sockaddr_in to{};
    to.sin_family = AF_INET;
    to.sin_port = htons(53);
    to.sin_addr.s_addr = htonl(0x08080808);
    uint32_t ip = 0;
    if (::connect(s, reinterpret_cast<sockaddr*>(&to), sizeof to) == 0) {
        sockaddr_in me{};
#ifdef _WIN32
        int len = sizeof me;
#else
        socklen_t len = sizeof me;
#endif
        if (::getsockname(s, reinterpret_cast<sockaddr*>(&me), &len) == 0)
            ip = ntohl(me.sin_addr.s_addr);
    }
#ifdef _WIN32
    closesocket(s);
#else
    ::close(s);
#endif
    return ip;
}

void PortMapper::setMessage(const std::string& m)
{
    std::lock_guard<std::mutex> l(mutex_);
    message_ = m;
}

std::string PortMapper::message() const
{
    std::lock_guard<std::mutex> l(mutex_);
    return message_;
}

void PortMapper::start(uint16_t port, Mode mode)
{
    stop();
    port_ = port;
    mode_ = mode;
    stopping_ = false;
    external_ = 0;
    if (mode == Mode::Off) {
        state_ = State::Idle;
        setMessage("UPnP off: forward UDP port " + std::to_string(port) + " on your router for internet play");
        return;
    }
    state_ = State::Working;
    setMessage("Asking your router to open UDP port " + std::to_string(port) + " (UPnP) ...");
    thread_ = std::thread([this] { run(); });
}

void PortMapper::stop()
{
    stopping_ = true;
    if (thread_.joinable())
        thread_.join();
}

void PortMapper::run()
{
    const uint32_t lan = localLanAddress();
    if (mode_ == Mode::DryRun) {   // testing: no COM, no packet, no mapping
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        setMessage("UPnP (dry run): would map UDP " + std::to_string(port_) + " to " + ipText(lan) + ":" + std::to_string(port_) +
                   "; nothing was sent");
        state_ = State::Mapped;
        while (!stopping_)
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        return;
    }
#ifdef _WIN32
    const std::string forward = "forward UDP port " + std::to_string(port_) + " to " + ipText(lan) + " on your router for internet play";
    if (lan == 0) {
        state_ = State::Failed;
        setMessage("No network found: " + forward);
        return;
    }
    if (FAILED(CoInitializeEx(nullptr, COINIT_MULTITHREADED))) {
        state_ = State::Failed;
        setMessage("UPnP not available: " + forward);
        return;
    }
    IUPnPNAT* nat = nullptr;
    IStaticPortMappingCollection* coll = nullptr;
    BSTR udp = SysAllocString(L"UDP");
    wchar_t lanW[32];
    std::swprintf(lanW, 32, L"%u.%u.%u.%u", lan >> 24, (lan >> 16) & 0xFF, (lan >> 8) & 0xFF, lan & 0xFF);
    BSTR client = SysAllocString(lanW);
    BSTR desc = SysAllocString(L"HOTD ReWrithe online co-op");
    if (SUCCEEDED(CoCreateInstance(__uuidof(UPnPNAT), nullptr, CLSCTX_ALL, __uuidof(IUPnPNAT), reinterpret_cast<void**>(&nat))) && nat &&
        SUCCEEDED(nat->get_StaticPortMappingCollection(&coll)) && coll) {
        IStaticPortMapping* m = nullptr;
        if (SUCCEEDED(coll->Add(port_, udp, port_, client, VARIANT_TRUE, desc, &m)) && m) {
            mapped_ = true;
            BSTR ext = nullptr;
            if (SUCCEEDED(m->get_ExternalIPAddress(&ext)) && ext) {
                char a[64] = {};
                std::snprintf(a, sizeof a, "%ls", ext);
                in_addr ia{};
                if (inet_pton(AF_INET, a, &ia) == 1)
                    external_ = ntohl(ia.s_addr);
                SysFreeString(ext);
            }
            m->Release();
            state_ = State::Mapped;
            setMessage("Router port UDP " + std::to_string(port_) + " open (UPnP)" +
                       (external_ ? "; internet address " + ipText(external_) : std::string()));
        } else {
            state_ = State::Failed;
            setMessage("The router refused UPnP: " + forward);
        }
    } else {
        state_ = State::Failed;
        setMessage("No UPnP router found: " + forward);
    }
    while (!stopping_)
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    if (mapped_ && coll) {   // removed again when the game stops hosting
        coll->Remove(port_, udp);
        mapped_ = false;
    }
    if (coll) coll->Release();
    if (nat) nat->Release();
    SysFreeString(udp), SysFreeString(client), SysFreeString(desc);
    CoUninitialize();
#else
    state_ = State::Failed;
    setMessage("UPnP not available here: forward UDP port " + std::to_string(port_) + " on your router for internet play");
#endif
}

} // namespace hotd::net
