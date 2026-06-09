#pragma once

#include <vector>

#include "Core/Curve/PipeCurveSegment.h"

// =====================================================
// PIPE CURVE
//
// Ordered list of curvature-driven pipe segments.
//
// This is the future common representation for:
// - CAD preview
// - manufacturing output
// - physics correction
// - springback
// - collision sampling
// =====================================================
struct PipeCurveSplitResult;

struct PipeCurveLocation
{
    bool valid = false;

    size_t segmentIndex = 0;

    // Arc length from start of whole curve.
    double globalS = 0.0;

    // Arc length from start of selected segment.
    double localS = 0.0;

    // Segment start/end in global curve coordinates.
    double segmentStartS = 0.0;
    double segmentEndS = 0.0;
};

struct PipeCurve
{

    //Debug helper

    double segmentStartArcLength(size_t index) const
    {
        if (index >= segments.size())
            return 0.0;

        double s = 0.0;

        for (size_t i = 0; i < index; ++i)
        {
            s += segments[i].length;
        }

        return s;
    }


	//======================================================
   
    bool isValidArcLength(double s) const
    {
        return s >= 0.0 && s <= totalLength();
    }

    PipeCurveLocation locateArcLength(double s) const
    {
        PipeCurveLocation location;

        if (segments.empty())
            return location;

        double total =
            totalLength();

        if (s < 0.0 || s > total)
            return location;

        double accumulated =
            0.0;

        for (size_t i = 0; i < segments.size(); ++i)
        {
            const auto& segment =
                segments[i];

            double startS =
                accumulated;

            double endS =
                accumulated + segment.length;

            if (s <= endS || i == segments.size() - 1)
            {
                location.valid = true;
                location.segmentIndex = i;
                location.globalS = s;
                location.segmentStartS = startS;
                location.segmentEndS = endS;
                location.localS = s - startS;

                return location;
            }

            accumulated =
                endS;
        }

        return location;
    }
    
    
    std::vector<PipeCurveSegment> segments;

    void clear()
    {
        segments.clear();
    }

    bool empty() const
    {
        return segments.empty();
    }

    size_t size() const
    {
        return segments.size();
    }

    void addSegment(const PipeCurveSegment& segment)
    {
        segments.push_back(segment);
    }

    void appendCurve(const PipeCurve& other)
    {
        for (const auto& segment : other.segments)
        {
            segments.push_back(segment);
        }
    }

    double totalLength() const
    {
        double total = 0.0;

        for (const auto& segment : segments)
        {
            total += segment.length;
        }

        return total;
    }
    PipeCurveSplitResult splitAtArcLength(double s) const;

};


struct PipeCurveSplitResult
{
    bool valid = false;

    PipeCurve before;
    PipeCurve after;

    PipeCurveLocation location;
};



inline PipeCurveSplitResult PipeCurve::splitAtArcLength( double s) const
{
    // =====================================================
    // CURVE SPLIT AT ARC LENGTH
    //
    // Phase 7O-2:
    // Supports exact split for Line segments only.
    //
    // Later:
    // - CircularArc split
    // - Helix split
    // - VariableCurvature split
    //
    // Current behavior:
    //
    // before = curve before s
    // after  = curve after s
    // =====================================================

    PipeCurveSplitResult result;

    PipeCurveLocation loc =
        locateArcLength(s);

    if (!loc.valid)
        return result;

    result.location =
        loc;

    result.valid =
        true;

    for (size_t i = 0; i < segments.size(); ++i)
    {
        const PipeCurveSegment& segment =
            segments[i];

        if (i < loc.segmentIndex)
        {
            result.before.addSegment(
                segment
            );
        }
        else if (i > loc.segmentIndex)
        {
            result.after.addSegment(
                segment
            );
        }
        else
        {
            // =================================================
            // Split segment containing s.
            // =================================================

            if (segment.type == PipeCurveSegmentType::Line)
            {
                double beforeLength =
                    loc.localS;

                double afterLength =
                    segment.length - loc.localS;

                if (beforeLength > 1e-9)
                {
                    result.before.addSegment(
                        PipeCurveSegment::makeLine(
                            beforeLength
                        )
                    );
                }

                if (afterLength > 1e-9)
                {
                    result.after.addSegment(
                        PipeCurveSegment::makeLine(
                            afterLength
                        )
                    );
                }
            }
            else if (segment.type == PipeCurveSegmentType::CircularArc)
            {
                if (segment.radius <= 1e-9)
                {
                    result.after.addSegment(segment);
                }
                else
                {
                    double beforeLength =
                        loc.localS;

                    double afterLength =
                        segment.length - loc.localS;

                    double beforeAngle =
                        beforeLength / segment.radius;

                    double afterAngle =
                        afterLength / segment.radius;

                    if (beforeAngle > 1e-9)
                    {
                        result.before.addSegment(
                            PipeCurveSegment::makeCircularArc(
                                segment.radius,
                                beforeAngle,
                                segment.bendDirection
                            )
                        );
                    }

                    if (afterAngle > 1e-9)
                    {
                        result.after.addSegment(
                            PipeCurveSegment::makeCircularArc(
                                segment.radius,
                                afterAngle,
                                segment.bendDirection
                            )
                        );
                    }
                }
            }
        }
    }

    return result;
}





