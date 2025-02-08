#pragma once

#include <cassert>
#include <utility>

#include <componentlib/Table.h>
#include <trc_util/algorithm/IteratorRange.h>
#include <trc_util/data/TypeMap.h>

#include "trc/Types.h"
#include "trc/core/SceneModule.h"

namespace trc
{
    class SceneBase
    {
    public:
        using ModuleIterator = componentlib::Table<u_ptr<SceneModule>>::ValueIterator;
        using ConstModuleIterator = componentlib::Table<u_ptr<SceneModule>>::ConstValueIterator;

        /**
         * @throw std::out_of_range
         */
        template<SceneModuleT Module>
        void registerModule(u_ptr<Module> newModule);

        /**
         * @throw std::out_of_range
         */
        template<SceneModuleT Module>
        void registerModule(s_ptr<Module> newModule);

        /**
         * @throw std::out_of_range
         */
        template<SceneModuleT Module>
        auto getModule() -> Module&;

        /**
         * @throw std::out_of_range
         */
        template<SceneModuleT Module>
        auto getModule() const -> const Module&;

        template<SceneModuleT Module>
        auto tryGetModule() -> Module*;

        template<SceneModuleT Module>
        auto tryGetModule() const -> const Module*;

        auto iterModules()       -> algorithm::IteratorRange<ModuleIterator>;
        auto iterModules() const -> algorithm::IteratorRange<ConstModuleIterator>;

    private:
        using TypeIndex = data::TypeIndexAllocator<SceneBase>;

        componentlib::Table<s_ptr<SceneModule>> modules;
    };



    template<SceneModuleT Module>
    inline void SceneBase::registerModule(u_ptr<Module> newModule)
    {
        modules.emplace(TypeIndex::get<Module>(), std::move(newModule));
    }

    template<SceneModuleT Module>
    inline void SceneBase::registerModule(s_ptr<Module> newModule)
    {
        modules.emplace(TypeIndex::get<Module>(), std::move(newModule));
    }

    template<SceneModuleT Module>
    inline auto SceneBase::getModule() -> Module&
    {
        assert(nullptr != dynamic_cast<Module*>(modules.get(TypeIndex::get<Module>()).get()));
        return static_cast<Module&>(*modules.get(TypeIndex::get<Module>()));
    }

    template<SceneModuleT Module>
    inline auto SceneBase::getModule() const -> const Module&
    {
        assert(nullptr != dynamic_cast<const Module*>(modules.get(TypeIndex::get<Module>()).get()));
        return static_cast<const Module&>(*modules.get(TypeIndex::get<Module>()));
    }

    template<SceneModuleT Module>
    inline auto SceneBase::tryGetModule() -> Module*
    {
        auto ptr = modules.try_get(TypeIndex::get<Module>());
        if (ptr != nullptr)
        {
            assert(ptr->get() != nullptr);
            assert(dynamic_cast<Module*>(ptr->get()) != nullptr);
            return static_cast<Module*>(ptr->get());
        }
        return nullptr;
    }

    template<SceneModuleT Module>
    inline auto SceneBase::tryGetModule() const -> const Module*
    {
        auto ptr = modules.try_get(TypeIndex::get<Module>());
        if (ptr != nullptr)
        {
            assert(ptr->get() != nullptr);
            assert(dynamic_cast<const Module*>(ptr->get()) != nullptr);
            return static_cast<const Module*>(ptr->get());
        }
        return nullptr;
    }
} // namespace trc
