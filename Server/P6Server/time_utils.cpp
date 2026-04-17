/**
 * @file time_utils.cpp
 * @brief Implements utility functions for timestamp parsing and elapsed time calculation.
 * @author Mohammad Aljabery
 */

#include "time_utils.h"
#include <iomanip>
#include <sstream>

 /**
  * @brief Parses a timestamp string into a tm structure.
  * @param timestampText The timestamp text to parse.
  * @param outTime The tm structure that receives the parsed date and time values.
  * @return true if the timestamp was parsed successfully, otherwise false.
  */
bool ParseTimestamp(const char* timestampText, std::tm& outTime)
{
    std::istringstream input(timestampText);
    input >> std::get_time(&outTime, "%m_%d_%Y %H:%M:%S");

    return !input.fail();
}

/**
 * @brief Calculates the number of seconds between two time values.
 * @param earlier The earlier timestamp.
 * @param later The later timestamp.
 * @return The elapsed time in seconds between the two timestamps.
 */
double GetSecondsBetween(const std::tm& earlier, const std::tm& later)
{
    std::tm earlierCopy = earlier;
    std::tm laterCopy = later;

    std::time_t earlierTime = std::mktime(&earlierCopy);
    std::time_t laterTime = std::mktime(&laterCopy);

    return std::difftime(laterTime, earlierTime);
}