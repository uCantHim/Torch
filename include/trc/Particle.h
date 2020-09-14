#pragma once

#include <vector>
#include <mutex>

#include <glm/gtc/quaternion.hpp>
#include <vkb/util/Timer.h>
#include <vkb/Buffer.h>
#include <vkb/MemoryPool.h>

#include "Boilerplate.h"
#include "utils/Util.h"
#include "utils/ThreadPool.h"
#include "base/SceneBase.h"
#include "Node.h"
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
        ui32 texture;
    };

    struct ParticlePhysical
    {
        vec3 position{ 0.0f };
        vec3 linearVelocity{ 0.0f };
        vec3 linearAcceleration{ 0.0f };

        quat orientation{ glm::angleAxis(0.0f, vec3{ 0, 1, 0 }) };
        vec3 rotationAxis{ 0, 1, 0 };
        float angularVelocity{ 0.0f };

        vec3 scaling{ 1.0f };

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
        void addParticles(const std::vector<Particle>& particles);

        void setUpdateMethod(ParticleUpdateMethod method);
        void update();

    private:
        class Updater
        {
        public:
            virtual void update(std::vector<ParticlePhysical>& particles,
                                mat4* transformData,
                                ParticleMaterial* materialData) = 0;
        };

        /**
         * @brief Simulates particles on the CPU
         */
        class HostUpdater : public Updater
        {
            void update(std::vector<ParticlePhysical>& particles,
                        mat4* transformData,
                        ParticleMaterial* materialData) override;

            vkb::Timer<std::chrono::microseconds> frameTimer;
        };

        /**
         * @brief Simulates particles on the GPU
         */
        class DeviceUpdater : public Updater
        {
            void update(std::vector<ParticlePhysical>& particles,
                        mat4* transformData,
                        ParticleMaterial* materialData) override;
        };

        const ui32 maxParticles;

        // GPU resources
        std::vector<ParticlePhysical> particles;
        vkb::Buffer particleMatrixStagingBuffer;
        vkb::DeviceLocalBuffer particleMatrixBuffer;
        vkb::Buffer particleMaterialBuffer;

        // Updater
        std::mutex lockParticleUpdate;
        std::unique_ptr<Updater> updater;

        // Drawable registrations
        SceneBase* currentScene{ nullptr };
        std::vector<SceneBase::RegistrationID> sceneRegistrations;

        // Static resources
        static vkb::StaticInit _init;
        static inline std::unique_ptr<vkb::MemoryPool> memoryPool{ nullptr };
        static inline vkb::DeviceLocalBuffer vertexBuffer;
    };

    /**
     * @brief A spawn point for particles
     *
     * Creates particles at a ParticleCollection.
     */
    class ParticleSpawn : public Node
    {
    public:
        explicit ParticleSpawn(ParticleCollection& collection,
                               std::vector<Particle> particles = {});

        void addParticle(Particle particle);

        void spawnParticles();
        // void generateParticles();

    private:
        static inline ThreadPool threads;

        std::vector<Particle> particles;
        ParticleCollection* collection{ nullptr };
    };
}
