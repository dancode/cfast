// ============================================================================
// game_types.h - Shared type definitions
// ============================================================================

#ifndef GAME_TYPES_H
#define GAME_TYPES_H

#include <stdint.h>

// Actual C structs - shared between modules
typedef struct Vec3
{
    float x, y, z;
} Vec3;

typedef struct Transform
{
    Vec3  position;
    Vec3  rotation;
    float scale;

} Transform;

typedef struct Health
{
    float current;
    float maximum;
    float regen_rate;

} Health;

typedef struct Player
{
    uint32_t  id;
    char      name[ 32 ];
    Transform transform;
    Health    health;
    float     speed;
    uint32_t  flags;

} Player;

// Field indices - the "contract" for fast access
enum
{
    PLAYER_ID        = 0,
    PLAYER_NAME      = 1,
    PLAYER_TRANSFORM = 2,
    PLAYER_HEALTH    = 3,
    PLAYER_SPEED     = 4,
    PLAYER_FLAGS     = 5,
};

#endif    // GAME_TYPES_H