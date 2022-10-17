#include "trc/material/MaterialFunction.h"



namespace trc
{

MaterialFunction::MaterialFunction(Signature sig, std::vector<Capability> requiredCapabilities)
    :
    signature(std::move(sig)),
    requiredCapabilities(std::move(requiredCapabilities))
{
}

auto MaterialFunction::getSignature() const -> const Signature&
{
    return signature;
}

auto MaterialFunction::getRequiredCapabilities() const -> const std::vector<Capability>&
{
    return requiredCapabilities;
}

} // namespace trc
