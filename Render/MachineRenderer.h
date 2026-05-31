#pragma once

#include <vector>

#include "Render/PipeRenderer.h"
#include "Render/RenderMode.h"
#include "Render/MachineReferenceRenderer.h"
#include "Core/Machine/MachineRenderData.h"

// =====================================================
// MACHINE RENDERER
//
// Owns rendering of machine/tooling visualization.
//
// GLView decides WHEN to call it.
// MachineRenderer decides HOW to render machine data.
// MachineReferenceRenderer builds the reference geometry.
// =====================================================

class MachineRenderer
{
public:
    void init()
    {
        lineRenderer.init();
    }

    void drawReference(
        const MachineRenderData& data)
    {
        std::vector<std::vector<float>> strips =
            MachineReferenceRenderer::buildLineStrips(data);

        if (strips.empty())
            return;

        lineRenderer.setMode(RenderMode::LINE);
        lineRenderer.uploadLineStrips(strips);
        lineRenderer.draw();
    }

private:
    PipeRenderer lineRenderer;
};