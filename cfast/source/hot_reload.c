// ============================================================================
// hot_reload.c - Hot reload system
// ============================================================================
#define _CRT_SECURE_NO_WARNINGS

#include "reflection_core.h"    // reflection data ModuleInfo defintion
#include <stdio.h>

#ifdef _WIN32
#    include <windows.h>

// ============================================================================

typedef struct Module
{
    HMODULE     handle;
    ModuleInfo* info;
    void*       state;
    const char* path;
    FILETIME    last_write_time;
} Module;

int
check_module_changed( Module* mod )
{
    WIN32_FILE_ATTRIBUTE_DATA data;
    if ( !GetFileAttributesEx( mod->path, GetFileExInfoStandard, &data ) )
    {
        return 0;
    }

    if ( CompareFileTime( &data.ftLastWriteTime, &mod->last_write_time ) != 0 )
    {
        mod->last_write_time = data.ftLastWriteTime;
        return 1;
    }
    return 0;
}

void
reload_module( Module* mod )
{
    printf( "Reloading %s...\n", mod->path );

    // Get current state
    typedef void* ( *GetStateFunc )( void );
    GetStateFunc get_state = (GetStateFunc)GetProcAddress( mod->handle, "get_module_state" );
    if ( get_state )
    {
        mod->state = get_state();
    }

    // Unregister old types
    if ( mod->info && mod->info->unregister_types )
    {
        mod->info->unregister_types( &g_registry );
    }

    // Unload old DLL
    FreeLibrary( mod->handle );

    // Copy DLL to temp file (so we can rebuild while running)
    char temp_path[ 256 ];
    sprintf( temp_path, "%s.tmp", mod->path );
    CopyFile( mod->path, temp_path, FALSE );

    // Load new DLL
    mod->handle = LoadLibrary( temp_path );
    if ( mod->handle == NULL )
    {
        printf( "Module failed to load %s...\n", mod->path );
        return;
    }

    // Get module info
    typedef ModuleInfo* ( *GetInfoFunc )( void );
    GetInfoFunc get_info = (GetInfoFunc)GetProcAddress( mod->handle, "get_module_info" );
    if ( get_info )
    {
        mod->info = get_info();

        // Register new types
        if ( mod->info->register_types )
        {
            mod->info->register_types( &g_registry );
        }

        // Restore state
        if ( mod->info->hot_reload_fixup )
        {
            mod->info->hot_reload_fixup( &g_registry, mod->state );
        }
    }
}

// ============================================================================
#endif