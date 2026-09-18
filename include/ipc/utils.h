#pragma once

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <msgpack.hpp>

namespace spqr {

// TCP Communication: sender-side counterpart to recv_latest below -- loops
// send() until the full buffer has been handed to the kernel. Writes no
// length header: recv_latest's persistent msgpack::unpacker tracks message
// boundaries itself (MessagePack values are self-delimiting), so nothing
// needs to declare how many bytes are coming. Returns the number of bytes
// sent, or -1 on error.
inline ssize_t send_unframed(int fd, const char* buf, size_t len) {
    ssize_t total = 0;
    while (static_cast<size_t>(total) < len) {
        ssize_t n = send(fd, buf + total, len - static_cast<size_t>(total), 0);
        if (n <= 0) {
            return -1;
        }
        total += n;
    }
    return total;
}

// TCP Communication: reads whatever bytes are currently available on fd into
// the caller-owned, per-connection `unp` (kept alive across calls, e.g. in a
// std::unordered_map<int, msgpack::unpacker> keyed by fd), then extracts the
// most recently completed message -- any older ones that had piled up in the
// same read (fast sender, slow reader) are discarded on purpose, since only
// the latest is ever still relevant. Unlike a length-prefixed protocol, this
// needs no length header: MessagePack values are self-delimiting, so `unp`
// tracks message boundaries -- and any partial trailing bytes -- itself.
// Returns 1 if `out` now holds a complete message, 0 if the read only added
// to a still-incomplete one, or -1 if the connection was closed/errored (the
// caller should close fd).
inline int recv_latest(int fd, msgpack::unpacker& unp, msgpack::object_handle& out, size_t maxChunk) {
    unp.reserve_buffer(maxChunk);
    ssize_t n = read(fd, unp.buffer(), unp.buffer_capacity());
    if (n <= 0) {
        return -1;
    }
    unp.buffer_consumed(static_cast<size_t>(n));

    msgpack::object_handle oh;
    bool gotOne = false;
    while (unp.next(oh)) {
        out = std::move(oh);
        gotOne = true;
    }
    return gotOne ? 1 : 0;
}

}