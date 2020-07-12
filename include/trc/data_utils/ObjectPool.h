#pragma once

#include <memory>
#include <vector>
#include <string>
#include <type_traits>
#include <functional>

class Exception : public std::exception
{
public:
    Exception(std::string err = "") : errorMessage(std::move(err))
    {}

    auto what() const noexcept -> const char* override {
        return errorMessage.c_str();
    }

private:
    std::string errorMessage;
};


namespace trc::data
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
    private:
        template<typename _T>
        struct has_poolDestroy
        {
        private:
            using one = char;
            struct two { char _[2]; };

            template<typename C>
            static one test(decltype(&C::poolDestroy));
            template<typename C>
            static two test(...);

        public:
            static constexpr bool value = sizeof(test<_T>(0)) == sizeof(char);
        };

        template<typename _T>
        static constexpr bool has_poolDestroy_v = has_poolDestroy<_T>::value;

    public:
        static_assert(std::is_base_of_v<PooledObject<T>, T>,
                      "Type in object pool must be derived from an instance of PooledObject<>.");

        explicit inline ObjectPool(size_t poolSize = 100);

        /**
         * @brief Create an object from the pool
         *
         * Calls T::poolInit() with the passed arguments on the created object.
         *
         * @param Args... args Arguments to T::poolInit()
         *
         * @return T&          The created object
         */
        template<
            typename ...Args,
            class _T = T,                       // Locally-depentent redefinition of derived type
            typename std::enable_if_t<          // Enable if T::poolInit() exists
                std::is_member_function_pointer_v<decltype(&_T::poolInit)>,
                bool
            > = true
        >
        inline auto createObject(Args&&... args) -> T&
        {
            T& result = createObject();
            result.poolInit(std::forward<Args>(args)...);

            return result;
        }

        /**
         * @brief Create an object from the pool
         *
         * This overload is called if T::poolInit() does not exist. Define a
         * method T::poolInit() to be called when the object is initialized.
         *
         * @return T& The created object
         */
        inline auto createObject() -> T&
        {
            size_t index = nextSlot;
            if (!freeSlotsStack.empty())
            {
                index = freeSlotsStack.back();
                freeSlotsStack.pop_back();
            }
            else {
                nextSlot++;
            }

            if (index >= objects.size()) {
                throw ObjectPoolOverflowError();
            }

            auto& obj = objects[index];
            obj.alive = true;

            return obj;
        }

        /**
         * @brief Release the object, give memory back to the pool
         *
         * Calls T::poolDestroy() on the released object.
         *
         * @param T& object The released object
         */
        template<
            class _T = T,                       // Locally-depentent redefinition of derived type
            typename std::enable_if_t<          // Enable overload if T::poolDestroy() exists
                has_poolDestroy_v<_T>,
                bool
            > = true
        >
        inline void releaseObject(T& object)
        {
            object.alive = false;
            object.poolDestroy();
            freeSlotsStack.push_back(object.id);
        }

        /**
         * @brief Release the object, give memory back to the pool
         *
         * This overload is compiled if the derived type does not have a
         * method T::poolDestroy().
         *
         * @param T& object The released object
         */
        template<
            class _T = T,                       // Locally-depentent redefinition of derived type
            typename std::enable_if_t<
                !has_poolDestroy_v<_T>,
                bool
            > = true
        >
        inline void releaseObject(T& object)
        {
            object.alive = false;
            freeSlotsStack.push_back(object.id);
        }

        inline void foreachActive(std::function<void(T&)> f);

    private:
        std::vector<T> objects;
        size_t nextSlot{ 0u };
        std::vector<size_t> freeSlotsStack;
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

        size_t id;
        Pool* owningPool;
        bool alive{ false };

    protected: // Required by vector
        PooledObject() = default;
        PooledObject(PooledObject&&) = default;

    public:
        PooledObject(const PooledObject&) = delete;
        PooledObject& operator=(const PooledObject&) = delete;
        PooledObject& operator=(PooledObject&&) = delete;

        inline bool isAlive();

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
