#pragma once

#include <chrono>
#include <msgpack.hpp>
#include <mutex>
#include <optional>
#include <shared_mutex>

class Sensor {
    public:
        Sensor(std::optional<float> frequency = std::nullopt) {
            if (frequency.has_value()) {
                dt = std::chrono::duration<float>(1.0 / frequency.value());
            } else {
                dt = std::chrono::duration<float>(0);  // Always update
            }
            lastUpdateTime = std::chrono::steady_clock::now();
        }

        virtual ~Sensor() = default;

        virtual void update() final {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = now - lastUpdateTime;

            if (elapsed < dt) {
                return;  // Skip this update
            }

            std::unique_lock lock(mtx_);
            doUpdate();
            lastUpdateTime = now;
        }

        virtual msgpack::object serialize(msgpack::zone& z) final {
            std::shared_lock lock(mtx_);
            return doSerialize(z);
        }

    protected:
        virtual void doUpdate() = 0;
        virtual msgpack::object doSerialize(msgpack::zone& z) = 0;

    private:
        mutable std::shared_mutex mtx_;
        std::chrono::duration<float> dt;
        std::chrono::steady_clock::time_point lastUpdateTime{};
};
