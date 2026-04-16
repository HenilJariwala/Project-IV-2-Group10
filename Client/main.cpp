/**
 * @file main.cpp
 * @brief Entry point for the client application.
 *
 * Handles command-line argument parsing, displays help/usage information,
 * and starts the client execution based on the provided configuration.
 *
 * @author Nathan
 * @date 2026
 */

#include "client_config.h"
#include "network_client.h"

#include <iostream>
#include <string>

 /**
  * @brief Program entry point.
  *
  * Parses command-line arguments, validates input, optionally displays help,
  * and launches the client execution.
  *
  * @param argc Argument count
  * @param argv Argument vector
  * @return Exit code (0 for success, non-zero for failure)
  */
int main(int argc, char* argv[])
{
    ClientConfig config;
    bool showHelp = false;
    std::string errorMessage;

    // Parse and validate command-line arguments before starting the client
    if (!ParseArguments(argc, argv, config, showHelp, errorMessage))
    {
        // Display parsing error and show correct usage format
        std::cerr << errorMessage << std::endl;
        PrintUsage((argc > 0 && argv[0] != nullptr) ? argv[0] : "client.exe");
        return 1;
    }

    // Display usage information if help flag is requested
    if (showHelp)
    {
        PrintUsage((argc > 0 && argv[0] != nullptr) ? argv[0] : "client.exe");
        return 0;
    }

    // Start the client execution using the validated configuration
    return RunClient(config);
}
