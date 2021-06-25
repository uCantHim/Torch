#pragma once

#include "TorchResources.h"
#include "Drawable.h"

namespace trc
{
    /**
     * @brief Purely component-based Drawable class
     */
    class DrawableInstanced : public Node
    {
    public:
        struct InstanceDescription
        {
            InstanceDescription() = default;
            InstanceDescription(const mat4& t, ui32 matIndex)
                : modelMatrix(t), materialIndex(matIndex)
            {}
            InstanceDescription(const Transformation& t, ui32 matIndex)
                : modelMatrix(t.getTransformationMatrix()), materialIndex(matIndex)
            {}

            mat4 modelMatrix;
            ui32 materialIndex;
        };

        DrawableInstanced(ui32 maxInstances, Geometry& geo);
        DrawableInstanced(ui32 maxInstances, Geometry& geo, SceneBase& scene);

        void attachToScene(SceneBase& scene);
        void removeFromScene();

        void setGeometry(Geometry& geo);

        void addInstance(InstanceDescription instance);

    private:
        Geometry* geometry;

        ui32 maxInstances;
        ui32 numInstances{ 0 };
        vkb::Buffer instanceDataBuffer;

        SceneBase::RegistrationID deferredRegistration;
        SceneBase::RegistrationID shadowRegistration;
        SceneBase* scene{ nullptr };
    };
} // namespace trc
