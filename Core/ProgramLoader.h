#pragma once
#include <string>
#include <vector>
#include "Operations.h"

/// Parses CNC program files (tube_program.txt)
/// 
/// Format:
///   FEED <length_mm>
///   BEND <radius_mm> <angle_degrees> [CCW|CW]
///   ROTATE <angle_degrees>
///   CLAMP
///   UNCLAMP
///   RETRACT <length_mm>
class ProgramLoader
{
public:
    struct ParseResult
    {
        bool success = false;
        std::string errorMessage;
        std::vector<Operation> operations;
        int lineCount = 0;
    };

    /// Load and parse program file
    /// @param filePath Path to program file
    /// @return ParseResult with success flag and operations
    static ParseResult loadProgram(const std::string& filePath);

private:
    /// Parse single line of program
    static Operation parseLine(const std::string& line, int lineNumber);

    // Helpers
    static std::string trim(const std::string& str);
    static std::vector<std::string> split(const std::string& str, char delimiter);
};
