#pragma once

#include "Table.h"

namespace componentlib
{

/**
 * @brief Join two tables
 *
 * Join two tables on their keys. Call a callback for each matching pair
 * of entries. Passes the joined rows as well the their common key to the
 * callback.
 *
 * @param Table& t First table
 * @param Table& u Second table
 * @param F func   Callback
 */
template<
    typename T, typename U,          // Table object types
    typename TKey, typename UKey,    // Table key types
    typename TImpl, typename UImpl,
    std::invocable<TKey, T&, U&> F   // Callback type
>
inline void join(Table<T, TKey, TImpl>& t, Table<U, UKey, UImpl>& u, F&& func)
{
    for (auto [key, t_v, u_v] : t.join(u))
    {
        func(key, t_v, u_v);
    }
}

/**
 * @brief Join two tables
 *
 * Join two tables on their keys. Call a callback for each matching pair
 * of entries.
 *
 * @param Table& t First table
 * @param Table& u Second table
 * @param F func   Callback
 */
template<
    typename T, typename U,          // Table object types
    typename TKey, typename UKey,    // Table key types
    typename TImpl, typename UImpl,
    std::invocable<T&, U&> F         // Callback type
>
inline void join(Table<T, TKey, TImpl>& t, Table<U, UKey, UImpl>& u, F&& func)
{
    join(t, u, [&func](auto, T& t, U& u) { func(t, u); });
}

} // namespace componentlib
