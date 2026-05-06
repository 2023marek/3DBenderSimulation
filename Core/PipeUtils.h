#pragma once
#include <vector>
#include "PipeAxis3D.h"

// Clips nodes to given arc-length
std::vector<PipeAxis3D::Node>
clipByLength(const std::vector<PipeAxis3D::Node>& nodes, double maxLength);