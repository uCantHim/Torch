#pragma once

#include <memory>
#include <optional>
#include <vector>

#include <trc_util/data/IdPool.h>

#include "ComponentBase.h"
#include "Table.h"

namespace componentlib
{

/**
 * @brief Declares additional settings for a component type
 *
 * @tparam T The type of component for which the specialization is declared.
 *
 * Specializations may define a `TableImpl<KeyT>` template. This template
 * defines the `Table<>` implementation to use for the specialization's
 * component type. As hinted above, this template has one type parameter, which
 * is the key type for the table. If no such template is defined,
 * `DefaultComponentTableImpl<T, KeyT>` is used instead.
 *
 * Define specializations of this template to define callbacks for creation or
 * destruction of components. Possible callbacks are `onCreate` and `onDelete`.
 * Both of them take the owning storage, the object ID, and the created/deleted
 * object.
 *
 * Note that `onDelete`, if it is defined, takes the *already deleted*
 * component as an *l-value*. At the time `onDelete` is called, the
 * component does not exist in the table anymore.
 *
 * Example:
 *
 *     struct MyComponent {};
 *
 *     template<> struct ComponentTraits<MyComponent>
 *     {
 *         template<typename KeyT>
 *         using TableImpl = StableTableImpl<MyComponent, KeyT>;
 *
 *         void onCreate(MyStorage& storage, Key obj, MyComponent& comp)
 *         {
 *             // ...
 *         }
 *
 *         void onDelete(MyStorage& storage, Key obj, MyComponent comp)
 *         {
 *             MyComponent& c = storage.get<MyComponent>(obj);
 *             // ...
 *         }
 *     };
 */
template<ComponentType T>
struct ComponentTraits;

template<ComponentType T, typename KeyT>
using DefaultComponentTableImpl = StableTableImpl<T, KeyT>;

/**
 * This structure only exists to make the use of the `ComponentStorage::createObject`
 * function more convenient. If specialized, it must define a type
 * `ConstructedComponentType`, which is the component that can be constructed
 * from T. Example:
 *
 *    struct TestCreateInfo {};
 *
 *    struct Test {
 *        Test(TestCreateInfo info) { ... }
 *    };
 *
 *    template<> struct ComponentCreateInfoTraits<TestCreateInfo> {
 *        using ConstructedComponentType = Test;
 *    };
 *
 * It is then possible to call `createObject` to create an object with multiple
 * components at once, like this:
 *
 *    storage.createObject(TestCreateInfo{}, ...);
 */
template<typename T>
struct ComponentCreateInfoTraits;

/**
 * Ensures that a specialization of ComponentTraits<> exists for T.
 */
template<typename T>
concept HasValidComponentTraits = requires {
    typename ComponentTraits<T>;
};

/**
 * @brief Shortcut to access a create info's constructed type declaration.
 */
template<typename T>
using ConstructedType = typename ComponentCreateInfoTraits<T>::ConstructedComponentType;

/**
 * @brief Tests whether a type can be used as a create info
 */
template<typename T>
concept ComponentCreateInfo = requires {
    typename ConstructedType<T>;
    requires HasValidComponentTraits<ConstructedType<T>>;
    requires std::constructible_from<ConstructedType<T>, T>;
};

/**
 * @brief T is either a component or a create info for a component
 */
template<typename T>
concept ComponentOrCreateInfo = ComponentType<T> || ComponentCreateInfo<T>;

/**
 * @brief Central storage and management for components
 *
 * @tparam Derived I need to be able to pass the scene-like object which
 *         derives from ComponentStorage to the construction and destruction
 *         functions.
 *
 * @param Key The type that is used as an object ID.
 *
 * Added components are automatically destroyed when their owning
 * `Key` is.
 */
template<typename Derived, TableKey Key>
class ComponentStorage
{
private:
    using TableKeyType = Key;

    // Table implementation type for components that use the default definition
    template<ComponentType C>
    struct TableImpl {
        using Type = DefaultComponentTableImpl<C, TableKeyType>;
    };

    // Table implementation type for component traits that define a TableImpl
    // template.
    template<ComponentType C>
        requires requires { typename ComponentTraits<C>::template TableImpl<TableKeyType>; }
    struct TableImpl<C> {
        using Type = typename ComponentTraits<C>::template TableImpl<TableKeyType>;
    };

    // The type of table used for a component type
    template<ComponentType C>
    using TableType = Table<C, TableKeyType, typename TableImpl<C>::Type>;

    template<ComponentType C>
    static constexpr bool hasComponentConstructor =
        HasValidComponentTraits<C>
        && requires (Derived& d, Key obj, C& c) { ComponentTraits<C>{}.onCreate(d, obj, c); };

    template<ComponentType C>
    static constexpr bool hasComponentDestructor =
        HasValidComponentTraits<C>
        && requires (Derived& d, Key obj, C c) { ComponentTraits<C>{}.onDelete(d, obj, std::move(c)); };

public:
    ComponentStorage(const ComponentStorage&) = delete;
    ComponentStorage& operator=(const ComponentStorage&) = delete;

    ComponentStorage() = default;
    ComponentStorage(ComponentStorage&&) noexcept = default;
    ComponentStorage& operator=(ComponentStorage&&) noexcept = default;

    ~ComponentStorage();

    /**
     * @brief Create an object with no components attached to it
     */
    auto createObject() -> Key;

    /**
     * @brief Create a game object with arbitrary components attached to it
     *
     * @tparam ...Args Components
     */
    template<ComponentOrCreateInfo ...Args> inline
    auto createObject(Args&&... args) -> Key;

    /**
     * @brief Delete a game object and destroy all its components
     */
    void deleteObject(Key obj);

    /**
     * @brief Add a component to a game object
     *
     * If a component of type C already exists for the object, this does
     * nothing and returns the existing component.
     */
    template<ComponentType C, typename ...Args>
        requires std::constructible_from<C, Args...>
    inline auto add(Key key, Args&&... args) -> C&
    {
        return createComponent<C>(key, std::forward<Args>(args)...);
    }

    /**
     * @brief Test if a component exists for a key
     *
     * @return bool True if a component of type C exists at the key `key`.
     */
    template<ComponentType C>
    inline auto has(Key key) const -> bool
    {
        return getTable<C>().has(key);
    }

    /**
     * @brief Retrieve a single component
     *
     * @return C&
     */
    template<ComponentType C>
    inline auto get(Key key) -> C&
    {
        return getTable<C>().get(key);
    }

    /**
     * @brief Retrieve a single component
     *
     * @return C&
     */
    template<ComponentType C>
    inline auto get(Key key) const -> const C&
    {
        return getTable<C>().get(key);
    }

    /**
     * @brief Retrieve a single component
     *
     * @return trc::Maybe<C&>
     */
    template<ComponentType C>
    inline auto tryGet(Key key) -> std::optional<C*>
    {
        return getTable<C>().try_get(key);
    }

    /**
     * @brief Retrieve a single component
     *
     * @return trc::Maybe<C&>
     */
    template<ComponentType C>
    inline auto tryGet(Key key) const -> std::optional<const C*>
    {
        return getTable<C>().try_get(key);
    }

    /**
     * @brief Retrieve a single component
     *
     * @return trc::Maybe<C&>
     */
    template<ComponentType C>
    inline auto getM(Key key) -> trc::Maybe<C&>
    {
        return getTable<C>().get_m(key);
    }

    /**
     * @brief Retrieve a single component
     *
     * @return trc::Maybe<C&>
     */
    template<ComponentType C>
    inline auto getM(Key key) const -> trc::Maybe<const C&>
    {
        return getTable<C>().get_m(key);
    }

    /**
     * @brief Get the underlying table for a component
     *
     * Useful for iterating over all components of a type
     */
    template<ComponentType C>
    inline auto get() -> TableType<C>&
    {
        return getTable<C>();
    }

    /**
     * @brief Get the underlying table for a component
     *
     * Useful for iterating over all components of a type
     */
    template<ComponentType C>
    inline auto get() const -> const TableType<C>&
    {
        return getTable<C>();
    }

    /**
     * @brief Get the underlying table for a component
     *
     * Useful for iterating over all components of a type
     */
    template<ComponentType C>
    inline auto getTable() -> TableType<C>&;

    /**
     * @brief Get the underlying table for a component
     *
     * Useful for iterating over all components of a type
     */
    template<ComponentType C>
    inline auto getTable() const -> const TableType<C>&;

    /**
     * @brief Remove and destroy a component
     */
    template<ComponentType C>
    void remove(Key key)
    {
        auto comp = getTable<C>().erase(key);
        if constexpr (hasComponentDestructor<C>) {
            ComponentTraits<C>{}.onDelete(asDerived(), key, std::move(comp));
        }
    }

    /**
     * @brief Remove and destroy a component
     */
    template<ComponentType C>
    bool tryRemove(Key key)
    {
        if (auto comp = getTable<C>().try_erase(key))
        {
            if constexpr (hasComponentDestructor<C>) {
                ComponentTraits<C>{}.onDelete(asDerived(), key, std::move(*comp));
            }
            return true;
        }
        return false;
    }

    /**
     * @brief Destroy all object and all components
     */
    void clear();

private:
    ////////////////////////////////////////
    //  Basic table-handling and storage  //
    ////////////////////////////////////////

    trc::data::IdPool<size_t> objectIdPool;

    using Del = std::function<void(void*)>;
    mutable std::vector<std::unique_ptr<void, Del>> tables;

    /**
     * @brief Overload to create a component from a create info struct
     */
    template<ComponentCreateInfo T>
    inline auto createComponent(Key obj, T&& createInfo) -> ConstructedType<T>;

    /**
     * @brief Logic that creates a component
     */
    template<ComponentType C, typename ...Args>
        requires std::constructible_from<C, Args...>
    inline auto createComponent(Key obj, Args&&... args) -> C&;

    template<ComponentType C>
    inline void createDestructor(Key obj);

private:
    ////////////////////////////////
    //  Advanced object creation  //
    ////////////////////////////////

    inline auto asDerived() -> Derived& {
        return *static_cast<Derived*>(this);
    }

    /**
     * @brief Recursion terminator
     */
    template<typename T>
    inline void addGenericComponent(Key obj, T&& component);

    /**
     * @brief Compile-time recursive function
     */
    template<typename T, typename ...Ts>
    inline void addGenericComponent(Key obj, T&& component, Ts&&... comps);

    using ComponentDestructor = void(*)(Derived&, Key);
    Table<std::vector<ComponentDestructor>, TableKeyType> componentDestructors;
};



template<typename Derived, TableKey Key>
ComponentStorage<Derived, Key>::~ComponentStorage()
{
    clear();
}

template<typename Derived, TableKey Key>
auto ComponentStorage<Derived, Key>::createObject() -> Key
{
    return Key(objectIdPool.generate());
}

template<typename Derived, TableKey Key>
template<ComponentOrCreateInfo ...Args>
inline auto ComponentStorage<Derived, Key>::createObject(Args&&... args) -> Key
{
    Key newObj{ createObject() };

    // Add custom (optional) components
    addGenericComponent(newObj, std::forward<Args>(args)...);

    return newObj;
}

template<typename Derived, TableKey Key>
inline void ComponentStorage<Derived, Key>::deleteObject(Key obj)
{
    if (auto destructors = componentDestructors.try_erase(obj))
    {
        for (auto func : *destructors)
        {
            func(asDerived(), obj);
        }
    }

    objectIdPool.free(static_cast<uint32_t>(obj));
}

template<typename Derived, TableKey Key>
template<ComponentType C>
inline auto ComponentStorage<Derived, Key>::getTable() -> TableType<C>&
{
    // This is, together with the const-variant of this function, the only
    // point of instantiation for the template
    using Base = ComponentBase<C>;

    if (tables.size() <= Base::getComponentId()) {
        tables.resize(Base::getComponentId() + 1);
    }

    auto& el = tables.at(Base::getComponentId());
    if (el == nullptr)
    {
        el = std::unique_ptr<TableType<C>, Del>(
            new TableType<C>(),
            [](void* ptr) { delete static_cast<TableType<C>*>(ptr); }
        );
    }

    return *static_cast<TableType<C>*>(el.get());
}

template<typename Derived, TableKey Key>
template<ComponentType C>
inline auto ComponentStorage<Derived, Key>::getTable() const -> const TableType<C>&
{
    // This is probably the first and only point of instantiation for the template
    using Base = ComponentBase<C>;

    if (tables.size() <= Base::getComponentId()) {
        tables.resize(Base::getComponentId() + 1);
    }

    auto& el = tables.at(Base::getComponentId());
    if (el == nullptr)
    {
        el = std::unique_ptr<TableType<C>, Del>(
            new TableType<C>(),
            [](void* ptr) { delete static_cast<TableType<C>*>(ptr); }
        );
    }

    return *static_cast<const TableType<C>*>(el.get());
}

template<typename Derived, TableKey Key>
inline void ComponentStorage<Derived, Key>::clear()
{
    for (Key key : componentDestructors.keys())
    {
        for (ComponentDestructor& func : componentDestructors.get(key))
        {
            func(asDerived(), key);
        }
    }

    componentDestructors.clear();
    tables.clear();
    objectIdPool.reset();
}

template<typename Derived, TableKey Key>
template<typename T>
inline void ComponentStorage<Derived, Key>::addGenericComponent(Key obj, T&& createInfo)
{
    createComponent<T>(obj, std::forward<T>(createInfo));
}

template<typename Derived, TableKey Key>
template<typename T, typename ...Ts>
inline void ComponentStorage<Derived, Key>::addGenericComponent(
    Key obj,
    T&& createInfo,
    Ts&&... comps)
{
    createComponent<T>(obj, std::forward<T>(createInfo));
    addGenericComponent(obj, std::forward<Ts>(comps)...);
}

template<typename Derived, TableKey Key>
template<ComponentCreateInfo T>
inline auto ComponentStorage<Derived, Key>::createComponent(Key obj, T&& createInfo)
    -> ConstructedType<T>
{
    using C = ConstructedType<T>;

    return createComponent<C>(obj, std::forward<T>(createInfo));
}

template<typename Derived, TableKey Key>
template<ComponentType C, typename ...Args>
    requires std::constructible_from<C, Args...>
inline auto ComponentStorage<Derived, Key>::createComponent(Key obj, Args&&... args) -> C&
{
    // Construct the component
    auto [component, success] = getTable<C>().try_emplace(obj, std::forward<Args>(args)...);
    if (!success) {
        return component;
    }

    // Optionally call initializer functions
    if constexpr (hasComponentConstructor<C>)
    {
        ComponentTraits<C>{}.onCreate(asDerived(), obj, component);
    }

    createDestructor<C>(obj);

    return component;
}

template<typename Derived, TableKey Key>
template<ComponentType C>
inline void ComponentStorage<Derived, Key>::createDestructor(Key obj)
{
    auto& destructors = componentDestructors.try_emplace(obj).first;

    destructors.emplace_back([](Derived& storage, Key obj) {
        storage.template tryRemove<C>(obj);
    });
}

} // namespace componentlib
