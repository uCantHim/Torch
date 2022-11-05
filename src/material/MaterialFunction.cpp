#include "trc/material/MaterialFunction.h"



namespace trc
{

MaterialFunction::MaterialFunction(Signature sig)
    :
    signature(std::move(sig))
{
}

auto MaterialFunction::getSignature() const -> const Signature&
{
    return signature;
}

} // namespace trc
