// ============================================================================
// game_module.c - Game DLL that can be reloaded
// ============================================================================

#include "reflection_core.h"    // reflection data Type defintion
#include "game_types.h"         // reflected game module types defintion

#include <stdio.h>

// Module state - survives hot reload
typedef struct GameState
{
    Player*  players;
    uint32_t player_count;
    float    game_time;

} GameState;

static GameState* g_state = NULL;

// ============================================================================
// Register our types when DLL loads
// ============================================================================

void
game_register_types( Registry* reg )
{
    // Register Vec3
    Type vec3_type = {
        .hash        = hash_string( "Vec3" ),
        .name        = "Vec3",
        .size        = sizeof( Vec3 ),
        .alignment   = _Alignof( Vec3 ),
        .field_count = 3,
        .fields =
            {
                { "x", offsetof( Vec3, x ), sizeof( float ), 0, 0 },
                { "y", offsetof( Vec3, y ), sizeof( float ), 0, 0 },
                { "z", offsetof( Vec3, z ), sizeof( float ), 0, 0 },
            },
        .module_id = 1,    // This module's ID
        .version   = 1,
    };
    TypeID vec3_id = type_register( &vec3_type );

    // Register Transform
    Type transform_type = {
        .hash        = hash_string( "Transform" ),
        .name        = "Transform",
        .size        = sizeof( Transform ),
        .alignment   = _Alignof( Transform ),
        .field_count = 3,
        .fields =
            {
                { "position", offsetof( Transform, position ), sizeof( Vec3 ), vec3_id, 0 },
                { "rotation", offsetof( Transform, rotation ), sizeof( Vec3 ), vec3_id, 0 },
                { "scale", offsetof( Transform, scale ), sizeof( float ), 0, 0 },
            },
        .module_id = 1,
        .version   = 1,
    };
    TypeID transform_id = type_register( &transform_type );

    // Register Health
    Type health_type = {
        .hash        = hash_string( "Health" ),
        .name        = "Health",
        .size        = sizeof( Health ),
        .alignment   = _Alignof( Health ),
        .field_count = 3,
        .fields =
            {
                { "current", offsetof( Health, current ), sizeof( float ), 0, 0 },
                { "maximum", offsetof( Health, maximum ), sizeof( float ), 0, 0 },
                { "regen_rate", offsetof( Health, regen_rate ), sizeof( float ), 0, 0 },
            },
        .module_id = 1,
        .version   = 1,
    };
    TypeID health_id = type_register( &health_type );

    // Register Player
    Type player_type = {
        .hash        = hash_string( "Player" ),
        .name        = "Player",
        .size        = sizeof( Player ),
        .alignment   = _Alignof( Player ),
        .field_count = 6,
        .fields =
            {
                { "id", offsetof( Player, id ), sizeof( uint32_t ), 0, 0 },
                { "name", offsetof( Player, name ), 32, 0, 0 },
                { "transform", offsetof( Player, transform ), sizeof( Transform ), transform_id, 0 },
                { "health", offsetof( Player, health ), sizeof( Health ), health_id, 0 },
                { "speed", offsetof( Player, speed ), sizeof( float ), 0, 1 },    // Flag: editable
                { "flags", offsetof( Player, flags ), sizeof( uint32_t ), 0, 0 },
            },
        .module_id = 1,
        .version   = 2,    // Increment when struct changes
    };
    type_register( &player_type );
}

// ============================================================================
// Called when DLL is hot-reloaded
// ============================================================================

void
game_hot_reload_fixup( Registry* reg, void* old_state )
{
    if ( old_state )    // only if reloaded?
    {
        g_state = (GameState*)old_state;
        printf( "Hot reload: Restored game state (time=%.2f)\n", g_state->game_time );

        // Fix up any pointers if needed.
        // Re-register types with new function pointers.
        game_register_types( reg );
    }
}

// ============================================================================
// Game update - can access types by ID for speed
// ============================================================================

void
game_update( float dt )
{
    if ( !g_state )
        return;

    g_state->game_time += dt;

    Type* player_type = type_find_by_hash( hash_string( "Player" ) );

    for ( uint32_t i = 0; i < g_state->player_count; i++ )
    {
        Player* p = &g_state->players[ i ];

        // Fast path - direct access
        p->health.current += p->health.regen_rate * dt;
        if ( p->health.current > p->health.maximum )
        {
            p->health.current = p->health.maximum;
        }

        // Or via reflection for tools
        float* speed = (float*)field_get_ptr( p, player_type, PLAYER_SPEED );
        p->transform.position.x += *speed * dt;
    }
}

// ============================================================================
// Export module info
// ============================================================================

__declspec( dllexport ) ModuleInfo*
get_module_info( void )
{
    static ModuleInfo info = {
        .name             = "GameModule",
        .version          = 1,
        .register_types   = game_register_types,
        .unregister_types = NULL,
        .hot_reload_fixup = game_hot_reload_fixup,
    };
    return &info;
}

// ============================================================================
// Export state for hot reload
// ============================================================================

__declspec( dllexport ) void*
get_module_state( void )
{
    return g_state;
}

// ============================================================================
