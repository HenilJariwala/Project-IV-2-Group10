#include "time_utils.h"
#include <iomanip>
#include <sstream>

bool ParseTimestamp(const char* timestampText, std::tm& outTime)
{
    std::istringstream input(timestampText);
    input >> std::get_time(&outTime, "%m_%d_%Y %H:%M:%S");

    return !input.fail();
}

double GetSecondsBetween(const std::tm& earlier, const std::tm& later)
{
    std::tm earlierCopy = earlier;
    std::tm laterCopy = later;

    std::time_t earlierTime = std::mktime(&earlierCopy);
    std::time_t laterTime = std::mktime(&laterCopy);

    return std::difftime(laterTime, earlierTime);
}