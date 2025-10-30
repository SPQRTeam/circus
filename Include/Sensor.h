#pragma once

#include <msgpack.hpp>
class Sensor {
    virtual void update() = 0;
    virtual msgpack::object serialize(msgpack::zone& z) = 0;
    // virtual void visualize() = 0;
};
