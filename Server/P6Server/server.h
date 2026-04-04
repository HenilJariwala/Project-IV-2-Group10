#ifndef SERVER_H
#define SERVER_H

//the port, change it here if needed:
const int SERVER_PORT = 18080;

//agreed packet format
struct TelemetryPacket
{
    unsigned int planeID;
    char timestamp[32];
    double remainingFuel;
    bool endOfFlight;
};

#endif