#pragma once

#include "Types.h"
#include "Node.h"

namespace trc
{
    /**
     * @brief A camera defining view frustum and projection
     *
     * For siplicity's sake, the camera contains two closely realted, but
     * not necessarily dependent concepts:
     *
     *  - The actual camera transformation, also known as the view matrix,
     *
     *  - The projection matrix, which is often used in comibnation with the
     *    view matrix,
     *
     * The camera has a position, a view direction, and an up-vector. These
     * define the camera matrix.
     *
     * Cameras support two types of projection - perspective and orthogonal.
     * The camera can be set to use a specific type of projection either by
     * constructing it with the respective constructor, or later on with the
     * methods Camera::makePerspective() and Camera::makeOrthogonal(). The
     * default constructor initializes the camera in perspective mode.
     *
     * #define TRC_FLIP_Y_PROJECTION to flip the y-Axis in the projection
     * matrix.
     */
    class Camera : public Node
    {
    public:
        static constexpr float DEFAULT_FOV = 45.0f;

        Camera();

        /**
         * @brief Construct a camera with perspective projection
         *
         * @param float aspect       The viewport's aspect ratio
         * @param float fov          The field of view angle in degrees
         * @param float nearZ Distance of the near clipping plane from the camera
         * @param float farZ  Distance of the far clipping plane from the camera
         */
        Camera(float aspect, float fovDegrees, float zNear, float zFar);

        /**
         * @brief Construct a camera with orthogonal projection
         *
         * The projection rectangle extents are specified in world coordinates,
         * rather than pixels.
         *
         * @param float left         The projection rectange's extent to the left
         * @param float right        The projection rectange's extent to the right
         * @param float bottom       The projection rectange's extent to the bottom
         * @param float top          The projection rectange's extent to the top
         * @param float nearZ Distance of the near clipping plane from the camera
         * @param float farZ  Distance of the far clipping plane from the camera
         */
        Camera(float left, float right, float bottom, float top, float zNear, float zFar);

        auto getViewMatrix() const noexcept -> mat4;
        auto getProjectionMatrix() const noexcept -> const mat4&;

        /**
         * @brief Calculate transformation from look-at parameters
         *
         * Set the node's transformation to the calculated look-at matrix
         *
         * @param vec3 position Camera position
         * @param vec3 point    The point to look at
         * @param vec3 upVector
         */
        void lookAt(vec3 position, vec3 point, vec3 upVector);

        /**
         * @brief Set the distance of the depth clipping planes from the camera
         *
         * @param float nearZ Distance of the near clipping plane from the camera
         * @param float farZ  Distance of the far clipping plane from the camera
         */
        void setDepthBounds(float nearZ, float farZ);

        /**
         * @brief Set the view angle for perspective projection
         *
         * @param float fovDegrees The view angle (field of view) in degrees
         */
        void setFov(float fovDegrees);

        /**
         * @brief Set the aspect ratio
         */
        void setAspect(float aspectRatio);

        /**
         * @brief Set the size of the projection rectangle for orthogonal projection
         *
         * @param float left         The projection rectange's extent to the left
         * @param float right        The projection rectange's extent to the right
         * @param float bottom       The projection rectange's extent to the bottom
         * @param float top          The projection rectange's extent to the top
         */
        void setSizeOrtho(float left, float right, float bottom, float top);

        /**
         * @brief Set the camera's projection mode to perspective projection
         *
         * @param float aspect       The viewport's aspect ratio
         * @param float fov          The field of view angle in degrees
         * @param vec2  depthBounds  The distance of the near and far clipping
         *                           planes from the camera
         */
        void makePerspective(float aspect, float fov, float zNear, float zFar);

        /**
         * @brief Set the camera's projection mode to orthogonal projection
         *
         * @param float left         The projection rectange's extent to the left
         * @param float right        The projection rectange's extent to the right
         * @param float bottom       The projection rectange's extent to the bottom
         * @param float top          The projection rectange's extent to the top
         */
        void makeOrthogonal(float left, float right, float bottom, float top, float zNear, float zFar);

        /**
         * @brief Manually set the projection matrix
         *
         * The matrix is reset when either of setDepthBounds, setFov,
         * setAspect, makePerspective, or makeOrthogonal is called.
         *
         * @param mat4 proj The new projection matrix
         */
        void setProjectionMatrix(mat4 proj) noexcept;

    private:
        void calcProjMatrix();
        bool isOrtho{ false };

        vec2 depthBounds{ 1.0f, 100.0f };
        float fov{ DEFAULT_FOV };
        float aspect{ 1.0f };

        float orthoLeft   { 0.0f };
        float orthoRight  { 0.0f };
        float orthoBottom { 0.0f };
        float orthoTop    { 0.0f };

        mat4 projectionMatrix{ 1.0f };
    };
}
