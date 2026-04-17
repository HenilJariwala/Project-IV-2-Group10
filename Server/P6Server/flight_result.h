/**
 * @file flight_result.h
 * @brief Defines the structure used to store the final fuel consumption result of a completed flight.
 * @author Mohammad Aljabery
 */

#ifndef FLIGHT_RESULT_H
#define FLIGHT_RESULT_H

 /**
  * @brief Stores the final average fuel consumption result for a completed flight.
  */
struct FlightResult
{
    unsigned int planeID;
    double averageFuelConsumptionRate;
};

#endif