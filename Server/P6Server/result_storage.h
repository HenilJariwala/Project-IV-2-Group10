/**
 * @file result_storage.h
 * @brief Declares the function used to save completed flight results.
 * @author Mohammad Aljabery
 */

#ifndef RESULT_STORAGE_H
#define RESULT_STORAGE_H

#include "flight_result.h"

 /**
  * @brief Saves a completed flight result to persistent storage.
  * @param result The flight result to be saved.
  */
void SaveFlightResult(const FlightResult& result);

#endif