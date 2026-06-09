#pragma once

#include <iostream>
#include <string>
#include <vector>

#include "Core/Operations.h"
#include "Core/Curve/PipeCurveSegment.h"
#include "Core/Forming/ManufacturingPass.h"
#include "Core/Forming/TubeFormingProcessType.h"

// =====================================================
// ROTARY DRAW PASS BUILDER
//
// Converts classic FEED / ROTATE / BEND operations into
// a ManufacturingPass with a curvature-driven output curve.
//
// This is the bridge:
//
//      Operation list
//          ?
//      PipeCurve segments
//          ?
//      ManufacturingPass
//
// Important:
// This does NOT simulate manufacturing playback.
// This only builds the ideal curve representation of
// a rotary draw bending pass.
// =====================================================

class RotaryDrawPassBuilder
{
public:
    static ManufacturingPass buildPass(
        const std::vector<Operation>& operations,
        const std::string& name = "Rotary draw bending pass")
    {
        ManufacturingPass pass;

        pass.name =
            name;

        pass.processType =
            TubeFormingProcessType::RotaryDrawBending;
        pass.placement =
            PassPlacement::append();
        pass.operations =
            operations;

        

        pass.outputCurve.clear();

        for (const auto& op : operations)
        {
            if (op.type == Operation::FEED)
            {
                pass.outputCurve.addSegment(
                    PipeCurveSegment::makeLine(
                        op.length
                    )
                );
            }
            else if (op.type == Operation::BEND)
            {
                pass.outputCurve.addSegment(
                    PipeCurveSegment::makeCircularArc(
                        op.R,
                        op.angle,
                        op.bendDirection
                    )
                );
            }
            else if (op.type == Operation::ROTATE)
            {
                pass.outputCurve.addSegment(
                    PipeCurveSegment::makeRotationOnly(
                        op.angle,
                        op.rotationDirection
                    )
                );
            }
            else
            {
                std::cerr
                    << "[ROTARY DRAW PASS WARNING] Unsupported operation type\n";
            }
        }

        pass.completed =
            true;

        return pass;
    }
};
