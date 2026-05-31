#pragma once

#include <string>

#include "Core/Geometry/Frame.h"
#include "Core/Math/Vec3D.h"

// =====================================================
// MACHINE PART
//
// Semantic machine/tooling part.
//
// This is model data, not OpenGL data.
// It may reference an external STL mesh asset.
// =====================================================

enum class MachinePartType
{
    BendDie,
    ClampDie,
    PressureDie,
    Mandrel,
    Base,
    ReferenceOnly
};

enum class MachinePartGeometryType
{
    BoxPlaceholder,
    CylinderPlaceholder,
    MeshAsset
};

struct MachinePart
{
    MachinePartType type =
        MachinePartType::ReferenceOnly;

    MachinePartGeometryType geometryType =
        MachinePartGeometryType::BoxPlaceholder;

    Frame frame;

    // Placeholder dimensions.
    Vec3D halfSize = { 10.0, 10.0, 10.0 };
    double radius = 0.0;

    // STL / mesh asset path.
    // Example: "Assets/Machine/bend_die.stl"
    std::string meshPath;

    // Useful if STL export scale needs correction.
    // Prefer keeping FreeCAD export in millimeters,
    // then this stays 1.0.
    double meshScale = 1.0;

    bool visible = true;
};
