#pragma once
#include "ContourData.h"
#include <string>
class RtStructReader {
public:
    RtStructData Read(const std::string& rsFilePath);
    static std::string FindRsFile(const std::string& dicomFolder);
private:
    static std::array<double, 3> ParseColor(const std::string& colorStr);
    static std::vector<std::array<double, 3>> ParsePoints(const std::string& data);
};
