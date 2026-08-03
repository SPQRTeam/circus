#pragma once

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstdint>
#include <cstring>
#include <string>
#include <type_traits>

#include "ipc/SharedMemoryWriter.h"  // for NoSharedMemoryMeta

namespace spqr {

// Reader counterpart to SharedMemoryWriter<T, Meta> (external/circus/include/ipc/SharedMemoryWriter.h).
// Used when circus is the *reader* (e.g. commands written by simbridge). Must
// stay in sync with whatever writes the segment: same Header layout, same
// magic/version, same ring-buffer/atomic-seq protocol. Mirrors
// external/simbridge/src/SharedMemoryReader.hpp -- kept as a separate copy on
// purpose, same duplication discipline used for the image Header struct.
template <typename T, typename Meta = NoSharedMemoryMeta>
class SharedMemoryReader {
    public:
        static_assert(std::is_trivially_copyable_v<T>, "SharedMemoryReader<T> requires a trivially-copyable T");
        static_assert(std::is_trivially_copyable_v<Meta>, "SharedMemoryReader<T, Meta> requires a trivially-copyable Meta");

        SharedMemoryReader() = default;

        ~SharedMemoryReader() {
            close_();
        }

        void configure(const std::string& path) {
            path_ = path;
        }

        // Reads the Meta blob written at configure() time on the writer side.
        // Safe to call before any frame has been written (meta is set independently of seq).
        bool getMeta(Meta& out) {
            if (!ensureMapped_() || !header_ || header_->magic != kMagic || header_->version != kVersion) {
                return false;
            }
            out = header_->meta;
            return true;
        }

        // out must point to at least `count` T's (count == element_count the writer was configured with).
        bool readLatest(T* out, size_t count) {
            if (!ensureMapped_()) {
                return false;
            }
            if (!header_) {
                return false;
            }
            if (header_->magic != kMagic || header_->version != kVersion || header_->slot_count == 0 || header_->slot_bytes == 0) {
                return false;
            }
            if (count * sizeof(T) != header_->slot_bytes) {
                return false;
            }

            const uint64_t seq = __atomic_load_n(&header_->seq, __ATOMIC_ACQUIRE);
            if (seq == 0 || seq == last_seq_) {
                return false;
            }

            const uint32_t slot_idx = static_cast<uint32_t>(seq % header_->slot_count);
            const uint8_t* slots_base = static_cast<const uint8_t*>(map_ptr_) + sizeof(Header);
            const uint8_t* src = slots_base + static_cast<size_t>(slot_idx) * header_->slot_bytes;

            std::memcpy(out, src, header_->slot_bytes);

            last_seq_ = seq;
            return true;
        }

        // Convenience for element_count == 1 (single POD struct per slot).
        bool readLatest(T& out) {
            return readLatest(&out, 1);
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

        bool ensureMapped_() {
            if (map_ptr_) {
                return true;
            }
            if (path_.empty()) {
                return false;
            }

            fd_ = open(path_.c_str(), O_RDONLY);
            if (fd_ < 0) {
                return false;
            }

            struct stat st {};
            if (fstat(fd_, &st) != 0 || st.st_size < static_cast<off_t>(sizeof(Header))) {
                close_();
                return false;
            }

            map_bytes_ = static_cast<size_t>(st.st_size);
            map_ptr_ = mmap(nullptr, map_bytes_, PROT_READ, MAP_SHARED, fd_, 0);
            if (map_ptr_ == MAP_FAILED) {
                map_ptr_ = nullptr;
                close_();
                return false;
            }
            header_ = static_cast<const Header*>(map_ptr_);
            return true;
        }

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
        }

        std::string path_;
        int fd_ = -1;
        void* map_ptr_ = nullptr;
        size_t map_bytes_ = 0;
        const Header* header_ = nullptr;
        uint64_t last_seq_ = 0;
};

}  // namespace spqr
