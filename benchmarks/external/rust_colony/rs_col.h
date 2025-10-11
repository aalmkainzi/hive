#ifndef COLONY_BINDINGS_H
#define COLONY_BINDINGS_H

#include <stddef.h>
#include <stdbool.h>
#include <stdalign.h>

#ifdef __cplusplus
extern "C" {
#endif

// Opaque types that match Rust Colony types
// You need to define COLONY_SIZE, COLONY_ALIGNMENT, HANDLE_SIZE, 
// HANDLE_ALIGNMENT, ITERATOR_SIZE, and ITERATOR_ALIGNMENT based on
// the values returned by the colony_get_*_size/alignment functions

typedef struct ExposedColony {
    alignas(COLONY_ALIGNMENT) char _data[COLONY_SIZE];
} ExposedColony;

typedef struct ExposedHandle {
    alignas(HANDLE_ALIGNMENT) char _data[HANDLE_SIZE];
} ExposedHandle;

typedef struct ExposedIterator {
    alignas(ITERATOR_ALIGNMENT) char _data[ITERATOR_SIZE];
} ExposedIterator;

// Colony management functions
void colony_create(ExposedColony* colony);
void colony_clone(const ExposedColony* src, ExposedColony* dst);
void colony_destroy(ExposedColony* colony);

// Element operations
void colony_insert(ExposedColony* colony, Big value, ExposedHandle* out_handle);
bool colony_erase(ExposedColony* colony, const ExposedHandle* handle);
const Big* colony_get(const ExposedColony* colony, const ExposedHandle* handle);
Big* colony_get_mut(ExposedColony* colony, const ExposedHandle* handle);

// Container info
size_t colony_size(const ExposedColony* colony);
bool colony_is_empty(const ExposedColony* colony);
void colony_clear(ExposedColony* colony);

// Iterator functions
void colony_iter_begin(const ExposedColony* colony, ExposedIterator* out_iter);
bool colony_iter_next(ExposedIterator* iter, const Big** out_value, ExposedHandle* out_handle);

// Size and alignment query functions
// Call these at runtime or compile time to get the correct values
size_t colony_get_colony_size(void);
size_t colony_get_colony_alignment(void);
size_t colony_get_handle_size(void);
size_t colony_get_handle_alignment(void);
size_t colony_get_iterator_size(void);
size_t colony_get_iterator_alignment(void);

#ifdef __cplusplus
}
#endif

#endif // COLONY_BINDINGS_H