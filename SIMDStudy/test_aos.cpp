#include "test_aos.h"

#include "stopwatch.h"
#include "actor.h"
#include "component.h"

#include <iostream>

constexpr uint32 NUM_PLAYERS = 256;
constexpr uint32 NUM_DOORS = 16384;

class PositionComponent : public Component
{
public:
	PositionComponent()
		: x(0.0f)
		, y(0.0f)
		, z(0.0f)
	{
	}

	float x;
	float y;
	float z;
};

class PlayerActor : public Actor
{
public:
	PlayerActor()
	{
		position = new PositionComponent;
		addComponent(position);

		allegiance = 0;
	}
	void setPosition(float x, float y, float z)
	{
		position->x = x;
		position->y = y;
		position->z = z;
	}
	inline uint32 getAllegiance() const { return allegiance; }
private:
	PositionComponent* position;
	uint32 allegiance;
};

class DoorActor : public Actor
{
	static float distanceSquared(PositionComponent* A, PositionComponent* B)
	{
		const float dx = A->x - B->x;
		const float dy = A->y - B->y;
		const float dz = A->z - B->z;
		return (dx * dx + dy * dy + dz * dz);
	}
public:
	DoorActor()
	{
		position = new PositionComponent;
		addComponent(position);

		allegiance = 0;
		openDistanceSquared = 100.0f * 100.0f;
	}
	void setPosition(float x, float y, float z)
	{
		position->x = x;
		position->y = y;
		position->z = z;
	}
	void update(const std::vector<PlayerActor*> players)
	{
		bool shouldOpen = false;

		for (PlayerActor* player : players)
		{
			if (player->getAllegiance() == allegiance)
			{
				PositionComponent* playerPosition = player->findComponent<PositionComponent>();
				if (playerPosition)
				{
					if (distanceSquared(playerPosition, position) < openDistanceSquared)
					{
						shouldOpen = true;
						break;
					}
				}
			}
		}
	}
private:
	PositionComponent* position;
	uint32 allegiance;
	float openDistanceSquared;
};

void test_aos()
{
	std::vector<PlayerActor*> players;
	std::vector<DoorActor*> doors;

	srand(time(NULL));

	players.reserve(NUM_PLAYERS);
	for (uint32 i = 0; i < NUM_PLAYERS; ++i)
	{
		PlayerActor* P = new PlayerActor;
		P->setPosition(float(rand() % 32768), float(rand() % 32768), float(rand() % 32768));
		players.push_back(P);
	}
	doors.reserve(NUM_DOORS);
	for (uint32 i = 0; i < NUM_DOORS; ++i)
	{
		DoorActor* D = new DoorActor;
		D->setPosition(float(rand() % 32768), float(rand() % 32768), float(rand() % 32768));
		doors.push_back(D);
	}

	Stopwatch watch;
	{
		for (uint32 i = 0; i < NUM_DOORS; ++i)
		{
			doors[i]->update(players);
		}
	}
	float elapsed = watch.stop();
	std::cout << "test1: " << elapsed << " seconds" << std::endl;
}
