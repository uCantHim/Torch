#include "trc/LightRegistry.h"



auto trc::LightRegistry::makeSunLight(vec3 color, vec3 direction, float ambientPercent)
    -> SunLight
{
    return makeLight<SunLightInterface>(
        sunLights,
        LightDeviceData{
            .color                = vec4(color, 1.0f),
            .position             = vec4(0.0f),
            .direction            = vec4(direction, 0.0f),
            .ambientPercentage    = ambientPercent,
            .attenuationLinear    = 0.0f,
            .attenuationQuadratic = 0.0f,
            .type                 = LightDeviceData::Type::eSunLight
        }
    );
}

auto trc::LightRegistry::makePointLight(
    vec3 color,
    vec3 position,
    float attLinear,
    float attQuadratic) -> PointLight
{
    return makeLight<PointLightInterface>(
        pointLights,
        LightDeviceData{
            .color                = vec4(color, 1.0f),
            .position             = vec4(position, 1.0f),
            .direction            = vec4(0.0f),
            .ambientPercentage    = 0.0f,
            .attenuationLinear    = attLinear,
            .attenuationQuadratic = attQuadratic,
            .type                 = LightDeviceData::Type::ePointLight
        }
    );
}

auto trc::LightRegistry::makeAmbientLight(vec3 color) -> AmbientLight
{
    return makeLight<AmbientLightInterface>(
        ambientLights,
        LightDeviceData{
            .color                = vec4(color, 1.0f),
            .position             = vec4(0.0f),
            .direction            = vec4(0.0f),
            .ambientPercentage    = 1.0f,
            .attenuationLinear    = 0.0f,
            .attenuationQuadratic = 0.0f,
            .type                 = LightDeviceData::Type::eAmbientLight
        }
    );
}

template<typename LightType>
auto trc::LightRegistry::makeLight(
    const s_ptr<LightDataStorage>& storage,
    const LightDeviceData& init)
    -> LightHandle<LightType>
{
    auto [id, data] = storage->allocLight(init);

    return LightHandle<LightType>{
        new LightType{ impl::LightInterfaceBase{data} },
        [id, storage](LightType* ptr) {
            storage->freeLight(id);
            delete ptr;
        }
    };
}

auto trc::LightRegistry::getRequiredLightDataSize() const -> size_t
{
    return kHeaderSize
        + kLightSize * sunLights->size()
        + kLightSize * pointLights->size()
        + kLightSize * ambientLights->size();
}

void trc::LightRegistry::writeLightData(ui8* buf) const
{
    // Initialize header
    ui32* header = reinterpret_cast<ui32*>(buf);
    header[0] = sunLights->size();
    header[1] = pointLights->size();
    header[2] = ambientLights->size();

    // Write light data
    auto lightBuf = reinterpret_cast<LightDeviceData*>(buf + kHeaderSize);

    size_t i = 0;
    for (const LightDeviceData& light : *sunLights) {
        lightBuf[i++] = light;
    }
    for (const LightDeviceData& light : *pointLights) {
        lightBuf[i++] = light;
    }
    for (const LightDeviceData& light : *ambientLights) {
        lightBuf[i++] = light;
    }
}
