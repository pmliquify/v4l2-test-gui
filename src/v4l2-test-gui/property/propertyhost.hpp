// Copyright (c) 2026 Peter Martienssen
// SPDX-License-Identifier: MIT

#pragma once

#include "propertyadapter.hpp"

class PropertyHost {
public:
    virtual ~PropertyHost() = default;
    virtual PropertyAdapter* propertyAdapter() const = 0;
};
