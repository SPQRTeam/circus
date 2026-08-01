#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

#include "ipc/SharedMemoryWriter.h"

namespace spqr {

struct ImageMeta {
        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t channels = 0;
};

class ImageSharedMemoryWriter {
    public:
        void configure(const std::string& path, int width, int height, int channels, int ring_slots = 3) {
            if (width <= 0 || height <= 0 || channels <= 0) {
                throw std::invalid_argument("Invalid shared-memory image configuration");
            }
            const size_t element_count = static_cast<size_t>(width) * static_cast<size_t>(height) * static_cast<size_t>(channels);
            ImageMeta meta{static_cast<uint32_t>(width), static_cast<uint32_t>(height), static_cast<uint32_t>(channels)};
            writer_.configure(path, element_count, meta, ring_slots);
        }

        bool isReady() const {
            return writer_.isReady();
        }

        void write(const std::vector<uint8_t>& frame) {
            writer_.write(frame.data(), frame.size());
        }

    private:
        SharedMemoryWriter<uint8_t, ImageMeta> writer_;
};

}  // namespace spqr
