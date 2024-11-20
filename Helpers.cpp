#include <main.h>

namespace Helpers
{
	int getRandomNumber(int MIN, int MAX)
	{
		return MIN + rand() % ((MAX + 1) - MIN);
	}

	float calculateDistance(const stObject& obj1, const stObject& obj2)
	{
		float dx = obj2.destLong - obj1.destLong;
		float dy = obj2.destLat - obj1.destLat;
		return sqrt(dx * dx + dy * dy);
	}
}