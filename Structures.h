#pragma once

struct stObject 
{
    std::string name;
    uint8_t destinationId;
    std::string destinationName;
    float destLong;
    float destLat;
    uint16_t weight;
    size_t assignedTir = 0;
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
    float fitness = FLT_MAX;
    uint8_t lifeSpan = 4;
};

struct stGeneration 
{
	std::vector<std::unique_ptr<stIndivid>> population;
};

struct stAparitii
{
    uint8_t destinationId;
    uint8_t aparitii;
    uint8_t pos;
};

