/**
 * @file client_config.h
 * @brief Defines client configuration structure and argument parsing interface.
 *
 * Contains the ClientConfig structure and function declarations for parsing
 * command-line arguments and displaying usage information.
 *
 * @author Henil
 * @date 2026
 */

#ifndef CLIENT_CONFIG_H
#define CLIENT_CONFIG_H

#include <string>

 /**
  * @struct ClientConfig
  * @brief Stores configuration parameters for the client.
  */
struct ClientConfig
{
    std::string host;          // Server IP or hostname
    unsigned short port = 18080; // Server port (default: 18080)
    unsigned int startPlaneID = 0; // Starting plane ID
    unsigned int endPlaneID = 0;   // Ending plane ID
};

/**
 * @brief Parses command-line arguments and populates client configuration.
 *
 * Validates input arguments and sets configuration fields accordingly.
 * Also handles help flag and error reporting.
 *
 * @param argc Argument count
 * @param argv Argument vector
 * @param config Output configuration structure
 * @param showHelp Flag indicating if help should be displayed
 * @param errorMessage Output error message in case of failure
 * @return true if parsing is successful, false otherwise
 */

bool ParseArguments(
    int argc,
    char* argv[],
    ClientConfig& config,
    bool& showHelp,
    std::string& errorMessage
);

/**
 * @brief Prints usage instructions for the client application.
 *
 * Displays expected command-line arguments and example usage.
 *
 * @param executableName Name of the executable
 */
void PrintUsage(const char* executableName);

#endif
