#include "compiler.h"
#include <assert.h>
#include "../helpers/vector.h"

struct scope *scope_alloc()
{
    struct scope *scope = calloc(sizeof(struct scope), 1);
    scope->entities = vector_create(sizeof(void *));
    vector_set_peek_pointer_end(scope->entities);
    vector_set_flag(scope->entities, VECTOR_FLAG_PEEK_DECREMENT);

    return scope;
}

struct scope *scope_create_root(struct compile_process *process)
{
    // Assert no root is currently set
    assert(!process->scope.root);
    assert(!process->scope.current);

    struct scope *root_scope = scope_alloc();
    process->scope.root = root_scope;
    process->scope.current = root_scope;
    return root_scope;
}

void scope_free_root(struct compile_process *process)
{
    process->scope.root = NULL;
    process->scope.current = NULL;
}

struct scope *scope_new(struct compile_process *process, int flags)
{
    assert(process->scope.root);
    assert(process->scope.current);

    struct scope *new_scope = scope_alloc();
    new_scope->flags = flags;
    new_scope->parent = process->scope.current;
    process->scope.current = new_scope;
}

void scope_iteration_start(struct scope *scope)
{
    vector_set_peek_pointer(scope->entities, 0);

    // Decrementing vectors our iteration should start at the end.
    if (scope->entities->flags & VECTOR_FLAG_PEEK_DECREMENT)
    {
        vector_set_peek_pointer_end(scope->entities);
    }
}

void *scope_iterate_back(struct scope *scope)
{
    if (vector_count(scope->entities) == 0)
        return NULL;

    return *(void **)(vector_peek(scope->entities));
}

void *scope_last_entity_at_scope(struct scope *scope)
{
    if (!vector_count(scope->entities))
    {
        return NULL;
    }

    return *(void **)(vector_back(scope->entities));
}

void *scope_last_entity_from_scope_stop_at(struct scope *scope, struct scope *stop_scope)
{
    if (scope == stop_scope)
    {
        // Nothing to be done
        return NULL;
    }

    void *last = scope_last_entity_at_scope(scope);
    if (last)
    {
        return last;
    }

    struct scope *parent = scope->parent;
    if (!last && parent)
    {
        return scope_last_entity_from_scope_stop_at(parent, stop_scope);
    }

    return NULL;
}

void *scope_last_entity_stop_at(struct compile_process *process, struct scope *stop_scope)
{
    return scope_last_entity_from_scope_stop_at(process->scope.current, stop_scope);
}

void *scope_last_entity(struct compile_process *process)
{
    return scope_last_entity_stop_at(process, NULL);
}

void scope_push(struct compile_process *process, void *ptr, size_t elem_size)
{
    vector_push(process->scope.current->entities, &ptr);

    process->scope.current->size += elem_size;
}

void scope_finish(struct compile_process *process)
{
    struct scope *new_current_scope = process->scope.current->parent;
    process->scope.current = new_current_scope;

    if (process->scope.root && !process->scope.current)
    {
        process->scope.root = NULL;
    }
}

struct scope *scope_current(struct compile_process *process)
{
    return process->scope.current;
}