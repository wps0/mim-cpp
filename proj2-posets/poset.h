#ifndef POSET_H
#define POSET_H

#include <stddef.h>

#ifdef __cplusplus
#include <iostream> // Solving the static initialization order fiasco.
namespace cxx {
    extern "C" {
#else
#include <stdbool.h>
#endif

    /*
     * Creates a new poset and returns its id.
     */
    unsigned long poset_new(void);

    /*
     * If a poset with the identifier id exists, and the element value doesn't belong to
     * this set, it adds the element to the set, otherwise, it does nothing. New element
     * is not related to any other element. The result is true when the element is added, and
     * false otherwise.
     */
    bool poset_insert(unsigned long id, char const *value);

    /*
     * If a poset with the given id exists, and elements value1 and value2 belong
     * to this set, and element value1 precedes element value2, the result is true, otherwise, it's false.
     */
    bool poset_test(unsigned long id, char const *value1, char const *value2);

    /*
     * If a poset with the given id exists, and elements 'value1' and 'value2' belong to this set
     * and are not in relation, it adds the relation in a way that element 'value1' now precedes element 'value2'
     * (transitive relation), otherwise, it does nothing. The result is true when the relation is added,
     * and false otherwise.
     */
    bool poset_add(unsigned long id, char const *value1, char const *value2);

    /*
     * If a poset with the 'id' exists, the result is the number of its elements, otherwise, it's 0.
     */
    size_t poset_size(unsigned long id);

    /*
     * If a poset with the 'id' exists, it deletes it, otherwise, it does nothing.
     */
    void poset_delete(unsigned long id);

    /*
     * If a poset with the 'id' exists, it removes all of its elements and relations between them, otherwise, it does nothing.
     */
    void poset_clear(unsigned long id);

    /*
     * If there exists a poset with the 'id' and the element 'value' belongs to this set,
     * it removes the element from the set along with all elements it was in relation with, otherwise, it does nothing.
     * The result is true if the element has been removed, and false otherwise.
     */
    bool poset_remove(unsigned long id, char const* value);

    /*
     * If there exists a poset with the 'id', and elements 'value1' and 'value2' belong to this poset,
     * with 'value1' being in relation with 'value2', and removing the relationship between 'value1' and 'value2'
     * will not violate the conditions of a partial order,
     * it removes the relationship between these elements, otherwise, it does nothing.
     * The result is true if the relationship has been changed, and false otherwise.
     */
    bool poset_del(unsigned long id, char const* value1, char const* value2);

#ifdef __cplusplus
    }
}
#endif // __cplusplus
#endif // POSET_H
