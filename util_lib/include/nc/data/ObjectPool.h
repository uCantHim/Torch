#pragma once

#include <memory>
#include <vector>
#include <string>
#include <type_traits>

#include "../Exception.h"

namespace nc::data
{
    class ObjectPoolOverflowError : public Exception {};

    template<class>
    class PooledObject;

    /**
     * @brief Pool that manages the memory of a fixed number of objects
     *
     * One may optionally implement T::poolInit(...) and/or T::poolDestroy().
     * This class uses SFINAE to select appropriate implementations based
     * on presence/absence of these methods.
     *
     * @tparam class  T    The contained type
     * @tparam size_t Size Capacity of the pool
     */
    template<class T>
    class ObjectPool
    {
    public:
        // Can't use concepts instead because PooledObject is incomplete
        static_assert(std::is_base_of_v<PooledObject<T>, T>,
                      "Type in object pool must be derived from an instance of PooledObject<>.");

        /**
         * @param size_t poolSize Maximum number of objects T in the pool.
         *                        The pool can not be resized.
         */
        explicit inline ObjectPool(size_t poolSize);

        /**
         * @brief Create an object from the pool
         *
         * Calls T::poolInit() with the passed arguments on the created object.
         *
         * @param Args... args Arguments to T::poolInit()
         *
         * @return T&          The created object
         */
        template<typename ...Args>
        inline auto createObject(Args&&... args) -> T&
            requires requires (T a, Args&&... args) { a.poolInit(std::forward<Args>(args)...); };

        /**
         * @brief Create an object from the pool
         *
         * This overload is called if T::poolInit() does not exist. Define a
         * method T::poolInit() to be called when the object is initialized.
         *
         * @return T& The created object
         */
        inline auto createObject() -> T&;

        /**
         * @brief Release the object, give memory back to the pool
         *
         * Calls T::poolDestroy() on the released object.
         *
         * @param T& object The released object
         */
        inline void releaseObject(T& object)
            requires requires (T a) { a.poolDestroy(); };

        /**
         * @brief Release the object, give memory back to the pool
         *
         * This overload is compiled if the derived type does not have a
         * method T::poolDestroy().
         *
         * @param T& object The released object
         */
        inline void releaseObject(T& object);

        template<std::invocable<T&> F>
        inline void foreachActive(F func);

    private:
        void doReleaseObject(T& object);

        std::vector<T> objects;
        T* firstFreeObject;
    };


    /**
     * @brief A class managed by an object pool
     *
     * @tparam class  Derived  The derived class, CRTP style
     * @tparam size_t PoolSize Capacity of the pool
     *
     * The pool is a static member of every class template instantiation. It is
     * possible to create more pools of any instantiation's type. The methods
     * PooledObject<...>::create() and PooledObject<...>::release() must not be
     * called on objects allocated from other pools because they target the
     * static pool.
     *
     * Derived classes **can** define following methods:
     *
     *  - poolInit(...): Called when an object is created. May take an
     *                   arbitrary number of arguments.
     *
     *  - poolDestroy(): Called when the object is destroyed. Takes no
     *                  arguments.
     *
     * Both methods must be accessible by the pool. You may want to hide the
     * methods and declare the pool as a friend.
     *
     * No other methods need to be defined. Derived classes must be default
     * constructible though. The best approach is to just not define any custom
     * constructors. Destructors are pretty useless since the allocated objects
     * will only be destroyed at program termination.
     */
    template<class Derived>
    class PooledObject
    {
    private:
        using Pool = ObjectPool<Derived>;
        friend Pool;

        bool alive{ false };
        union
        {
            // Used when alive
            Pool* owningPool;
            // Used when dead
            Derived* next;
        } poolState;

        inline bool isAlive();

    protected: // Required by vector
        PooledObject() = default;
        PooledObject(PooledObject&&) = default;

    public:
        PooledObject(const PooledObject&) = delete;
        PooledObject& operator=(const PooledObject&) = delete;
        PooledObject& operator=(PooledObject&&) = delete;

        /**
         * @brief Releases the object and gives the memory back to the pool
         *
         * Calls ObjectPool::releaseObject() on the static pool.
         *
         * The object shall not be used after a call to this method. Calls
         * Derived::poolDestroy().
         */
        inline void release();
    };


    // Do not use, contents are under construction
    namespace under_construction
    {
        template<class Sub>
        class ManagedPoolObjectHandle
        {
        public:
            explicit ManagedPoolObjectHandle(Sub& object)
                :
                object(&object),
                refCount(std::shared_ptr<void>(
                    nullptr,
                    [&](auto _) {
                        this->object->release();
                    }))
            {}
            ~ManagedPoolObjectHandle() = default;

            ManagedPoolObjectHandle(const ManagedPoolObjectHandle& other) = default;
            ManagedPoolObjectHandle(ManagedPoolObjectHandle&& other) noexcept = default;
            ManagedPoolObjectHandle& operator=(const ManagedPoolObjectHandle& rhs) = default;
            ManagedPoolObjectHandle& operator=(ManagedPoolObjectHandle&& rhs) noexcept = default;

            inline constexpr auto operator*() const noexcept -> Sub& {
                return *object;
            }
            inline constexpr auto operator->() const noexcept -> Sub* {
                return object;
            }

        private:
            Sub* object;
            std::shared_ptr<void> refCount;
        };


        template<class T>
        class ManagedObjectPool : private ObjectPool<T>
        {
        public:

        private:
            ObjectPool<T> pool;
        };
    } // namespace under_construction


#include "ObjectPool.inl"

} // namespace ic::util
