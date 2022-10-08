#include "trc/core/TypeErasedStructureChain.h"



trc::TypeErasedStructureChain::TypeErasedStructureChain()
    :
    data(nullptr),
    destroy([](void*){}),
    getFirstStructure([](void*) -> void* { return nullptr; })
{
}

trc::TypeErasedStructureChain::~TypeErasedStructureChain()
{
    destroy(data);
}

auto trc::TypeErasedStructureChain::getPNext() const -> void*
{
    return getFirstStructure(data);
}
