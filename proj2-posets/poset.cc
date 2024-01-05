#include <iostream>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <cassert>
#include <algorithm>
#include <memory>
#include "poset.h"

namespace {
#ifndef NDEBUG
    const bool DEBUG = true;
#else
    const bool DEBUG = false;
#endif
    // ----- Type aliases ----- //

    using poset_id_t        = unsigned long;
    using string_id_t       = size_t;
    using string_ref_cnt_t  = size_t;
    using relation_t        = int_fast8_t;
    using edge_t            = std::pair<string_id_t, relation_t>;
    using edge_collection_t = std::unordered_map<string_id_t, relation_t>;
    // if poset[u][v] == ELEMENT_GREATER then u > v and poset[v][u] == ELEMENT_SMALLER. See add_edge.
    using poset_t           = std::unordered_map<string_id_t, edge_collection_t>;
    using posets_t          = std::unordered_map<poset_id_t, poset_t>;
    // Using 'string' instead of 'char*' to have the appropriate hashing function.
    using string_to_id_t    = std::unordered_map<std::string, string_id_t>;
    using string_ref_t      = std::unordered_map<string_id_t, string_ref_cnt_t>;

    const int_fast8_t ELEMENT_GREATER = 1;
    const int_fast8_t ELEMENT_LESS = 2;

    poset_id_t last_added_poset_id = -1;
    string_id_t last_added_string_id = 0;

    // Solving the static initialization order fiasco.
    posets_t& active_posets() {
        static std::unique_ptr<posets_t> result = std::make_unique<posets_t>();
        return *result;
    }

    string_to_id_t& string_to_id() {
        static std::unique_ptr<string_to_id_t> result = std::make_unique<string_to_id_t>();
        return *result;
    }

    string_ref_t& string_references() {
        static std::unique_ptr<string_ref_t> result = std::make_unique<string_ref_t>();
        return *result;
    }

    // ----- Printing functions ----- //

    void print_debug_message() {
        std::cerr << std::endl;
    }

    template<typename T, typename... Ts>
    void print_debug_message(const T& first, const Ts... other) {
        std::cerr << first;
        print_debug_message(other...);
    }

    void print_does_not_exists(poset_id_t id, const std::string& f_name) {
        print_debug_message(f_name, ": poset ", id, " does not exist");
    }

    inline std::string char_pointer_to_string(char const* str) {
        return str == NULL ? "NULL" : "\"" + std::string(str) + "\"";
    }

    // ----- String manipulation ----- //

    inline bool is_string_mapped(char const* str) {
        return string_to_id().count(str) > 0;
    }

    string_id_t add_string(char const* str) {
        last_added_string_id++;
        string_to_id().insert({str, last_added_string_id});
        string_references().insert({last_added_string_id, 0});
        return last_added_string_id;
    }

    string_id_t get_string_id(char const* str, bool insert_if_absent = false) {
        if (insert_if_absent && !is_string_mapped(str)) {
            return add_string(str);
        }

        if constexpr (DEBUG) {
            if (!insert_if_absent && !is_string_mapped(str)) {
                assert(false);
            }
        }

        return string_to_id().at(str);
    }

    inline bool is_string_in_poset(poset_t& poset, char const* value) {
        return poset.count(get_string_id(value)) > 0;
    }

    inline void add_string_reference(string_id_t sid) {
        string_references().at(sid)++;
    }

    // If str is null, the string and its reference counter won't be auto-removed.
    void remove_string_reference(string_id_t sid, char const* str) {
        string_ref_cnt_t& ref = string_references().at(sid);

        if constexpr (DEBUG) {
            if (ref <= 0) {
                assert(false);
            }
        }

        ref--;

        if (str != NULL && ref == 0) {
            string_references().erase(sid);
            string_to_id().erase(str);
        }
    }

    inline string_ref_cnt_t active_references(string_id_t sid) {
        return string_references().at(sid);
    }

    void cleanup_empty_references() {
        for (auto it = string_to_id().begin(), last = string_to_id().end(); it != last;) {
            if (active_references(it->second) == 0) {
                string_references().erase(it->second);
                it = string_to_id().erase(it);
            } else {
                it++;
            }
        }
    }

    // Ignoring transitivity, checks if value1 < value2.
    inline bool directly_greater_than(poset_t const& poset, string_id_t value1, string_id_t value2) {
        return poset.count(value2) > 0 && poset.at(value2).count(value1) > 0
               && poset.at(value2).at(value1) == ELEMENT_GREATER;
    }

    // ----- Poset manipulation ----- //

    inline bool poset_exists(poset_id_t id) {
        return active_posets().count(id) > 0;
    }

    inline poset_id_t add_poset() {
        last_added_poset_id++;
        active_posets().insert({last_added_poset_id, poset_t()});
        return last_added_poset_id;
    }

    inline poset_t& get_poset(poset_id_t pid) {
        return active_posets().at(pid);
    }

    void clear_poset(poset_id_t pid) {
        poset_t& poset = get_poset(pid);

        for (auto& [sid, edges] : poset) {
            remove_string_reference(sid, NULL);
        }

        cleanup_empty_references();
        poset.clear();
    }

    void remove_poset(poset_id_t pid) {
        clear_poset(pid);
        active_posets().erase(pid);
    }

    /*
    * If an element 'value' exists in the poset with the identifier 'id',
    * the result is true, otherwise, it's false.
    */
    bool is_element_in_poset(poset_id_t id, char const* value) {
        return is_string_mapped(value) && active_posets().at(id).count(get_string_id(value)) > 0;
    }

    void remove_edge(poset_t& poset, string_id_t u, string_id_t v) {
        if (u == v)
            return;
        poset[u].erase(v);
        poset[v].erase(u);
    }

    /*
     * Adds an edge between u and v indicating that u > v
     */
    void add_edge(poset_t& poset, string_id_t u, string_id_t v) {
        if (u == v)
            return;
        poset[u][v] = ELEMENT_GREATER;
        poset[v][u] = ELEMENT_LESS;
    }

    /*
    * Recursively checks if value1 < value2.
    */
    bool poset_test_recursive(const poset_t& poset, string_id_t value1, string_id_t value2) {
        bool result = false;

        if (!poset.at(value2).empty()) {
            if (directly_greater_than(poset, value1, value2)) {
                result = true;
            }
            else { // Checking if any element from the set is in a relation with 'value1'.
                for (const auto& set_element : poset.at(value2)) {
                    if (!result && set_element.second == ELEMENT_GREATER) {
                        result = poset_test_recursive(poset, value1, set_element.first);
                    }
                }
            }
        }

        return result;
    }

    /*
    * Works the same way as 'poset_test' (checks if value1 <= value2),
    * but doesn't print messages and doesn't check input data for validity.
    * The 'poset_test' function may print messages,
    * which is not needed when used as a helper function.
.
    */
    bool does_relation_exist(poset_id_t id, char const* value1, char const* value2) {
        string_id_t sid1 = get_string_id(value1), sid2 = get_string_id(value2);
        if (sid1 == sid2) { // We assume that the element is in relation with itself.
            return true;
        }
        return poset_test_recursive(get_poset(id), sid1, sid2);
    }

    std::pair<std::vector<poset_id_t>, std::vector<poset_id_t>> extract_neighbours(poset_t const& poset,
                                                                                   string_id_t sid,
                                                                                   bool ignore_less = false) {

        std::vector<poset_id_t> less, greater;

        for (auto edge : poset.at(sid)) {
            if (edge.second == ELEMENT_LESS) {
                greater.push_back(edge.first);
            } else if (!ignore_less && edge.second == ELEMENT_GREATER) {
                less.push_back(edge.first);
            }
        }

        return {less, greater};
    }

    /*
     * Checks if the poset with the given id exists and
     * if value1 and value2 are both not NULL, and both belong to the given poset.
     */
    bool validate_arguments(poset_id_t id, char const* value1,
                            char const* value2, const std::string& function_name) {

        if constexpr (DEBUG) {
            print_debug_message(function_name, "(", std::to_string(id), ", ",
                                char_pointer_to_string(value1), ", ",
                                char_pointer_to_string(value2), ")");

            if (value1 == NULL) {
                print_debug_message(function_name, ": invalid value1 (NULL)");
            }
            if (value2 == NULL) {
                print_debug_message(function_name, ": invalid value2 (NULL)");
            }
        }

        bool does_poset_exist = poset_exists(id);
        if constexpr (DEBUG) {
            if (!does_poset_exist) {
                print_debug_message(function_name, ": poset ",
                                    std::to_string(id), " does not exist");
            }
        }

        if (value1 == NULL || value2 == NULL || !does_poset_exist) {
            return false;
        }

        if (!is_element_in_poset(id, value1)) {
            if constexpr (DEBUG) {
                print_debug_message(function_name, ": poset ", std::to_string(id),
                                    ", element \"", value1, "\" does not exist");
            }
            return false;
        }
        else if (!is_element_in_poset(id, value2)) {
            if constexpr (DEBUG) {
                print_debug_message(function_name, ": poset ", std::to_string(id),
                                    ", element \"", value2, "\" does not exist" );
            }
            return false;
        }

        return true;
    }

    void reachable_dfs(poset_t& poset, string_id_t v, std::unordered_set<string_id_t>& vis) {
        vis.insert(v);

        for (std::pair<const string_id_t, relation_t>& e : poset[v]) {
            if (e.second == ELEMENT_GREATER && vis.find(e.first) == vis.end()) {
                reachable_dfs(poset, e.first, vis);
            }
        }
    }

    inline std::unordered_set<string_id_t> reachable_from(poset_t& poset, poset_id_t source) {
        std::unordered_set<string_id_t> vis;
        reachable_dfs(poset, source, vis);
        return vis;
    }

    inline void disconnect(poset_t& poset, string_id_t sid) {
        for (edge_t neigh : poset[sid]) {
            poset[neigh.first].erase(sid);
        }
    }

    bool can_be_removed(poset_t& poset, string_id_t lower, string_id_t upper) {
        remove_edge(poset, upper, lower);
        std::unordered_set<string_id_t> vis = reachable_from(poset, upper);
        add_edge(poset, upper, lower);

        return vis.empty() || vis.count(lower) == 0;
    }

    namespace cxx {

        extern "C" poset_id_t poset_new() {
            poset_id_t pid = add_poset();

            if constexpr (DEBUG) {
                print_debug_message("poset_new()\nposet_new: poset ", pid, " created");
            }

            return pid;
        }

        extern "C" bool poset_insert(poset_id_t id, char const* value) {

            if constexpr (DEBUG) {
                if (value != NULL) {
                    print_debug_message("poset_insert(",
                                        std::to_string(id), ", \"", value, "\")");
                }
                else {
                    print_debug_message("poset_insert(", std::to_string(id), ", NULL)\n",
                                        "poset_insert: invalid value (NULL)");
                }
            }

            if (value == NULL) {
                return false;
            }

            if (poset_exists(id)) {
                string_id_t sid = get_string_id(value, true);
                poset_t& poset = get_poset(id);

                if (!is_string_in_poset(poset, value)) {
                    poset[sid] = edge_collection_t();
                    add_string_reference(sid);

                    if constexpr (DEBUG) {
                        print_debug_message("poset_insert: poset ", std::to_string(id),
                                            ", element \"", value, "\" inserted" );
                    }

                    return true;
                }
                else if constexpr (DEBUG) {
                    print_debug_message("poset_insert: poset ",  std::to_string(id),
                                        ", element \"", value, "\" already exists");
                }
            }
            else if constexpr (DEBUG) {
                print_does_not_exists(id, "poset_insert");
            }

            return false;
        }

        extern "C" bool poset_test(poset_id_t id, char const* value1, char const* value2) {

            if (!validate_arguments(id, value1, value2, "poset_test")) {
                return false;
            }

            string_id_t sid1 = get_string_id(value1);
            string_id_t sid2 = get_string_id(value2);

            // We assume that the element is in relation with itself.
            if (sid1 == sid2) {
                if constexpr (DEBUG) {
                    print_debug_message("poset_test: poset ", std::to_string(id),
                                        ", relation (\"", value1, "\", \"", value2, "\") exists");
                }
                return true;
            }

            bool result = poset_test_recursive(get_poset(id), sid1, sid2);

            if constexpr (DEBUG) {
                print_debug_message("poset_test: poset ", id, ", relation (\"", value1, "\", \"",
                                    value2, "\") ",  (result ? "exists" : "does not exist"));
            }

            return result;
        }

        extern "C" bool poset_add(poset_id_t id, char const* value1, char const* value2) {

            if (!validate_arguments(id, value1, value2, "poset_add")) {
                return false;
            }

            if (!does_relation_exist(id, value1, value2) &&
                    !does_relation_exist(id, value2, value1)) {

                add_edge(get_poset(id), get_string_id(value2), get_string_id(value1));

                if constexpr (DEBUG) {
                    print_debug_message("poset_add: poset ", std::to_string(id), ", relation (\"",
                                        value1, "\", \"", value2, "\") added");
                }

                return true;
            }
            else if constexpr (DEBUG) {
                print_debug_message("poset_add: poset ", std::to_string(id), ", relation (\"",
                                    value1, "\", \"", value2, "\") cannot be added");
            }

            return false;
        }

        extern "C" size_t poset_size(poset_id_t id) {
            if constexpr (DEBUG) {
                print_debug_message("poset_size(", std::to_string(id), ")");
            }

            size_t poset_size = 0;
            if (poset_exists(id)) {
                poset_size = get_poset(id).size();

                if constexpr (DEBUG) {
                    print_debug_message("poset_size: poset ", std::to_string(id), " contains ",
                                        std::to_string(poset_size), " element(s)");
                }
            }
            else if constexpr (DEBUG) {
                print_debug_message("poset_size: poset ", std::to_string(id), " does not exist");
            }

            return poset_size;
        }

        extern "C" void poset_delete(poset_id_t id) {
            if constexpr(DEBUG) {
                print_debug_message("poset_delete(", id, ")");
            }

            if (poset_exists(id)) {
                remove_poset(id);

                if constexpr(DEBUG) {
                    print_debug_message("poset_delete: poset ", id, " deleted");
                }
            }
            else if constexpr(DEBUG) {
                print_does_not_exists(id, "poset_delete");
            }
        }

        extern "C" void poset_clear(poset_id_t id) {
            if constexpr (DEBUG) {
                print_debug_message("poset_clear(", id, ")");
            }

            if (poset_exists(id)) {
                clear_poset(id);

                if constexpr (DEBUG) {
                    print_debug_message("poset_clear: poset ", id, " cleared");
                }
            }
            else if constexpr (DEBUG) {
                print_does_not_exists(id, "poset_clear");
            }
        }

        extern "C" bool poset_remove(poset_id_t id, char const* value) {
            static const std::string FUNCTION_NAME = "poset_remove";
            if constexpr (DEBUG) {
                print_debug_message(FUNCTION_NAME, "(", id,
                                    ", ", char_pointer_to_string(value), ")");
            }

            bool status = false;
            if (poset_exists(id) && value != NULL) {
                if (is_element_in_poset(id, value)) {
                    poset_t& poset = get_poset(id);
                    string_id_t sid = get_string_id(value);

                    disconnect(poset, sid);

                    auto [less, greater] = extract_neighbours(poset, sid);
                    for (string_id_t sid_greater : greater) {
                        std::unordered_set<string_id_t> reachable = reachable_from(poset, sid_greater);

                        for (string_id_t sid_less : less)
                            if (reachable.find(sid_less) == reachable.end())
                                add_edge(poset, sid_greater, sid_less);
                    }

                    if constexpr (DEBUG) {
                        print_debug_message(FUNCTION_NAME, ": poset ", id,
                                            ", element \"", value, "\" removed");
                    }
                    poset.erase(sid);
                    remove_string_reference(sid, value);

                    status = true;

                } else if constexpr (DEBUG) {
                    print_debug_message(FUNCTION_NAME, ": poset ", id,
                                        ", element \"", value, "\" does not exist");
                }
            }
            else if constexpr (DEBUG) {
                if (value == NULL)
                    print_debug_message(FUNCTION_NAME, ": invalid value (NULL)");
                if (!poset_exists(id))
                    print_does_not_exists(id, FUNCTION_NAME);
            }

            return status;
        }

        extern "C" bool poset_del(poset_id_t id, char const* value1, char const* value2) {
            bool status = false;
            bool are_arguments_valid = validate_arguments(id, value1, value2, "poset_del");

            if (are_arguments_valid && does_relation_exist(id, value1, value2)) {

                string_id_t sid1 = get_string_id(value1), sid2 = get_string_id(value2);
                poset_t& poset = get_poset(id);

                if (can_be_removed(poset, sid1, sid2)) {
                    remove_edge(poset, sid1, sid2);

                    auto [less, greater] = extract_neighbours(poset, sid1);
                    for (string_id_t l : less) {
                        add_edge(poset, sid2, l);
                    }

                    auto [less2, greater2] = extract_neighbours(poset, sid2);
                    for (string_id_t g2 : greater2) {
                        add_edge(poset, g2, sid1);
                    }

                    status = true;

                    if constexpr (DEBUG) {
                        print_debug_message("poset_del: poset ", id,", relation (\"",
                                            value1, "\", \"", value2, "\") deleted");
                    }
                } else if constexpr (DEBUG) {
                    print_debug_message("poset_del: poset ", id, ", relation (\"",
                                        value1, "\", \"", value2, "\") cannot be deleted");
                }
            }
            else if constexpr (DEBUG) {
                if (are_arguments_valid) {
                    print_debug_message("poset_del: poset ", id, ", relation (",
                                        char_pointer_to_string(value1), ", ",
                                        char_pointer_to_string(value2), ") cannot be deleted");
                }
            }

            return status;
        }
    }
}
