#pragma once

#include <msgpack.hpp>
#include <mutex>
#include <shared_mutex>
#include <type_traits>

class SensorBase {
    public:
        virtual ~SensorBase() = default;

        virtual void update() final {
            std::unique_lock lock(mtx_);
            doUpdate();
        }

        virtual msgpack::object serialize(msgpack::zone& z) final {
            std::shared_lock lock(mtx_);
            return doSerialize(z);
        }

    protected:
        virtual void doUpdate() = 0;
        virtual msgpack::object doSerialize(msgpack::zone& z) = 0;

        mutable std::shared_mutex mtx_;
};

template <typename Data>
class Sensor : public SensorBase {
    public:
        using SensorData = Data;

        static_assert(std::is_trivially_copyable_v<SensorData>, "SensorData must be trivially copyable");

        SensorData serializeSHM() const {
            std::shared_lock lock(mtx_);
            return doSerializeSHM();
        }

    protected:
        virtual SensorData doSerializeSHM() const = 0;
};
