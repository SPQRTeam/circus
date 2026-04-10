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
#include <vector>

namespace spqr {

class ImageSharedMemoryWriter {
    public:
        ImageSharedMemoryWriter() = default;

        ~ImageSharedMemoryWriter() {
            close_();
        }

        void configure(const std::string& path, int width, int height, int channels, int ring_slots = 3) {
            if (width <= 0 || height <= 0 || channels <= 0 || ring_slots <= 0) {
                throw std::invalid_argument("Invalid shared-memory image configuration");
            }
            // Set the sizes of bytes
            const size_t slot_bytes = static_cast<size_t>(width) * static_cast<size_t>(height) * static_cast<size_t>(channels);
            const size_t total_bytes = sizeof(Header) + static_cast<size_t>(ring_slots) * slot_bytes;

            close_();

            const std::filesystem::path p(path);
            if (p.has_parent_path()) {
                std::filesystem::create_directories(p.parent_path());
            }

            fd_ = open(path.c_str(), O_RDWR | O_CREAT, 0666);
            if (fd_ < 0) {
                throw std::runtime_error("Failed to open shared image file: " + path);
            }

            if (ftruncate(fd_, static_cast<off_t>(total_bytes)) != 0) {
                close_();
                throw std::runtime_error("Failed to resize shared image file: " + path);
            }

            map_bytes_ = total_bytes;
            // map the file in the process memory, map_ptr is the pointer of the memory area linked to that file
            map_ptr_ = mmap(nullptr, map_bytes_, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0);
            if (map_ptr_ == MAP_FAILED) {
                map_ptr_ = nullptr;
                close_();
                throw std::runtime_error("Failed to mmap shared image file: " + path);
            }

            header_ = static_cast<Header*>(map_ptr_);
            if (header_->magic != kMagic || header_->version != kVersion || header_->slot_bytes != slot_bytes
                || header_->slot_count != static_cast<uint32_t>(ring_slots)) {
                std::memset(header_, 0, sizeof(Header));
                header_->magic = kMagic;
                header_->version = kVersion;
                header_->width = static_cast<uint32_t>(width);
                header_->height = static_cast<uint32_t>(height);
                header_->channels = static_cast<uint32_t>(channels);
                header_->slot_count = static_cast<uint32_t>(ring_slots);
                header_->slot_bytes = slot_bytes;
                header_->seq = 0;
            }

            path_ = path;
        }

        bool isReady() const {
            return header_ != nullptr;
        }

        void write(const std::vector<uint8_t>& frame) {
            if (!header_) {
                return;
            }
            if (frame.size() != header_->slot_bytes) {
                return;
            }

            const uint64_t next_seq = __atomic_load_n(&header_->seq, __ATOMIC_RELAXED) + 1;
            const uint32_t slot_idx = static_cast<uint32_t>(next_seq % header_->slot_count);
            uint8_t* slots_base = static_cast<uint8_t*>(map_ptr_) + sizeof(Header);
            uint8_t* dst = slots_base + static_cast<size_t>(slot_idx) * header_->slot_bytes;
            std::memcpy(dst, frame.data(), frame.size());
            __atomic_store_n(&header_->seq, next_seq, __ATOMIC_RELEASE);
        }

    private:
        struct Header {
                uint32_t magic = 0;
                uint32_t version = 0;
                uint32_t width = 0;
                uint32_t height = 0;
                uint32_t channels = 0;
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
