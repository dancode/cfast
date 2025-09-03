// ============================================================================
// editor.c - Editor that uses reflection
// ============================================================================


#include "reflection_core.h"    // reflection data Type defintion
#include <stdio.h>

// ============================================================================
// Generic property editor using reflection
// ============================================================================

void
draw_property_editor( void* obj, Type* type )
{
    printf( "=== %s Editor ===\n", type->name );

    for ( uint8_t i = 0; i < type->field_count; i++ )
    {
        Field* field     = &type->fields[ i ];
        void*  field_ptr = field_get_ptr( obj, type, i );

        // Check if field is editable
        if ( field->flags & 1 )
        {    // Editable flag
            printf( "  %s: ", field->name );

            if ( field->type_id == 0 && field->size == sizeof( float ) )
            {
                // Float field
                float* value = (float*)field_ptr;
                printf( "%.2f [editable]\n", *value );
                // In real editor: ImGui::DragFloat(field->name, value);
            }
            else if ( field->type_id != 0 )
            {
                // Nested type - recurse
                Type* field_type = type_get( field->type_id );
                if ( field_type )
                {
                    printf( "\n" );
                    draw_property_editor( field_ptr, field_type );
                }
            }
        }
        else
        {
            printf( "  %s: [read-only]\n", field->name );
        }
    }
}

// ============================================================================
// Serialize any object to JSON using reflection
// ============================================================================

void
serialize_to_json( void* obj, Type* type, FILE* file )
{
    fprintf( file, "{\n" );
    fprintf( file, "  \"_type\": \"%s\",\n", type->name );

    for ( uint8_t i = 0; i < type->field_count; i++ )
    {
        Field* field     = &type->fields[ i ];
        void*  field_ptr = field_get_ptr( obj, type, i );

        fprintf( file, "  \"%s\": ", field->name );

        if ( field->type_id == 0 )
        {
            // Primitive type
            if ( field->size == sizeof( float ) )
            {
                fprintf( file, "%.3f", *(float*)field_ptr );
            }
            else if ( field->size == sizeof( uint32_t ) )
            {
                fprintf( file, "%u", *(uint32_t*)field_ptr );
            }
            else
            {
                fprintf( file, "\"<binary>\"" );
            }
        }
        else
        {
            // Nested object
            Type* field_type = type_get( field->type_id );
            serialize_to_json( field_ptr, field_type, file );
        }

        if ( i < type->field_count - 1 )
            fprintf( file, "," );
        fprintf( file, "\n" );
    }
    fprintf( file, "}" );
}

// ============================================================================