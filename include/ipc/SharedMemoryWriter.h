#pragma once

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <initializer_list>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace spqr {

// No metadata (default): use when the reader already knows how to interpret
// the payload (e.g. a fixed compile-time state struct with no trailing data).
struct NoSharedMemoryMeta {};

// Metadata for a segment whose trailing region holds one or more camera frames:
// lets a reader discover each frame's shape at runtime, since the resolution
// comes from the MuJoCo model and so is not known at compile time.
struct ImageMeta {
        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t channels = 0;
};

// One source region of a slot's trailing payload, for the composite write below.
struct SharedMemoryChunk {
        const void* data = nullptr;
        size_t bytes = 0;
};

// Lock-free single-writer/multi-reader shared-memory ring buffer for any
// trivially-copyable T, with an optional trivially-copyable Meta blob written
// once at configure() time (e.g. image width/height/channels, needed by a
// reader that doesn't otherwise know the payload's shape).
//
// Shared memory is just shared pages, not a wire protocol: a trivially-copyable
// type's bytes are already valid in any process built from the same struct
// definition, so plain memcpy is sufficient (and fastest) -- no serialization
// step is needed or performed.
//
// A slot is one T (the head) optionally followed by a runtime-sized trailing
// region of opaque bytes. That covers payloads whose size is only known at
// startup -- camera frames, whose resolution comes from the MuJoCo model --
// without giving up the compile-time type of the head: T stays POD, and the
// shape of the trailing region travels in Meta. Head and trailing are
// published under a single seq, so a reader never mixes a head from one tick
// with trailing bytes from another. trailing_bytes = 0 is the degenerate case
// of a plain POD message struct with no trailing data.
template <typename T, typename Meta = NoSharedMemoryMeta>
class SharedMemoryWriter {
    public:
        static_assert(std::is_trivially_copyable_v<T>, "SharedMemoryWriter<T> requires a trivially-copyable T");
        static_assert(std::is_trivially_copyable_v<Meta>, "SharedMemoryWriter<T, Meta> requires a trivially-copyable Meta");

        SharedMemoryWriter() = default;

        ~SharedMemoryWriter() {
            close_();
        }

        // trailing_bytes: how many bytes of opaque data follow the T head in each
        // slot (e.g. summed image sizes), fixed for the segment's lifetime; 0 for a
        // plain POD message struct with no trailing data.
        void configure(const std::string& path, size_t trailing_bytes = 0, Meta meta = {}, int ring_slots = 3) {
            if (ring_slots <= 0) {
                throw std::invalid_argument("Invalid shared-memory configuration");
            }
            trailing_bytes_ = trailing_bytes;
            // Pad so every slot start stays 8-aligned, keeping the head's doubles
            // naturally aligned for a reader that maps the slot in place.
            const size_t padded_trailing = (trailing_bytes + 7u) & ~static_cast<size_t>(7u);
            const size_t slot_bytes = sizeof(T) + padded_trailing;
            const size_t total_bytes = sizeof(Header) + static_cast<size_t>(ring_slots) * slot_bytes;

            close_();

            const std::filesystem::path p(path);
            if (p.has_parent_path()) {
                std::filesystem::create_directories(p.parent_path());
            }

            fd_ = open(path.c_str(), O_RDWR | O_CREAT, 0666);
            if (fd_ < 0) {
                throw std::runtime_error("Failed to open shared memory file: " + path);
            }

            if (ftruncate(fd_, static_cast<off_t>(total_bytes)) != 0) {
                close_();
                throw std::runtime_error("Failed to resize shared memory file: " + path);
            }

            map_bytes_ = total_bytes;
            map_ptr_ = mmap(nullptr, map_bytes_, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0);
            if (map_ptr_ == MAP_FAILED) {
                map_ptr_ = nullptr;
                close_();
                throw std::runtime_error("Failed to mmap shared memory file: " + path);
            }

            header_ = static_cast<Header*>(map_ptr_);
            if (header_->magic != kMagic || header_->version != kVersion || header_->slot_bytes != slot_bytes
                || header_->slot_count != static_cast<uint32_t>(ring_slots)) {
                std::memset(header_, 0, sizeof(Header));
                header_->magic = kMagic;
                header_->version = kVersion;
                header_->meta = meta;
                header_->slot_count = static_cast<uint32_t>(ring_slots);
                header_->slot_bytes = slot_bytes;
                header_->seq = 0;
            } else {
                // Segment already initialized with a compatible layout; keep it current.
                header_->meta = meta;
            }

            path_ = path;
        }

        bool isReady() const {
            return header_ != nullptr;
        }

        // Writes the head, followed by the trailing chunks (if any), into one slot
        // published with a single seq bump -- so head and trailing are always from
        // the same tick. The chunk sizes must add up to the trailing_bytes passed to
        // configure(); a mismatch is dropped rather than writing a partial slot.
        void write(const T& head, std::initializer_list<SharedMemoryChunk> trailing = {}) {
            if (!header_) {
                return;
            }
            size_t supplied = 0;
            for (const SharedMemoryChunk& chunk : trailing) {
                supplied += chunk.bytes;
            }
            if (supplied != trailing_bytes_) {
                return;
            }

            const uint64_t next_seq = __atomic_load_n(&header_->seq, __ATOMIC_RELAXED) + 1;
            const uint32_t slot_idx = static_cast<uint32_t>(next_seq % header_->slot_count);
            uint8_t* slots_base = static_cast<uint8_t*>(map_ptr_) + sizeof(Header);
            uint8_t* dst = slots_base + static_cast<size_t>(slot_idx) * header_->slot_bytes;

            std::memcpy(dst, &head, sizeof(T));
            dst += sizeof(T);
            for (const SharedMemoryChunk& chunk : trailing) {
                if (chunk.bytes > 0) {
                    std::memcpy(dst, chunk.data, chunk.bytes);
                    dst += chunk.bytes;
                }
            }
            __atomic_store_n(&header_->seq, next_seq, __ATOMIC_RELEASE);
        }

    private:
        struct Header {
                uint32_t magic = 0;
                uint32_t version = 0;
                Meta meta{};
                uint32_t slot_count = 0;
                size_t slot_bytes = 0;
                uint64_t seq = 0;
        };

        static constexpr uint32_t kMagic = 0x5348514d;  // SHQM
        // v2 added the composite head+trailing slot layout. The bump matters because
        // Meta sits inside Header: a v1 peer would still match magic/version at
        // offsets 0 and 4 but read slot_count/slot_bytes from the wrong offsets.
        static constexpr uint32_t kVersion = 2;

        void close_() {
            if (map_ptr_) {
                munmap(map_ptr_, map_bytes_);
            }
            map_ptr_ = nullptr;
            header_ = nullptr;
            map_bytes_ = 0;
            if (fd_ >= 0) {
                close(fd_);
            }
            fd_ = -1;
            path_.clear();
        }

        int fd_ = -1;
        void* map_ptr_ = nullptr;
        size_t map_bytes_ = 0;
        Header* header_ = nullptr;
        std::string path_;
        size_t trailing_bytes_ = 0;  // unpadded; slot_bytes rounds this up to 8
};

}  // namespace spqr
