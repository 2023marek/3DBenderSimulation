#pragma once

#include <vector>
#include <cmath>
#include <fstream>
#include <iostream>

static constexpr double PI = 3.14159265358979323846;

struct Vec2
{
    double x = 0.0;
    double y = 0.0;
};

class PipeAxis2D
{
public:

    struct Node
    {
        Vec2 pos;
        double theta = 0.0;
    };

    enum class BendDirection
    {
        CCW = 1,
        CW = -1
    };

    struct Operation
    {
        enum Type
        {
            FEED,
            BEND
        };

        Type type = FEED;

        double length = 0.0;
        double R = 0.0;
        double angle = 0.0;
        BendDirection dir = BendDirection::CCW;
    };

public:

    explicit PipeAxis2D(double segmentLength)
        : ds(segmentLength)
    {
    }

    void addFeed(double L)
    {
        Operation op;
        op.type = Operation::FEED;
        op.length = L;

        ops.push_back(op);
    }

    void addBend(double R, double angle, BendDirection dir)
    {
        Operation op;
        op.type = Operation::BEND;
        op.R = R;
        op.angle = angle;
        op.dir = dir;

        ops.push_back(op);
    }

    void build()
    {
        nodes.clear();

        Vec2 P{ 0.0,0.0 };
        double theta = 0.0;

        nodes.push_back({ P,theta });

        for (const auto& op : ops)
        {
            if (op.type == Operation::FEED)
            {
                buildFeed(P, theta, op.length);
            }
            else
            {
                buildBend(P, theta, op.R, op.angle, op.dir);
            }
        }
    }

    void exportForGnuplot(const std::string& filename) const
    {
        std::ofstream file(filename);

        if (!file)
        {
            std::cerr << "Cannot open " << filename << "\n";
            return;
        }

        file << "# x y theta\n";

        for (const auto& n : nodes)
        {
            file << n.pos.x << " "
                << n.pos.y << " "
                << n.theta << "\n";
        }

        std::cout << "Exported " << nodes.size() << " nodes to " << filename << "\n";
    }

    const std::vector<Node>& getNodes() const
    {
        return nodes;
    }

private:

    std::vector<Node> nodes;
    std::vector<Operation> ops;

    double ds;

private:

    void buildFeed(Vec2& P, double& theta, double length)
    {
        int n = static_cast<int>(std::round(length / ds));

        for (int i = 0; i < n; i++)
        {
            P.x += ds * std::cos(theta);
            P.y += ds * std::sin(theta);

            nodes.push_back({ P,theta });
        }
    }

    void buildBend(Vec2& P,
        double& theta,
        double R,
        double angle,
        BendDirection dir)
    {
        double arcLength = R * angle;

        int n = static_cast<int>(std::round(arcLength / ds));

        if (n <= 0)
            return;

        double dAlpha = angle / n;

        int s = static_cast<int>(dir);

        Vec2 T{ std::cos(theta), std::sin(theta) };
        Vec2 N;

        if (dir == BendDirection::CCW)
            N = { -T.y, T.x };
        else
            N = { T.y, -T.x };

        Vec2 C
        {
            P.x + R * N.x,
            P.y + R * N.y
        };

        double phi0 = std::atan2(P.y - C.y, P.x - C.x);

        for (int i = 1; i <= n; i++)
        {
            double phi = phi0 + s * i * dAlpha;

            P.x = C.x + R * std::cos(phi);
            P.y = C.y + R * std::sin(phi);

            theta += s * dAlpha;

            nodes.push_back({ P,theta });
        }
    }
};
