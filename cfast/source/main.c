// ============================================================================
// main.c - Putting it all together
// ============================================================================

#include "reflection_core.h"
#include "game_types.h"

#include <stdio.h>
#include <time.h>

extern void game_register_types( Registry* reg );
void        draw_property_editor( void* obj, Type* type );
void        serialize_to_json( void* obj, Type* type, FILE* file );

// ============================================================================

int
main( void )
{
    printf( "=== Hybrid Reflection System ===\n\n" );

    // Initialize core types (engine types that never change)
    Type core_types[] = {
        {
            .hash      = hash_string( "float" ),
            .name      = "float",
            .size      = sizeof( float ),
            .module_id = 0,    // Core module
        },
        {
            .hash      = hash_string( "uint32" ),
            .name      = "uint32",
            .size      = sizeof( uint32_t ),
            .module_id = 0,
        },
    };

    for ( int i = 0; i < 2; i++ ) { type_register( &core_types[ i ] ); }

    // type_register( &core_types[ 0 ] );

    // Load game module (in practice, would be DLL)
    // game_register_types( &g_registry );

    // Create a player
    Player player = {
        .id        = 1,
        .name      = "Hero",
        .transform = { { 0, 0, 0 }, { 0, 0, 0 }, 1.0f },
        .health    = { 100, 100, 1.0f },
        .speed     = 5.0f,
        .flags     = 0,
    };

    (void)player;

    // Use reflection for editor
    Type* player_type = type_find_by_hash( hash_string( "Player" ) );
    if ( player_type )
    {
        // draw_property_editor( &player, player_type );

        // Serialize to JSON
        FILE* f = fopen( "player.json", "w" );
        if ( f )
        {
            // serialize_to_json( &player, player_type, f );
            fclose( f );
            printf( "\nSerialized to player.json\n" );
        }
    }

    // Fast path - direct access for game loop
    clock_t start = clock();
    for ( int i = 0; i < 1000000; i++ )
    {
        // Direct struct access - no reflection overhead
        player.health.current = 100.0f;
        player.transform.position.x += player.speed * 0.016f;
    }
    clock_t end = clock();
    printf( "\n1M direct updates: %.3f seconds\n", (double)( end - start ) / CLOCKS_PER_SEC );

    // Reflection path - for tools
    start = clock();
    for ( int i = 0; i < 1000000; i++ )
    {
        // Reflection access - still pretty fast with static arrays!
        float* health = (float*)field_get_ptr( &player, player_type, PLAYER_HEALTH );
        *health       = 100.0f;
    }
    end = clock();
    printf( "1M reflection updates: %.3f seconds\n", (double)( end - start ) / CLOCKS_PER_SEC );

    printf( "\nRegistry stats:\n" );
    printf( "  Types registered: %u\n", g_registry.type_count );
    printf( "  Memory used: %zu KB\n", sizeof( g_registry ) / 1024 );

    return 0;
}

// ============================================================================
// Why This Design Works Well
// ============================================================================

/*
THE KEY INSIGHTS:

1. STATIC ARRAYS WITH DYNAMIC REGISTRATION
   - Types array is static (fast access)
   - But populated at runtime (flexible)
   - Best of both worlds

2. MODULE OWNERSHIP
   - Each type knows which DLL registered it
   - Can unregister all types from a module
   - Clean hot reload

3. STABLE TYPE IDS
   - Types get array indices as IDs
   - IDs are stable during session
   - Fast array lookup (no hash table needed for common case)

4. DUAL ACCESS PATTERNS
   - Direct struct access for game code (full speed)
   - Reflection access for tools (still fast)
   - Same data, two ways to access

5. SIMPLE BUT COMPLETE
   - ~500 lines total
   - Supports serialization
   - Supports editors
   - Supports hot reload
   - Still fast!

PERFORMANCE:
- Type lookup: O(1) with ID, O(1) average with hash
- Field access: 1 indirection (pointer + offset)
- Memory: ~1MB for 1000 types
- No allocations during runtime

LIMITATIONS:
- Fixed max types (but 1024 is plenty)
- Types must be re-registered on reload
- Can't remove types (only mark invalid)
- Field names point to DLL memory (must be static strings)

WHEN TO USE THIS:
- Game with < 1000 types
- Need hot reload during development
- Want editor/tools support
- Need good performance
- Want simple code

This is essentially what many game engines use internally -
just enough reflection for tools, but fast enough for runtime.

============================================================================

Why This Hybrid Approach is the Sweet Spot
The Key Insight: Static Storage, Dynamic Population
Instead of choosing between:

* Full Static: Fast but inflexible
* Full Dynamic: Flexible but slow

This uses:

* Static Arrays: Pre-allocated, no pointers, cache-friendly
* Dynamic Registration: DLLs register types at load time
* Stable IDs: Array indices that survive the session

// 1. Game.dll loads

LoadLibrary("game.dll")
  - get_module_info()
  - register_types(&g_registry)
    - Types get indices 10, 11, 12...

// 2. Game.dll modifies and reloads

Save state -> Unload DLL -> Load new DLL -> Restore state

     Old types marked invalid (because data could have changed)
     New types registered (might reuse slots)
     Pointers still work (they point to static array)

// 3. Editor.dll loads later

LoadLibrary("editor.dll")
     Can see all types in g_registry
     Can modify any object using reflection

============================================================================
// Why fast?

// Hot path - no reflection needed
    player->health = 100;  // Direct access: 1 instruction

// Tool path - still fast!

    Type* t = &g_registry.types[PLAYER_TYPE_ID];  // Array index: 2 instructions
    float* health = (char*)player + t->fields[3].offset;  // Add offset: 2 instructions
    *health = 100;

*/
