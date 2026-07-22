#pragma once

#include <iostream>

#include "Core/Forming/ManufacturingPass.h"
#include "Core/Forming/HelixOperation.h"
#include "Core/Forming/HelixCurveBuilder.h"
#include "Core/Forming/TubeFormingProcessType.h"
#include "Core/Forming/LocalDeformableRegion.h" 

// =====================================================
// HELIX FORMING PASS BUILDER
//
// Converts one HelixOperation into a ManufacturingPass.
//
// Current scope:
// - geometric helix
// - curvature / torsion output curve
//
// Not included yet:
// - springback
// - material/tension model
// - real helix machine kinematics
// =====================================================

class HelixFormingPassBuilder
{
public:
    static ManufacturingPass buildPass(
        const HelixOperation& op,
        const std::string& name = "Helix forming pass")
    {
        ManufacturingPass pass;

        pass.name =
            name;

        pass.processType =
            TubeFormingProcessType::HelixForming;
        pass.enabled =
			true;   

        pass.placement =
            PassPlacement::append();
        //copy helix process parameters into the pass

		pass.helixLength = op.length;
		pass.helixRadius = op.helixRadius;
		pass.helixPitch = op.pitch;
		pass.helixFeedSpeed = op.feedSpeed;
        auto result =
            HelixCurveBuilder::build(op);

        if (!result.valid)
        {
            std::cerr << "[HELIX PASS ERROR] Cannot build helix pass\n";
            pass.enabled = false;
            pass.completed = false;
            return pass;
        }

        pass.outputCurve.clear();

        pass.outputCurve.addSegment(
            result.segment
        );

        pass.completed =
            true;

        return pass;
    }
};
