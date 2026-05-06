#include "GeometryUtils.h"

// ------------------------------------------------------------
// Clip nodes by arc length (keeps partial last segment)
// ------------------------------------------------------------
std::vector<PipeAxis3D::Node>
clipByLength(const std::vector<PipeAxis3D::Node>& nodes,
    double maxLength)
{
    std::vector<PipeAxis3D::Node> result;

    if (nodes.empty()) return result;

    double accumulated = 0.0;

    result.push_back(nodes[0]);

    for (size_t i = 1; i < nodes.size(); i++)
    {
        const auto& prev = nodes[i - 1];
        const auto& curr = nodes[i];

        Vec3D delta = curr.pos - prev.pos;
        double segmentLength = std::sqrt(
            delta.x * delta.x +
            delta.y * delta.y +
            delta.z * delta.z
        );

        // FULL segment fits
        if (accumulated + segmentLength <= maxLength)
        {
            result.push_back(curr);
            accumulated += segmentLength;
        }
        else
        {
            // ?? PARTIAL segment
            double remaining = maxLength - accumulated;

            if (remaining > 0.0 && segmentLength > 0.0)
            {
                double t = remaining / segmentLength;

                PipeAxis3D::Node clipped;

                clipped.pos = prev.pos + (delta * t);
                clipped.T = prev.T + (curr.T - prev.T) * t;

                result.push_back(clipped);
            }

            break;
        }
    }

    return result;
}