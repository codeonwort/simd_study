#include "test_soa.h"
#include "int_types.h"
#include "stopwatch.h"
#include "actor.h"
#include "component.h"

#include <iostream>
#include <vector>
#include <time.h>

// SIMD
#include <xmmintrin.h>
#include <emmintrin.h>
#include <immintrin.h>

constexpr uint32 MAX_PLAYERS = 512;
constexpr uint32 NUM_PLAYERS = 256;
constexpr uint32 NUM_DOORS = 16384;

// In memory, SOA
struct DoorArray
{
	uint32 count;
	std::vector<float> x;
	std::vector<float> y;
	std::vector<float> z;
	std::vector<float> radiusSquared;
	std::vector<uint32> allegiance;

	// output
	std::vector<uint32> shouldBeOpen;
};

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

class DoorArrayActor : public Actor
{
public:
	DoorArrayActor(uint32 maxDoors)
	{
		doorArray.count = 0;
		doorArray.x.reserve(maxDoors);
		doorArray.y.reserve(maxDoors);
		doorArray.z.reserve(maxDoors);
		doorArray.radiusSquared.reserve(maxDoors);
		doorArray.allegiance.reserve(maxDoors);
		doorArray.shouldBeOpen.reserve(maxDoors);
	}
	void addDoor(float x, float y, float z, float radius, uint32 allegiance)
	{
		doorArray.count += 1;
		doorArray.x.push_back(x);
		doorArray.y.push_back(y);
		doorArray.z.push_back(z);
		doorArray.radiusSquared.push_back(radius * radius);
		doorArray.allegiance.push_back(allegiance);
	}
	void resizeOutput(uint32 size)
	{
		doorArray.shouldBeOpen.resize(size);
	}
	const DoorArray& getData() const { return doorArray; }
private:
	DoorArray doorArray;
};

void test_soa()
{
	std::vector<PlayerActor*> players;
	DoorArrayActor* doorArray;

	srand(time(NULL));

	players.reserve(NUM_PLAYERS);
	for (uint32 i = 0; i < NUM_PLAYERS; ++i)
	{
		PlayerActor* P = new PlayerActor;
		P->setPosition(float(rand() % 32768), float(rand() % 32768), float(rand() % 32768));
		players.push_back(P);
	}
	doorArray = new DoorArrayActor(NUM_DOORS);
	for (uint32 i = 0; i < NUM_DOORS; ++i)
	{
		doorArray->addDoor(
			float(rand() % 32768),
			float(rand() % 32768),
			float(rand() % 32768),
			100.0f,
			rand() % 2);
	}

	// On the stack, AOS
	struct PlayerArray
	{
		float x;
		float y;
		float z;
		uint32 allegiance;
	} playersOnStack[MAX_PLAYERS];
	
	Stopwatch watch; // Construction cost for playersOnStack should be included

	for (uint32 i = 0; i < NUM_PLAYERS; ++i)
	{
		PositionComponent* position = players[i]->findComponent<PositionComponent>();
		playersOnStack[i].x = position->x;
		playersOnStack[i].y = position->y;
		playersOnStack[i].z = position->z;
		playersOnStack[i].allegiance = players[i]->getAllegiance();
	}

	// SIMD
	{
		const DoorArray& doors = doorArray->getData();
		uint32 numDoors4 = (doors.count + 3) & (~0x3);
		doorArray->resizeOutput(numDoors4);
		for (uint32 d = 0; d < numDoors4; d += 4)
		{
			__m128 door_x = _mm_load_ps(&doors.x[d]);
			__m128 door_y = _mm_load_ps(&doors.y[d]);
			__m128 door_z = _mm_load_ps(&doors.z[d]);
			__m128 door_r2 = _mm_load_ps(&doors.radiusSquared[d]);
			__m128i door_a = _mm_load_si128((__m128i*)(&doors.allegiance[d]));

			__m128i state = _mm_setzero_si128();

			for (uint32 c = 0; c < NUM_PLAYERS; ++c)
			{
				__m128 player_x = _mm_broadcast_ss(&playersOnStack[c].x);
				__m128 player_y = _mm_broadcast_ss(&playersOnStack[c].y);
				__m128 player_z = _mm_broadcast_ss(&playersOnStack[c].z);
				__m128i player_a = _mm_set1_epi32(playersOnStack[c].allegiance);

				__m128 ddx = _mm_sub_ps(door_x, player_x);
				__m128 ddy = _mm_sub_ps(door_y, player_y);
				__m128 ddz = _mm_sub_ps(door_z, player_z);
				__m128 dtx = _mm_mul_ps(ddx, ddx);
				__m128 dty = _mm_mul_ps(ddy, ddy);
				__m128 dtz = _mm_mul_ps(ddz, ddz);
				__m128 dst_2 = _mm_add_ps(_mm_add_ps(dtx, dty), dtz);

				__m128 rmask = _mm_cmple_ps(dst_2, door_r2);
				__m128i amask = _mm_cmpeq_epi32(player_a, door_a);
				__m128i mask = _mm_and_si128(_mm_castps_si128(rmask), amask);
				state = _mm_or_si128(mask, state);

				_mm_store_si128((__m128i*)(&doors.shouldBeOpen[d]), state);
			}
		}
	}
	float elapsed = watch.stop();
	std::cout << "test2: " << elapsed << " seconds" << std::endl;
}
