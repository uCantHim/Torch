#pragma once

#include <vector>
#include <mutex>

#include <glm/gtc/quaternion.hpp>
#include <vkb/util/Timer.h>
#include <vkb/Buffer.h>

#include "Boilerplate.h"
#include "base/SceneBase.h"
#include "PipelineRegistry.h"

namespace trc
{
    namespace internal
    {
        extern void makeParticleDrawPipeline();
        extern void makeParticleShadowPipeline();
    }

    struct ParticleMaterial
    {
        bool32 emitting{ false };
        bool32 hasShadow{ true };
        ui32 texture;

        ui32 __padding{ 0 };
    };

    struct ParticlePhysical
    {
        vec3 position;
        vec3 linearVelocity;

        quat orientation{ glm::angleAxis(0.0f, vec3{ 0, 1, 0 }) };
        vec3 rotationAxis{ 0, 1, 0 };
        float angularVelocity{ 0.0f };

        float lifeTime{ 1000.0f };
        float timeLived{ 0.0f };
        bool doRespawn{ false };
    };

    struct Particle
    {
        ParticlePhysical phys;
        ParticleMaterial material;
    };

    enum class ParticleUpdateMethod
    {
        eHost = 0,
        eDevice = 1,

        MAX_ENUM
    };

    /**
     * @brief A collection of particle drawing data
     *
     * A huge pool of particles that draws all particles with one call.
     */
    class ParticleCollection
    {
    public:
        explicit ParticleCollection(
            ui32 maxParticles,
            ParticleUpdateMethod updateMethod = ParticleUpdateMethod::eHost);

        void attachToScene(SceneBase& scene);
        void removeFromScene();

        void addParticle(const Particle& particle);

        void setUpdateMethod(ParticleUpdateMethod method);
        void update();

    private:
        class Updater
        {
        public:
            virtual void update(
                std::vector<ParticlePhysical>& particles,
                mat4* transformData) = 0;
        };

        class HostUpdater : public Updater
        {
            void update(std::vector<ParticlePhysical>& particles, mat4* transformData);
            vkb::Timer<std::chrono::microseconds> frameTimer;
        };
        class DeviceUpdater : public Updater
        {
            void update(std::vector<ParticlePhysical>& particles, mat4* transformData);
        };

        static vkb::StaticInit _init;
        static inline vkb::DeviceLocalBuffer vertexBuffer;

        const ui32 maxParticles;

        std::vector<ParticlePhysical> particles;
        std::vector<ParticleMaterial> materials;
        vkb::Buffer particleMatrixStagingBuffer;
        vkb::DeviceLocalBuffer particleMatrixBuffer;
        vkb::DeviceLocalBuffer particleMaterialBuffer;

        std::mutex lockParticleUpdate;
        std::unique_ptr<Updater> updater;

        SceneBase* currentScene{ nullptr };
        std::vector<SceneBase::RegistrationID> sceneRegistrations;

        // Register pipeline creation
        static inline const bool _particle_pipeline_register = []() {
            PipelineRegistry::registerPipeline(internal::makeParticleDrawPipeline);
            PipelineRegistry::registerPipeline(internal::makeParticleShadowPipeline);
            return true;
        }();
    };

    /**
     * @brief A spawn point for particles
     *
     * Creates particles at a ParticleCollection.
     */
    class ParticleSpawn
    {
    public:
    };
}
