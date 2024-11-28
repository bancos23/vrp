#pragma once

struct stObject 
{
    std::string name;
    uint8_t destinationId;
    std::string destinationName;
    float destLong, destLat;
    uint16_t weight;
    uint8_t assignedTir = 0;
};

struct stTir 
{
    uint16_t totalWeight = 0;
    uint16_t numDeliveries = 0;
    std::vector<stObject> objects;
};

struct stIndivid 
{
    std::vector<stTir> tirs;
    std::vector<stObject> objects;
    float fitness = MAXFLOAT;
    uint8_t lifeSpan = 4;
};

struct stAparitii
{
    uint8_t destinationId;
    uint8_t aparitii;
    uint8_t pos;
};

