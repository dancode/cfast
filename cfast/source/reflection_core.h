#ifndef REFLECTION_CORE_H
#define REFLECTION_CORE_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

// -----------------------------------------------------------------------------
// Key Design: Fixed-size arrays, but dynamically populated
// -----------------------------------------------------------------------------

#define MAX_TYPES   1024                 // Fixed limit - but 1024 types is huge
#define MAX_FIELDS  32                   // Max fields per type
#define MAX_MODULES 16                   // Max loaded DLLs
#define HASH_SIZE   ( MAX_TYPES * 2 )    // 2x size for good distribution

typedef uint32_t TypeHash;    // Simple hash for lookup
typedef uint16_t TypeID;      // Index into type array

// Field descriptor - minimal but enough for editors
typedef struct Field
{
    const char* name;       // Points to static string in DLL
    uint16_t    offset;     // Byte offset in struct
    uint16_t    size;       // Size in bytes
    uint16_t    type_id;    // Type of this field
    uint16_t    flags;      // Serializable, editable, etc.

} Field;

// flag = 1     // editables

// -----------------------------------------------------------------------------
// Type descriptor - everything needed for tooling
// -----------------------------------------------------------------------------

typedef struct Type
{
    // Identity
    TypeHash    hash;    // Hash of name (for lookup)
    const char* name;    // Human readable name

    TypeID id;    // Index in array (stable across reloads, assigned a registration)

    // Layout
    uint16_t size;         // sizeof(Type)
    uint16_t alignment;    // alignof(Type)

    // Fields (TODO: make union for type data)
    Field   fields[ MAX_FIELDS ];
    uint8_t field_count;

    // Functions (optional)
    void* ( *create )( void );         // Allocator
    void ( *destroy )( void* obj );    // Destructor
    void ( *serialize )( void* obj, void* stream );

    // Module ownership
    uint8_t module_id;    // Which DLL owns this type
    uint8_t version;      // Type version for hot reload

} Type;

// -----------------------------------------------------------------------------
// The global type registry - static array, no allocation!
// -----------------------------------------------------------------------------

typedef struct Registry
{
    Type     types[ MAX_TYPES ];    // All types
    uint16_t type_count;            // How many registered

    // Fast lookup table
    struct
    {
        TypeHash hash;
        TypeID   id;

    } hash_map[ HASH_SIZE ];    // 2x size for good distribution

    // Module tracking for hot reload
    struct
    {
        void*       handle;        // DLL handle
        const char* path;          // DLL path
        TypeID      type_start;    // First type ID for this module
        uint16_t    type_count;    // Number of types in module

    } modules[ MAX_MODULES ];

    uint8_t module_count;    // Number of dll

} Registry;

// Global registry - single instance in main.exe
extern Registry g_registry;

// -----------------------------------------------------------------------------
// Core API - Simple and fast
// -----------------------------------------------------------------------------

// Basic registration
TypeID type_register( const Type* type_info );
void   type_unregister_module( uint8_t module_id );
Type*  type_find_by_hash( TypeHash hash );
Type*  type_find_by_name( const char* name );
Type*  type_get( TypeID id );

// Fast field access - inlineable
static inline void*
field_get_ptr( void* obj, Type* type, uint8_t field_index )
{
    return (char*)obj + type->fields[ field_index ].offset;
}

// Hash function - simple and fast
static inline TypeHash
hash_string( const char* str )
{
    TypeHash hash = 5381;
    while ( *str ) hash = ( ( hash << 5 ) + hash ) + *str++;
    return hash;
}

// Module interface - what each DLL exports
typedef struct ModuleInfo
{
    const char* name;
    uint16_t    version;
    void ( *register_types )( Registry* reg );
    void ( *unregister_types )( Registry* reg );
    void ( *hot_reload_fixup )( Registry* reg, void* old_data );

} ModuleInfo;

#endif    // REFLECTION_CORE_H