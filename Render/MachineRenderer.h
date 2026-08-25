#pragma once

#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "Render/PipeRenderer.h"
#include "Render/RenderMode.h"
#include "Render/TubeMesh.h"
#include "Render/MachineReferenceRenderer.h"

#include "Core/Machine/MachineRenderData.h"
#include "Core/Machine/MachinePart.h"

#include "Core/Mesh/TriangleMesh.h"
#include "Core/Mesh/StlLoader.h"
#include "Core/Assets/AssetPathResolver.h"
#include "Core/Math/Vec3D.h"


// =====================================================
// MACHINE RENDERER
//
// Owns rendering of machine/tooling visualization.
//
// GLView decides WHEN to call it.
// MachineRenderer decides HOW to draw machine data.
//
// For now:
// - reference lines are drawn from MachineReferenceRenderer
// - STL mesh assets are loaded lazily and cached
// - each visible MeshAsset MachinePart is drawn as a mesh
// =====================================================

class MachineRenderer
{
public:
    void init()
    {
        referenceRenderer.init();
        meshRenderer.init();
    }

    void drawReference(
        const MachineRenderData& data)
    {
        std::vector<std::vector<float>> strips =
            MachineReferenceRenderer::buildLineStrips(data);

        if (strips.empty())
            return;

        referenceRenderer.setMode(RenderMode::LINE);
        referenceRenderer.uploadLineStrips(strips);
        referenceRenderer.draw();
    }

    void drawParts(
        const MachineRenderData& data)
    {
        for (const auto& part : data.parts)
        {
            if (!part.visible)
                continue;

            if (part.geometryType
                != MachinePartGeometryType::MeshAsset)
            {
                continue;
            }

            if (part.meshPath.empty())
                continue;

            const TriangleMesh* mesh =
                getMesh(part.meshPath);

            if (!mesh || mesh->empty())
                continue;

            drawMeshPart(part, *mesh);
        }
    }


    void drawHelixSupport(
        const MachineRenderData& data)
    {
        if (!data.supportVisible)
            return;

        if (!std::isfinite(
            data.supportOuterRadius
        ))
        {
            return;
        }

        if (data.supportOuterRadius <= 0.0)
            return;

        Vec3D axisDirection =
            data.supportAxisFrame.T;

        if (axisDirection.lengthSquared() < 1e-12)
            return;

        axisDirection =
            axisDirection.normalized();

        const Vec3D axisPoint =
            data.supportAxisFrame.P;

        // =====================================================
        // MH1.19C — TEMPORARY SUPPORT LENGTH
        //
        // Only radius and axis are being accepted in this phase.
        // Later derive support length from loaded helix extent.
        // =====================================================

        const double supportLength =
            1500.0;

        const Vec3D supportStart =
            axisPoint
            - axisDirection
            * (
                supportLength * 0.5
                );

        const Vec3D supportEnd =
            axisPoint
            + axisDirection
            * (
                supportLength * 0.5
                );

        // =====================================================
        // STRAIGHT CENTERLINE FOR PROCEDURAL CYLINDER
        // =====================================================

        const std::vector<Vec3D> centers =
        {
            supportStart,
            supportEnd
        };

        const std::vector<Vec3D> tangents =
        {
            axisDirection, 
            axisDirection
        };

        constexpr int radialSegments =
            72;

        // =====================================================
        // GENERATE CYLINDER MESH
        // =====================================================

        supportTubeMesh.generate(
            centers,
            tangents,
            data.supportOuterRadius -0.5,
            radialSegments
        );

        const std::vector<TubeMesh::Vertex>& vertices =
            supportTubeMesh.getVertices();

        const std::vector<unsigned int>& indices =
            supportTubeMesh.getIndices();

        if (vertices.empty()
            || indices.empty())
        {
            return;
        }

        // =====================================================
        // DRAW USING EXISTING MACHINE MESH RENDERER
        // =====================================================

        meshRenderer.setMode(
            RenderMode::MESH
        );

        meshRenderer.uploadMesh(
            vertices,
            indices
        );

        glDisable(GL_CULL_FACE);

        meshRenderer.draw();

        glEnable(GL_CULL_FACE);

        meshRenderer.draw();
    }
private:
    PipeRenderer referenceRenderer;
    PipeRenderer meshRenderer;
    TubeMesh supportTubeMesh;
    std::unordered_map<std::string, TriangleMesh> meshCache;
    std::unordered_set<std::string> failedPaths;

private:
    const TriangleMesh* getMesh(
        const std::string& path)
    {
        if (path.empty())
            return nullptr;

        std::string resolvedPath =
            AssetPathResolver::resolve(path);

        if (resolvedPath.empty())
        {
            failedPaths.insert(path);
            return nullptr;
        }

        auto found =
            meshCache.find(resolvedPath);

        if (found != meshCache.end())
            return &found->second;

        if (failedPaths.find(resolvedPath) != failedPaths.end())
            return nullptr;

        TriangleMesh mesh;

        bool ok =
            StlLoader::load(resolvedPath, mesh);

        if (!ok || mesh.empty())
        {
            failedPaths.insert(resolvedPath);
            return nullptr;
        }

        auto inserted =
            meshCache.emplace(resolvedPath, std::move(mesh));

        return &inserted.first->second;
    }

    void drawMeshPart(
        const MachinePart& part,
        const TriangleMesh& mesh)
    {
        std::vector<TubeMesh::Vertex> vertices;
        std::vector<unsigned int> indices;

        vertices.reserve(mesh.triangles.size() * 3);
        indices.reserve(mesh.triangles.size() * 3);

        for (const auto& tri : mesh.triangles)
        {
            unsigned int baseIndex =
                static_cast<unsigned int>(vertices.size());

            vertices.push_back(
                makeVertex(part, tri.v0, tri.normal)
            );

            vertices.push_back(
                makeVertex(part, tri.v1, tri.normal)
            );

            vertices.push_back(
                makeVertex(part, tri.v2, tri.normal)
            );

            indices.push_back(baseIndex + 0);
            indices.push_back(baseIndex + 1);
            indices.push_back(baseIndex + 2);
        }

        if (vertices.empty() || indices.empty())
            return;

        meshRenderer.setMode(RenderMode::MESH);

        meshRenderer.uploadMesh(
            vertices,
            indices
        );

        meshRenderer.draw();
    }

    TubeMesh::Vertex makeVertex(
        const MachinePart& part,
        const Vec3D& localPosition,
        const Vec3D& localNormal) const
    {
        TubeMesh::Vertex v;

        Vec3D worldPosition =
            transformPoint(part, localPosition);

        Vec3D worldNormal =
            transformDirection(part, localNormal);

        v.position[0] =
            static_cast<float>(worldPosition.x);

        v.position[1] =
            static_cast<float>(worldPosition.y);

        v.position[2] =
            static_cast<float>(worldPosition.z);

        v.normal[0] =
            static_cast<float>(worldNormal.x);

        v.normal[1] =
            static_cast<float>(worldNormal.y);

        v.normal[2] =
            static_cast<float>(worldNormal.z);

        return v;
    }

    Vec3D transformPoint(
        const MachinePart& part,
        const Vec3D& localPoint) const
    {
        Vec3D T =
            part.frame.T.normalized();

        Vec3D N =
            part.frame.N.normalized();

        Vec3D B =
            part.frame.B.normalized();

        if (T.lengthSquared() < 1e-12)
            T = { 1.0, 0.0, 0.0 };

        if (N.lengthSquared() < 1e-12)
            N = { 0.0, 1.0, 0.0 };

        if (B.lengthSquared() < 1e-12)
            B = { 0.0, 0.0, 1.0 };

        double s =
            part.meshScale;

        if (s <= 0.0)
            s = 1.0;

        return part.frame.P
            + T * (localPoint.x * s)
            + N * (localPoint.y * s)
            + B * (localPoint.z * s);
    }

    Vec3D transformDirection(
        const MachinePart& part,
        const Vec3D& localDirection) const
    {
        Vec3D T =
            part.frame.T.normalized();

        Vec3D N =
            part.frame.N.normalized();

        Vec3D B =
            part.frame.B.normalized();

        if (T.lengthSquared() < 1e-12)
            T = { 1.0, 0.0, 0.0 };

        if (N.lengthSquared() < 1e-12)
            N = { 0.0, 1.0, 0.0 };

        if (B.lengthSquared() < 1e-12)
            B = { 0.0, 0.0, 1.0 };

        Vec3D result =
            T * localDirection.x
            + N * localDirection.y
            + B * localDirection.z;

        if (result.lengthSquared() < 1e-12)
            return { 0.0, 0.0, 1.0 };

        return result.normalized();
    }
};