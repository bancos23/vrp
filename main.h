#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>
#include <random>
#include <limits>
#include <unordered_set>
#include <unordered_map>
#include <map>
#include <set>

#include "Structures.h"
#include "Helpers.h"


#include <iomanip>

void addObjectToTir(stTir& tir, stObject& obj, size_t tirId);
stObject generateRandomObject(std::set<uint8_t>& usedDestinationIds);
