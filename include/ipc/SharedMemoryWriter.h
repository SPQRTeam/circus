#pragma once

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace spqr {

// No metadata (default): use when the reader already knows how to interpret
// the payload (e.g. a fixed compile-time state struct written with element_count == 1).
struct NoSharedMemoryMeta {};

// Lock-free single-writer/multi-reader shared-memory ring buffer for any
// trivially-copyable T, with an optional trivially-copyable Meta blob written
// once at configure() time (e.g. image width/height/channels, needed by a
// reader that doesn't otherwise know the payload's shape).
//
// Shared memory is just shared pages, not a wire protocol: a trivially-copyable
// type's bytes are already valid in any process built from the same struct
// definition, so plain memcpy is sufficient (and fastest) -- no serialization
// step is needed or performed.
template <typename T, typename Meta = NoSharedMemoryMeta>
class SharedMemoryWriter {
    public:
        static_assert(std::is_trivially_copyable_v<T>, "SharedMemoryWriter<T> requires a trivially-copyable T");
        static_assert(std::is_trivially_copyable_v<Meta>, "SharedMemoryWriter<T, Meta> requires a trivially-copyable Meta");

        SharedMemoryWriter() = default;

        ~SharedMemoryWriter() {
            close_();
        }

        // element_count: how many T's fit in a single slot (e.g. width*height*channels
        // for a raw uint8_t image; 1 for a single POD message struct).
        void configure(const std::string& path, size_t element_count, Meta meta = {}, int ring_slots = 3) {
            if (element_count == 0 || ring_slots <= 0) {
                throw std::invalid_argument("Invalid shared-memory configuration");
            }
            const size_t slot_bytes = element_count * sizeof(T);
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

        void write(const T* data, size_t count) {
            if (!header_) {
                return;
            }
            if (count * sizeof(T) != header_->slot_bytes) {
                return;
            }

            const uint64_t next_seq = __atomic_load_n(&header_->seq, __ATOMIC_RELAXED) + 1;
            const uint32_t slot_idx = static_cast<uint32_t>(next_seq % header_->slot_count);
            uint8_t* slots_base = static_cast<uint8_t*>(map_ptr_) + sizeof(Header);
            uint8_t* dst = slots_base + static_cast<size_t>(slot_idx) * header_->slot_bytes;
            std::memcpy(dst, data, header_->slot_bytes);
            __atomic_store_n(&header_->seq, next_seq, __ATOMIC_RELEASE);
        }

        void write(const T& value) {
            write(&value, 1);
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
        static constexpr uint32_t kVersion = 1;

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
};

}  // namespace spqr
