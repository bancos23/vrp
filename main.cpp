#include <main.h>

constexpr auto MAX_INDIVIDS = 100;
constexpr auto MAX_DELIVERIES = 20   ;
constexpr auto MAX_WEIGHT_TIR = 25000;
constexpr auto MAX_GENERATIONS = 10;
constexpr auto MAX_TOURNAMENT_SIZE = 5;

#define DEBUG

#ifdef DEBUG
#define DEBUG_LOG(fmt, ...) printf(fmt, __VA_ARGS__)
#else
#define DEBUG_LOG(fmt, ...)
#endif

std::random_device rd;
std::mt19937 g(rd());

void addObjectToTir(stTir& tir, stObject& obj, size_t tirId) 
{
    obj.assignedTir = tirId;
    tir.totalWeight += obj.weight;
    tir.numDeliveries++;
    tir.objects.push_back(obj);

    DEBUG_LOG("Added object %s [Weight: %d] to Tir %d [Total Weight: %d]\n", obj.name.c_str(), obj.weight, tirId, tir.totalWeight);
}

stObject generateRandomObject(std::set<uint8_t>& usedDestinationIds)
{
    stObject obj;
    do {
        obj.destinationId = Helpers::getRandomNumber(0, 255);
    } while (usedDestinationIds.count(obj.destinationId));
    usedDestinationIds.insert(obj.destinationId);

    obj.name = "Object " + std::to_string(obj.destinationId);
    obj.destinationName = "Destination " + std::to_string(obj.destinationId);
    obj.destLong = static_cast<float>(rand() % 180) - 90.f;
    obj.destLat = static_cast<float>(rand() % 360) - 180.f;
    obj.weight = Helpers::getRandomNumber(1, 5000);

    DEBUG_LOG("%d ", obj.destinationId);

    return obj;
}

void initializePopulation(std::vector<std::unique_ptr<stIndivid>>& population, std::vector<stObject>& objectList)
{
    std::set<uint8_t> usedDestinationIds;

    DEBUG_LOG("Existing objects: ");
    for (uint8_t i = 0; i < MAX_DELIVERIES; ++i)
        objectList.push_back(generateRandomObject(usedDestinationIds));
    DEBUG_LOG("\n");

    for (uint16_t i = 0; i < MAX_INDIVIDS; ++i)
    {
        std::shuffle(objectList.begin(), objectList.end(), g);
        auto individ = std::make_unique<stIndivid>();
        uint8_t currentTir = 0;

        DEBUG_LOG("Individ %d -> number of deliveries: %d, content: \n", i, MAX_DELIVERIES);

        for (auto& obj : objectList)
        {
            if (individ->tirs.size() <= currentTir)
                individ->tirs.emplace_back();

            stTir& current = individ->tirs[currentTir];
            if (current.totalWeight + obj.weight <= MAX_WEIGHT_TIR) 
            {
                addObjectToTir(current, obj, currentTir);
                individ->objects.push_back(obj);
            }
            else
            {
                DEBUG_LOG("Tir %d is full, moving to the next one\n", currentTir);
                currentTir++;
                if (individ->tirs.size() <= currentTir)
                    individ->tirs.emplace_back();

				addObjectToTir(individ->tirs[currentTir], obj, currentTir);
                individ->objects.push_back(obj);
            }
        }
        population.push_back(std::move(individ));
    }
}

float calculateFitness(stIndivid& individ)
{
    float totalFitness = 0.0f;
    for (const auto& tir : individ.tirs)
        for (size_t j = 0; j < tir.objects.size(); ++j)
            for (size_t k = j + 1; k < tir.objects.size(); ++k)
                totalFitness += Helpers::calculateDistance(tir.objects[j], tir.objects[k]);

    individ.fitness = totalFitness;
    return totalFitness;
}

void printIndivid(stIndivid& individ, const std::string& name)
{
    DEBUG_LOG("%s:\n", name.c_str());
    DEBUG_LOG("Fitness: %f\n", individ.fitness);
    for (size_t i = 0; i < individ.tirs.size(); ++i)
    {
        const stTir& tir = individ.tirs[i];
        DEBUG_LOG("  Tir %zu [Total Weight: %d, Num Deliveries: %d]:\n", i, tir.totalWeight, tir.numDeliveries);
        for (const auto& obj : tir.objects)
            DEBUG_LOG("    Object %s [Weight: %d, Destination: %s (%.2f, %.2f)]\n", obj.name.c_str(), obj.weight, obj.destinationName.c_str(), obj.destLong, obj.destLat);
    }
}

void mutate(stIndivid& child, std::vector<uint8_t>& availableIds, const std::vector<stObject>& objectList)
{
    std::vector<stObject> shadowObjects = child.objects;
    std::map<uint8_t, stAparitii> apparitionMap;

    for (size_t i = 0; i < shadowObjects.size(); ++i)
    {
        uint8_t destId = shadowObjects[i].destinationId;
        if (apparitionMap.find(destId) == apparitionMap.end())
            apparitionMap[destId] = { destId, 1, static_cast<uint8_t>(i) };
        else
        {
            apparitionMap[destId].aparitii++;
            apparitionMap[destId].pos = static_cast<uint8_t>(i);
        }
    }

    DEBUG_LOG("Apparitions:\n");
    for (const auto& [key, value] : apparitionMap)
    {
        if (value.aparitii != 1 && !availableIds.empty())
        {
            DEBUG_LOG("DestinationId: %d, Count: %d, Last Position: %d\n", value.destinationId, value.aparitii, value.pos);
            int id = availableIds[0];
            auto it = std::find_if(objectList.begin(), objectList.end(),
                [id](const stObject& obj) {
                    return obj.destinationId == id;
                });

            if (it != objectList.end())
            {
                DEBUG_LOG("Swapping object %s with object %s\n", shadowObjects[value.pos].name.c_str(), it->name.c_str());
                shadowObjects[value.pos] = *it;
                availableIds.erase(availableIds.begin());
            }
            else
                DEBUG_LOG("Error: Could not find object with DestinationId %d in objectList\n", id);
        }
    }

    DEBUG_LOG("Mutated child (before reallocation):\n");
    for (const stObject& obj : shadowObjects)
        DEBUG_LOG("Object %s [Weight: %d]\n", obj.name.c_str(), obj.weight);

    child.tirs.clear();
    child.objects.clear();

    uint8_t currentTir = 0;
    for (auto& obj : shadowObjects)
    {
        if (child.tirs.size() <= currentTir)
            child.tirs.emplace_back();

        stTir& current = child.tirs[currentTir];
        if (current.totalWeight + obj.weight <= MAX_WEIGHT_TIR) 
        {
			addObjectToTir(current, obj, currentTir);
            child.objects.push_back(obj);
            DEBUG_LOG("Added object %s [Weight: %d] to Tir %d [Total Weight: %d]\n", obj.name.c_str(), obj.weight, currentTir, current.totalWeight);
        }
        else
        {
            DEBUG_LOG("Tir %d is full, moving to the next one\n", currentTir);
            currentTir++;
            if (child.tirs.size() <= currentTir)
                child.tirs.emplace_back();

			addObjectToTir(child.tirs[currentTir], obj, currentTir);
            child.objects.push_back(obj);
            DEBUG_LOG("Added object %s [Weight: %d] to Tir %d [Total Weight: %d]\n", obj.name.c_str(), obj.weight, currentTir, current.totalWeight);
        }
    }
}

std::pair<std::unique_ptr<stIndivid>, std::unique_ptr<stIndivid>> crossover(const stIndivid& parent1, const stIndivid& parent2, const std::vector<stObject>& objectList)
{
    auto child1 = std::make_unique<stIndivid>();
    auto child2 = std::make_unique<stIndivid>();

    size_t crossoverPoint = Helpers::getRandomNumber(0, MAX_DELIVERIES - 1);
	DEBUG_LOG("Crossover point: %d\n", crossoverPoint);

    child1->objects.insert(child1->objects.end(), parent1.objects.begin(), parent1.objects.begin() + crossoverPoint);
    child1->objects.insert(child1->objects.end(), parent2.objects.begin() + crossoverPoint, parent2.objects.end());

    child2->objects.insert(child2->objects.end(), parent2.objects.begin(), parent2.objects.begin() + crossoverPoint);
    child2->objects.insert(child2->objects.end(), parent1.objects.begin() + crossoverPoint, parent1.objects.end());

    DEBUG_LOG("First child:\n");
    for (const stObject& obj : child1->objects)
        DEBUG_LOG("Object %s [Weight: %d]\n", obj.name.c_str(), obj.weight);

    DEBUG_LOG("Second child:\n");
    for (const stObject& obj : child2->objects)
        DEBUG_LOG("Object %s [Weight: %d]\n", obj.name.c_str(), obj.weight);

    std::vector<uint8_t> allIds, child1Ids, child2Ids;
    for (const auto& obj : objectList) allIds.push_back(obj.destinationId);
    for (const auto& obj : child1->objects) child1Ids.push_back(obj.destinationId);
    for (const auto& obj : child2->objects) child2Ids.push_back(obj.destinationId);

    std::sort(allIds.begin(), allIds.end());
    std::sort(child1Ids.begin(), child1Ids.end());
    std::sort(child2Ids.begin(), child2Ids.end());

    std::vector<uint8_t> missingInChild1, missingInChild2;
    std::set_difference(allIds.begin(), allIds.end(), child1Ids.begin(), child1Ids.end(), std::back_inserter(missingInChild1));
    std::set_difference(allIds.begin(), allIds.end(), child2Ids.begin(), child2Ids.end(), std::back_inserter(missingInChild2));

    DEBUG_LOG("First child missing elements: ");
    for (uint8_t id : missingInChild1)
        DEBUG_LOG("%d ", id);
    DEBUG_LOG("\n");

    DEBUG_LOG("Second child missing elements: ");
    for (uint8_t id : missingInChild2)
        DEBUG_LOG("%d ", id);
    DEBUG_LOG("\n");

    mutate(*child1, missingInChild1, objectList);
    mutate(*child2, missingInChild2, objectList);

    return { std::move(child1), std::move(child2) };
}

void mutateIndividual(stIndivid& individ, float mutationRate) 
{
    for (auto& tir : individ.tirs) 
    {
        if ((float)rand() / RAND_MAX < mutationRate) 
        {
            if (tir.objects.size() > 1) 
            {
                size_t idx1 = rand() % tir.objects.size();
                size_t idx2 = rand() % tir.objects.size();

                while (idx1 == idx2) {
                    idx2 = rand() % tir.objects.size();
                }

                std::swap(tir.objects[idx1], tir.objects[idx2]);
                DEBUG_LOG("Swapped objects %d and %d in Tir\n", idx1, idx2);
            }
        }
    }

    if ((float)rand() / RAND_MAX < mutationRate) {
        size_t tir1 = rand() % individ.tirs.size();
        size_t tir2 = rand() % individ.tirs.size();

        while (tir1 == tir2) {
            tir2 = rand() % individ.tirs.size();
        }

        if (!individ.tirs[tir1].objects.empty() && tir1 != tir2) {
            size_t objIdx = rand() % individ.tirs[tir1].objects.size();
            auto obj = individ.tirs[tir1].objects[objIdx];
            individ.tirs[tir1].objects.erase(individ.tirs[tir1].objects.begin() + objIdx);
            individ.tirs[tir1].numDeliveries--;
            addObjectToTir(individ.tirs[tir2], obj, tir2);
            DEBUG_LOG("Moved object to Tir %zu\n", tir2);
        }
    }
}

auto createParent(std::vector<std::unique_ptr<stIndivid>>& population)
{
	std::vector<size_t> selectedParents;
	
    for (uint8_t i = 0; i < MAX_TOURNAMENT_SIZE; i++)
	{
		size_t randomParent = Helpers::getRandomNumber(0, population.size() - 1);
		selectedParents.push_back(randomParent);
	}
	
    std::sort(selectedParents.begin(), selectedParents.end(), [&population](size_t a, size_t b) {
		return population[a]->fitness < population[b]->fitness;
		});
	
    for (uint8_t i = 0; i < MAX_TOURNAMENT_SIZE; i++)
		DEBUG_LOG("Parent %d: %d (%.2f)\n", i, selectedParents[i], population[selectedParents[i]]->fitness);

	return selectedParents[0];
}

void selectParents(std::vector<std::unique_ptr<stIndivid>>& population, const std::vector<stObject>& objectList, float mutationRate)
{
	size_t p1 = createParent(population);
	size_t p2 = createParent(population);

    DEBUG_LOG("Selected parents: %d (%.2f) and %d (%.2f)\n", p1, population[p1]->fitness, p2, population[p2]->fitness);
    population[p1]->lifeSpan--;
	population[p2]->lifeSpan--;

    auto [c1, c2] = crossover(*population[p1], *population[p2], objectList);

    mutateIndividual(*c1, mutationRate);
    mutateIndividual(*c2, mutationRate);

    calculateFitness(*c1);
    calculateFitness(*c2);
    
    printIndivid(*c1, "Child 1");
    printIndivid(*c2, "Child 2");

	population.push_back(std::move(c1));
	population.push_back(std::move(c2));

    population.erase(std::remove_if(population.begin(), population.end(),
        [](const std::unique_ptr<stIndivid>& individ) {
            return individ->lifeSpan < 1;
        }), population.end());
}

int main()
{
    static float bestFitness = FLT_MAX;
    srand(static_cast<unsigned int>(time(nullptr)));

    auto generation = new stGeneration();
    std::vector<stObject> objectList;
    
    initializePopulation(generation->population, objectList);

    for (auto& individ : generation->population)
        calculateFitness(*individ);

    const float mutationRate = 0.05f;
    for (size_t i = 0; i < MAX_GENERATIONS; i++) 
    {
        DEBUG_LOG("\n--- Generation %d ---\n", i);
        selectParents(generation->population, objectList, mutationRate);
    }

    for (size_t i = 0; i < generation->population.size() - 1; ++i)
    {
		if (bestFitness > generation->population[i]->fitness)
			bestFitness = generation->population[i]->fitness;
        printf("Individ %d [fitness: %f, lifeSpan: %d]\n", i, generation->population[i]->fitness, generation->population[i]->lifeSpan);
    }

	printf("Best fitness: %f [%.2f mutation rate]\n", bestFitness, mutationRate);
    system("pause");
    return 0;
}