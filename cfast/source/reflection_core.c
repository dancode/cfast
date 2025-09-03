// ============================================================================
// reflection_core.c - Implementation (in main.exe)
// ============================================================================

#include "reflection_core.h"

#include <stdio.h>
#include <stdlib.h>

// ============================================================================
// The one global registry
// ============================================================================

Registry g_registry = { 0 };

// ============================================================================
// Register a type into the registry
// ============================================================================

TypeID
type_register( const Type* type_info )
{
    if ( g_registry.type_count >= MAX_TYPES )
    {
        printf( "ERROR: Type limit reached!\n" );
        return 0;
    }

    // TODO: check if type already exists (but could be reload).

    // Find or allocate type slot
    TypeID id = g_registry.type_count++;

    // Copy type info
    g_registry.types[ id ]    = *type_info;
    g_registry.types[ id ].id = id;

    // Update hash map for fast lookup (place in next++ index if used)
    TypeHash hash       = type_info->hash;
    size_t   hash_index = hash % ( MAX_TYPES * 2 );
    while ( g_registry.hash_map[ hash_index ].hash != 0 )
    {
        hash_index = ( hash_index + 1 ) % ( MAX_TYPES * 2 );
    }
    g_registry.hash_map[ hash_index ].hash = hash;
    g_registry.hash_map[ hash_index ].id   = id;

    return id;
}

// ============================================================================
// Type lookup
// ============================================================================

Type*
type_find_by_hash( TypeHash hash )
{
    size_t hash_index = hash % ( HASH_SIZE );
    while ( g_registry.hash_map[ hash_index ].hash != 0 )
    {
        if ( g_registry.hash_map[ hash_index ].hash == hash )
        {
            // get id stored at hash locaiton
            return &g_registry.types[ g_registry.hash_map[ hash_index ].id ];
        }
        hash_index = ( hash_index + 1 ) % ( HASH_SIZE );
    }
    return NULL;
}

Type*
type_get( TypeID id )
{
    if ( id >= g_registry.type_count )
        return NULL;
    return &g_registry.types[ id ];
}

// ============================================================================
// Unregister all types within a module
// ============================================================================

void
type_unregister_module( uint8_t module_id )
{
    // Mark types from this module as invalid
    for ( TypeID i = 0; i < g_registry.type_count; i++ )
    {
        if ( g_registry.types[ i ].module_id == module_id )
        {
            // Clear from hash map
            TypeHash hash       = g_registry.types[ i ].hash;
            size_t   hash_index = hash % ( HASH_SIZE );
            while ( g_registry.hash_map[ hash_index ].hash != hash )
            {
                hash_index = ( hash_index + 1 ) % ( HASH_SIZE );
            }
            g_registry.hash_map[ hash_index ].hash = 0;
            g_registry.hash_map[ hash_index ].id   = 0;
        }
    }
}

// ============================================================================