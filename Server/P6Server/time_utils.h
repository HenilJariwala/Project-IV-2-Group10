/**
 * @file time_utils.h
 * @brief Declares utility functions for parsing timestamps and calculating elapsed time.
 * @author Mohammad Aljabery
 */

#ifndef TIME_UTILS_H
#define TIME_UTILS_H

#include <string>
#include <ctime>

 /**
  * @brief Parses a timestamp string into a tm structure.
  * @param timestampText The timestamp text to parse.
  * @param outTime The tm structure that receives the parsed time values.
  * @return true if the timestamp was parsed successfully, otherwise false.
  */
bool ParseTimestamp(const char* timestampText, std::tm& outTime);

/**
 * @brief Calculates the number of seconds between two time values.
 * @param earlier The earlier time value.
 * @param later The later time value.
 * @return The elapsed time in seconds between the two timestamps.
 */
double GetSecondsBetween(const std::tm& earlier, const std::tm& later);

#endif