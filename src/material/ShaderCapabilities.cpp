#include "trc/material/ShaderCapabilities.h"

#include <stdexcept>



auto trc::to_string(Capability capability) -> std::string
{
    switch (capability)
    {
    case Capability::eVertexNormal:   return "VertexNormal";
    case Capability::eVertexPosition: return "VertexPosition";
    case Capability::eVertexUV:       return "VertexUV";
    case Capability::eTime:           return "Time";
    case Capability::eTimeDelta:      return "TimeDelta";
    case Capability::eTextureSample:  return "TextureSample";

    case Capability::eNumCapabilities:
        return "NUM_CAPABILITIES=" + std::to_string((uint32_t)capability) + ")";
    }

    throw std::logic_error("[In to_string(Capability)]: Unknown shader capability.");
}
