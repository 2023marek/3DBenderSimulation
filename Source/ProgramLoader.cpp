#include "Core/ProgramLoader.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <iostream>
#include <cmath>

#ifndef PI
#define PI 3.14159265358979323846
#endif

std::string ProgramLoader::trim(const std::string& str)
{
    auto start = str.begin();
    while (start != str.end() && std::isspace(*start))
        start++;

    auto end = str.end();
    if (start != end)
    {
        do { end--; } while (std::distance(start, end) > 0 && std::isspace(*end));
        end++;
    }

    return std::string(start, end);
}

std::vector<std::string> ProgramLoader::split(const std::string& str, char delimiter)
{
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string item;

    while (std::getline(ss, item, delimiter))
    {
        std::string trimmed = trim(item);
        if (!trimmed.empty())
            tokens.push_back(trimmed);
    }

    return tokens;
}

Operation ProgramLoader::parseLine(const std::string& line, int lineNumber)
{
    Operation op;
    std::string trimmedLine = trim(line);

    // Skip empty lines and comments
    if (trimmedLine.empty() || trimmedLine[0] == '#')
    {
        op.type = Operation::FEED;
        op.length = 0.0; // Skip marker
        return op;
    }

    std::vector<std::string> tokens = split(trimmedLine, ' ');
    if (tokens.empty())
        return op;

    std::string command = tokens[0];
    std::transform(command.begin(), command.end(), command.begin(), ::toupper);

    try
    {
        if (command == "FEED")
        {
            if (tokens.size() < 2)
            {
                std::cerr << "Line " << lineNumber << ": FEED requires length\n";
                op.type = Operation::FEED;
                op.length = 0.0;
                return op;
            }

            op.type = Operation::FEED;
            op.length = std::stod(tokens[1]);

            if (op.length < 0.0)
            {
                std::cerr << "Line " << lineNumber << ": FEED length must be positive\n";
                op.length = 0.0;
            }
        }
        else if (command == "BEND")
        {
            if (tokens.size() < 3)
            {
                std::cerr << "Line " << lineNumber << ": BEND requires radius and angle\n";
                return op;
            }

            op.type = Operation::BEND;
            op.R = std::stod(tokens[1]);
            op.angle = std::stod(tokens[2]) * PI / 180.0; // degrees ? radians

            if (tokens.size() >= 4)
            {
                std::string dir = tokens[3];
                std::transform(dir.begin(), dir.end(), dir.begin(), ::toupper);
                op.dir = (dir == "CW") ? BendDirection::CW : BendDirection::CCW;
            }

            // Validation
            if (op.R <= 0.0)
            {
                std::cerr << "Line " << lineNumber << ": BEND radius must be positive\n";
                return op;
            }
            if (op.angle < 0.0)
            {
                std::cerr << "Line " << lineNumber << ": BEND angle must be positive\n";
                op.angle = 0.0;
            }
        }
        else if (command == "ROTATE")
        {
            if (tokens.size() < 2)
            {
                std::cerr << "Line " << lineNumber << ": ROTATE requires angle\n";
                return op;
            }

            op.type = Operation::BEND;
            op.angle = std::stod(tokens[1]) * PI / 180.0;
            op.R = 1e9; // Effectively infinite (pure twist)

            if (op.angle < 0.0)
            {
                std::cerr << "Line " << lineNumber << ": ROTATE angle must be positive\n";
                op.angle = 0.0;
            }
        }
        else if (command == "CLAMP")
        {
            op.type = Operation::FEED;
            op.length = 0.0; // No movement, just state change
            // TODO: Add auxiliary operation type
        }
        else if (command == "UNCLAMP")
        {
            op.type = Operation::FEED;
            op.length = 0.0;
            // TODO: Add auxiliary operation type
        }
        else if (command == "RETRACT")
        {
            if (tokens.size() < 2)
            {
                std::cerr << "Line " << lineNumber << ": RETRACT requires length\n";
                return op;
            }

            op.type = Operation::FEED;
            op.length = -std::stod(tokens[1]); // Negative = backwards
        }
        else
        {
            std::cerr << "Line " << lineNumber << ": Unknown command '" << command << "'\n";
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Line " << lineNumber << ": Parse error: " << e.what() << "\n";
        op.type = Operation::FEED;
        op.length = 0.0;
    }

    return op;
}

ProgramLoader::ParseResult ProgramLoader::loadProgram(const std::string& filePath)
{
    ParseResult result;

    // Try to find file in multiple locations
    std::vector<std::string> searchPaths = {
        filePath,                                          // Current directory
        "../" + filePath,                                  // Parent directory
        "../../" + filePath,                               // Two levels up
        "C:/Users/marek/source/repos/pipe_bender_repo/" + filePath  // Absolute path
    };

    std::ifstream file;
    std::string foundPath;

    for (const auto& path : searchPaths)
    {
        file.open(path);
        if (file.is_open())
        {
            foundPath = path;
            std::cout << "?? Found program at: " << foundPath << "\n";
            break;
        }
    }

    if (!file.is_open())
    {
        result.success = false;
        result.errorMessage = "Cannot open file: " + filePath + " (tried multiple paths)";
        std::cerr << "? " << result.errorMessage << "\n";
        return result;
    }

    std::string line;
    int lineNumber = 0;

    std::cout << "?? Loading program: " << foundPath << "\n";

    while (std::getline(file, line))
    {
        lineNumber++;
        Operation op = parseLine(line, lineNumber);

        // Only add meaningful operations
        if (op.type != Operation::FEED || op.length != 0.0)
        {
            result.operations.push_back(op);
        }
    }

    file.close();

    result.success = true;
    result.lineCount = lineNumber;

    std::cout << "? Loaded " << result.operations.size() << " operations from "
        << result.lineCount << " lines\n";

    return result;
}