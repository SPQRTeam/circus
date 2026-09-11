#pragma once

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>

namespace spqr {

// TCP Communication: sends the size of the message first, then the message itself.
// Returns the number of bytes sent (excluding the size header), or -1 on error.

// Source - https://stackoverflow.com/a
// Posted by Arun, modified by community. See post 'Timeline' for change history
// Retrieved 2026-01-12, License - CC BY-SA 3.0
inline ssize_t send_all(int fd, const char* buf, size_t len) {
    // First, send the size of the message
    uint32_t msg_size = htonl(len);
    ssize_t bytes_sent = send(fd, &msg_size, sizeof(msg_size), 0);
    if (bytes_sent != sizeof(msg_size)) {
        return -1;
    }

    ssize_t total = 0;       // how many bytes we've sent
    size_t bytesleft = len;  // how many we have left to send
    ssize_t n = 0;
    while (total < len) {
        n = send(fd, buf + total, bytesleft, 0);
        if (n == -1) {
            /* print/log error details */
            return -1;
        }
        total += n;
        bytesleft -= n;
    }
    return total;
}

}