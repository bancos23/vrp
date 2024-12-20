#pragma once
#include <iostream>
#include <string>
#include <random>
#include <map>
#include <set>

#include "Structures.h"
#include "Helpers.h"

void addObjectToTir(stTir& tir, stObject& obj, size_t tirId);
stObject generateRandomObject(std::set<uint8_t>& usedDestinationIds);
