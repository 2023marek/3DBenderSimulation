#pragma once

#include <vector>

#include "Core/Forming/ManufacturingPass.h"

// =====================================================
// MANUFACTURING PLAN
//
// Ordered list of manufacturing passes.
//
// Future examples:
//
// Pass 1: rotary draw bending
// Pass 2: helix forming
// Pass 3: manual correction
// =====================================================

struct ManufacturingPlan
{
    std::vector<ManufacturingPass> passes;

    void clear()
    {
        passes.clear();
    }

    bool empty() const
    {
        return passes.empty();
    }

    size_t size() const
    {
        return passes.size();
    }

    void addPass(const ManufacturingPass& pass)
    {
        passes.push_back(pass);
    }
}; 
