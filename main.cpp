#include <main.h>

#define MAX_INDIVIDS 10
#define MAX_DELIVERIES 20   
#define MAX_WEIGHT_TIR 25000

std::random_device rd;
std::mt19937 g(rd());

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

    printf("%d ", obj.destinationId);

    return obj;
}

void initializePopulation(std::vector<std::unique_ptr<stIndivid>>& population, std::vector<stObject>& objectList)
{
    std::set<uint8_t> usedDestinationIds;

    printf("Existing objects: ");
    for (uint8_t i = 0; i < MAX_DELIVERIES; ++i)
        objectList.push_back(generateRandomObject(usedDestinationIds));
    printf("\n");

    for (uint8_t i = 0; i < MAX_INDIVIDS; ++i)
    {
        std::shuffle(objectList.begin(), objectList.end(), g);
        auto individ = std::make_unique<stIndivid>();
        uint8_t currentTir = 0;

        printf("Individ %d -> number of deliveries: %d, content: \n", i, MAX_DELIVERIES);

        for (const auto& obj : objectList)
        {
            if (individ->tirs.size() <= currentTir)
                individ->tirs.emplace_back();

            stTir& current = individ->tirs[currentTir];
            if (current.totalWeight + obj.weight <= MAX_WEIGHT_TIR) {
                stObject newObj = obj;
                newObj.assignedTir = currentTir;
                current.totalWeight += newObj.weight;
                current.numDeliveries++;
                current.objects.push_back(newObj);
                individ->objects.push_back(newObj);

                printf("Added object %s [Weight: %d] to Tir %d [Total Weight: %d]\n", newObj.name.c_str(), newObj.weight, currentTir, current.totalWeight);
            }
            else
            {
                printf("Tir %d is full, moving to the next one\n", currentTir);
                currentTir++;
                if (individ->tirs.size() <= currentTir)
                    individ->tirs.emplace_back();

                stTir& next = individ->tirs[currentTir];
                stObject newObj = obj;
                newObj.assignedTir = currentTir;
                next.totalWeight += newObj.weight;
                next.numDeliveries++;
                next.objects.push_back(newObj);
                individ->objects.push_back(newObj);
                printf("Added object %s [Weight: %d] to Tir %d [Total Weight: %d]\n", newObj.name.c_str(), newObj.weight, currentTir, current.totalWeight);
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

void printIndivid(stIndivid& individ, const std::string& name) {
    std::cout << name << ":\n";
    std::cout << "Fitness: " << individ.fitness << "\n";
    for (size_t i = 0; i < individ.tirs.size(); ++i) {
        const stTir& tir = individ.tirs[i];
        std::cout << "  Tir " << i << " [Total Weight: " << tir.totalWeight
            << ", Num Deliveries: " << tir.numDeliveries << "]:\n";
        for (const auto& obj : tir.objects) {
            std::cout << "    Object " << obj.name << " [Weight: " << obj.weight
                << ", Destination: " << obj.destinationName << " (" << obj.destLong << ", " << obj.destLat << ")]\n";
        }
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

    printf("Apparitions:\n");
    for (const auto& [key, value] : apparitionMap)
    {
        if (value.aparitii != 1 && !availableIds.empty())
        {
            printf("DestinationId: %d, Count: %d, Last Position: %d\n", value.destinationId, value.aparitii, value.pos);
            int id = availableIds[0];
            auto it = std::find_if(objectList.begin(), objectList.end(),
                [id](const stObject& obj) {
                    return obj.destinationId == id;
                });

            if (it != objectList.end())
            {
                printf("Swapping object %s with object %s\n", shadowObjects[value.pos].name.c_str(), it->name.c_str());
                shadowObjects[value.pos] = *it;
                availableIds.erase(availableIds.begin());
            }
            else
                printf("Error: Could not find object with DestinationId %d in objectList\n", id);
        }
    }

    printf("Mutated child (before reallocation):\n");
    for (const stObject& obj : shadowObjects)
        printf("Object %s [Weight: %d]\n", obj.name.c_str(), obj.weight);

    child.tirs.clear();
    child.objects.clear();

    uint8_t currentTir = 0;
    for (const auto& obj : shadowObjects)
    {
        if (child.tirs.size() <= currentTir)
            child.tirs.emplace_back();

        stTir& current = child.tirs[currentTir];
        if (current.totalWeight + obj.weight <= MAX_WEIGHT_TIR) {
            stObject newObj = obj;
            newObj.assignedTir = currentTir;
            current.totalWeight += newObj.weight;
            current.numDeliveries++;
            current.objects.push_back(newObj);
            child.objects.push_back(newObj);
            printf("Added object %s [Weight: %d] to Tir %d [Total Weight: %d]\n", newObj.name.c_str(), newObj.weight, currentTir, current.totalWeight);
        }
        else
        {
            printf("Tir %d is full, moving to the next one\n", currentTir);
            currentTir++;
            if (child.tirs.size() <= currentTir)
                child.tirs.emplace_back();

            stTir& next = child.tirs[currentTir];
            stObject newObj = obj;
            newObj.assignedTir = currentTir;
            next.totalWeight += newObj.weight;
            next.numDeliveries++;
            next.objects.push_back(newObj);
            child.objects.push_back(newObj);
            printf("Added object %s [Weight: %d] to Tir %d [Total Weight: %d]\n", newObj.name.c_str(), newObj.weight, currentTir, current.totalWeight);
        }
    }
}


std::pair<std::unique_ptr<stIndivid>, std::unique_ptr<stIndivid>> crossover(const stIndivid& parent1, const stIndivid& parent2, const std::vector<stObject>& objectList)
{
    auto child1 = std::make_unique<stIndivid>();
    auto child2 = std::make_unique<stIndivid>();

    size_t crossoverPoint = Helpers::getRandomNumber(0, MAX_DELIVERIES - 1);
    printf("Crossover point: %d\n", crossoverPoint);

    child1->objects.insert(child1->objects.end(), parent1.objects.begin(), parent1.objects.begin() + crossoverPoint);
    child1->objects.insert(child1->objects.end(), parent2.objects.begin() + crossoverPoint, parent2.objects.end());

    child2->objects.insert(child2->objects.end(), parent2.objects.begin(), parent2.objects.begin() + crossoverPoint);
    child2->objects.insert(child2->objects.end(), parent1.objects.begin() + crossoverPoint, parent1.objects.end());

    printf("First child:\n");
    for (const stObject& obj : child1->objects)
        printf("Object %s [Weight: %d]\n", obj.name.c_str(), obj.weight);

    printf("Second child:\n");
    for (const stObject& obj : child2->objects)
        printf("Object %s [Weight: %d]\n", obj.name.c_str(), obj.weight);

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

    printf("First child missing elements: ");
    for (uint8_t id : missingInChild1)
        printf("%d ", id);
    printf("\n");


    printf("Second child missing elements: ");
    for (uint8_t id : missingInChild2)
        printf("%d ", id);
    printf("\n");

    mutate(*child1, missingInChild1, objectList);
    mutate(*child2, missingInChild2, objectList);

    return { std::move(child1), std::move(child2) };
}

void selectParents(std::vector<std::unique_ptr<stIndivid>>& population, const std::vector<stObject>& objectList)
{
    size_t p1 = Helpers::getRandomNumber(0, MAX_INDIVIDS - 1);
    size_t p2 = -1;
    do 
    {
        p2 = Helpers::getRandomNumber(0, MAX_INDIVIDS - 1);
    } while (p1 == p2);

	printf("Selected parents: %d (%.2f) and %d (%.2f)\n", p1, population[p1]->fitness, p2, population[p2]->fitness);

    auto [c1, c2] = crossover(*population[p1], *population[p2], objectList);

    printf("%d | %d\n", c1->objects.size(), c2->objects.size());

    calculateFitness(*c1);
    calculateFitness(*c2);
    
    printIndivid(*c1, "Child 1");
    printIndivid(*c2, "Child 2");
}

int main()
{
    static float bestFitness = FLT_MAX;
    srand(static_cast<unsigned int>(time(0)));

    std::vector<std::unique_ptr<stIndivid>> population;
    std::vector<stObject> objectList;

    initializePopulation(population, objectList);

    for (auto& individ : population)
        calculateFitness(*individ);

    selectParents(population, objectList);
    return 0;
}