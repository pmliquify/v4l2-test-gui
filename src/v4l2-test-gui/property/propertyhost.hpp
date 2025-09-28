#pragma once

#include "propertyadapter.hpp"

class PropertyHost {
public:
    virtual ~PropertyHost() = default;
    virtual PropertyAdapter* propertyAdapter() const = 0;
};
