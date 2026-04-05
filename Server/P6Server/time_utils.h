#ifndef TIME_UTILS_H
#define TIME_UTILS_H

#include <string>
#include <ctime>

bool ParseTimestamp(const char* timestampText, std::tm& outTime);
double GetSecondsBetween(const std::tm& earlier, const std::tm& later);

#endif