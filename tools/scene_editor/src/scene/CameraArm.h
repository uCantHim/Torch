#pragma once

#include <trc/Camera.h>
#include <trc/Node.h>
#include <trc/Types.h>

using namespace trc::basic_types;

struct CameraArm : trc::Node
{
    CameraArm(s_ptr<trc::Camera> _camera, vec3 eyePos, vec3 center, vec3 up)
        : camera(_camera)
    {
        auto order = [](std::vector<trc::Node*> nodes) {
            for (size_t i = 0; i < nodes.size() - 1; ++i) {
                nodes[i]->attach(*nodes[i+1]);
            }
        };

        this->setFromMatrix(glm::lookAt(eyePos, center, up));
        order({
            this,
            &xRotationNode,
            &yRotationNode,
            &zoomNode,
            &translationNode,
            camera.get(),
        });
    }

    auto getCameraWorldPos() const -> vec3
    {
        return vec3{ glm::inverse(camera->getViewMatrix())[3] };
    }

    /**
     * @brief Move the pivot point.
     */
    void translate(float x, float y, float z)
    {
        translationNode.translate(x, y, z);
    }

    /**
     * @brief Rotate around the pivot point.
     *
     * There is no z rotation because we never want to tilt the camera.
     */
    void rotate(float x, float y)
    {
        xRotationNode.rotateX(x);
        yRotationNode.rotateY(y);
    }

    /**
     * @brief Zoom toward the pivot point.
     *
     * Zoom decays logarithmically when zooming in, meaning the zoom will never
     * invert the camera view.
     */
    void setZoomLevel(int level)
    {
        if (level == 0) {
            zoomNode.setScale(1.0f);
        }
        else if (level < 0) {
            zoomNode.setScale(1.0f / -(level - 1));
        }
        else {
            zoomNode.setScale(level + 1);
        }
    }

private:
    // *this is the view node
    trc::Node translationNode;
    trc::Node xRotationNode;
    trc::Node yRotationNode;
    trc::Node zoomNode;
    s_ptr<trc::Camera> camera;
};
