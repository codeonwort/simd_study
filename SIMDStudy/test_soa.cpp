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
#include <assert.h>

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

	// SIMD (SSE2)
	{
		Stopwatch watch; // Construction cost for playersOnStack should be included

		for (uint32 i = 0; i < NUM_PLAYERS; ++i)
		{
			PositionComponent* position = players[i]->findComponent<PositionComponent>();
			playersOnStack[i].x = position->x;
			playersOnStack[i].y = position->y;
			playersOnStack[i].z = position->z;
			playersOnStack[i].allegiance = players[i]->getAllegiance();
		}

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

		float elapsed = watch.stop();
		std::cout << "SSE2: " << elapsed << " seconds" << std::endl;
	}

	// SIMD (AVX2)
	{
		Stopwatch watch; // Construction cost for playersOnStack should be included

		for (uint32 i = 0; i < NUM_PLAYERS; ++i)
		{
			PositionComponent* position = players[i]->findComponent<PositionComponent>();
			playersOnStack[i].x = position->x;
			playersOnStack[i].y = position->y;
			playersOnStack[i].z = position->z;
			playersOnStack[i].allegiance = players[i]->getAllegiance();
		}

		const DoorArray& doors = doorArray->getData();
		uint32 numDoors8 = (doors.count + 7) & (~0x7);
		doorArray->resizeOutput(numDoors8);
		for (uint32 d = 0; d < numDoors8; d += 8)
		{
			__m256 door_x = _mm256_load_ps(&doors.x[d]);
			__m256 door_y = _mm256_load_ps(&doors.y[d]);
			__m256 door_z = _mm256_load_ps(&doors.z[d]);
			__m256 door_r2 = _mm256_load_ps(&doors.radiusSquared[d]);
			__m256i door_a = _mm256_load_si256((__m256i*)(&doors.allegiance[d]));

			__m256i state = _mm256_setzero_si256();

			for (uint32 c = 0; c < NUM_PLAYERS; ++c)
			{
				__m256 player_x = _mm256_broadcast_ss(&playersOnStack[c].x);
				__m256 player_y = _mm256_broadcast_ss(&playersOnStack[c].y);
				__m256 player_z = _mm256_broadcast_ss(&playersOnStack[c].z);
				__m256i player_a = _mm256_set1_epi32(playersOnStack[c].allegiance);

				__m256 ddx = _mm256_sub_ps(door_x, player_x);
				__m256 ddy = _mm256_sub_ps(door_y, player_y);
				__m256 ddz = _mm256_sub_ps(door_z, player_z);
				__m256 dtx = _mm256_mul_ps(ddx, ddx);
				__m256 dty = _mm256_mul_ps(ddy, ddy);
				__m256 dtz = _mm256_mul_ps(ddz, ddz);
				__m256 dst_2 = _mm256_add_ps(_mm256_add_ps(dtx, dty), dtz);

				__m256 rmask = _mm256_cmp_ps(dst_2, door_r2, _CMP_LE_OQ); // There is no _mm256_cmple_ps()
				__m256i amask = _mm256_cmpeq_epi32(player_a, door_a);
				__m256i mask = _mm256_and_si256(_mm256_castps_si256(rmask), amask);
				state = _mm256_or_si256(mask, state);
				
				_mm256_store_si256((__m256i*)(&doors.shouldBeOpen[d]), state);
			}
		}

		float elapsed = watch.stop();
		std::cout << "AVX2: " << elapsed << " seconds" << std::endl;
	}
}
