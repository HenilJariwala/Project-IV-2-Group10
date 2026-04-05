#include "result_storage.h"
#include <fstream>
#include <iostream>

void SaveFlightResult(const FlightResult& result)
{
    std::ofstream outputFile("results.txt", std::ios::app);

    if (!outputFile.is_open())
    {
        std::cerr << "Failed to open results.txt for writing." << std::endl;
        return;
    }

    outputFile << "Plane ID: " << result.planeID
        << ", Average Fuel Consumption: "
        << result.averageFuelConsumptionRate
        << std::endl;

    outputFile.close();

    std::cout << "Flight result saved for plane "
        << result.planeID << std::endl;
}