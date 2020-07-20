#pragma once

#include "Boilerplate.h"

namespace trc
{
    /**
     * @brief The rectangle into which the framebuffer is drawn
     *
     * Although it's often the case, the viewport doesn't have to have the
     * same size as the window,
     *
     * Size and offset are specified in pixels.
     *
     * @property uvec2 offset The position of the viewport in pixels.
     *                        Origin is in the bottom-left corner.
     * @property uvec2 size   Size of the viewport in pixels.
     */
    struct Viewport
    {
        ivec2 offset{ 0, 0 };
        uvec2 size{ 0, 0 };
    };


    /**
     * @brief A camera defining view frustum, projection, and viewport
     *
     * For siplicity's sake, the camera contains three closely realted, but
     * not necessarily dependent concepts:
     *
     *  - The actual camera transformation, also known as the view matrix,
     *
     *  - The projection matrix, which is often used in comibnation with the
     *    view matrix,
     *
     *  - A viewport, which defines a rectangle on the two-dimensional window
     *    screen. The viewport size also defines the aspect ration for
     *    perspective projection.
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
     * matrix. This also causes the face winding to change.
     */
    class Camera
    {
    public:
        static constexpr float DEFAULT_FOV = 45.0f;

        Camera() = default;

        /**
         * @brief Construct a camera with perspective projection
         *
         * @param uvec2 viewportSize The size of the viewport in pixels. Defines
         *                           the aspect ratio for perspective projection.
         * @param float fov          The field of view angle in degrees
         * @param vec2  depthBounds  The distance of the near and far clipping
         *                           planes from the camera
         */
        Camera(Viewport viewport, float fovDegrees, vec2 depthBounds);

        /**
         * @brief Construct a camera with orthogonal projection
         *
         * The projection rectangle extents are specified in world coordinates,
         * rather than pixels.
         *
         * @param uvec2 viewportSize The size of the viewport in pixels. Defines
         *                           the aspect ratio for perspective projection.
         * @param float left         The projection rectange's extent to the left
         * @param float right        The projection rectange's extent to the right
         * @param float bottom       The projection rectange's extent to the bottom
         * @param float top          The projection rectange's extent to the top
         * @param vec2  depthBounds  The distance of the near and far clipping
         *                           planes from the camera
         */
        Camera(Viewport viewport, float left, float right, float bottom, float top, vec2 depthBounds);

        mat4 getViewMatrix() const noexcept;
        mat4 getProjectionMatrix() const noexcept;
        vec3 getPosition() const noexcept;
        vec3 getForwardVector() const noexcept;
        vec3 getUpVector() const noexcept;
        auto getViewport() const noexcept -> const Viewport&;

        void setPosition(vec3 newPos);
        void setForwardVector(vec3 forward);
        void setUpVector(vec3 up);

        /**
         * @brief Set the viewport size
         *
         * The viewport size is specified in pixels.
         *
         * @param Viewport newViewport The viewport
         */
        void setViewport(Viewport newViewport);

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
         * @param uvec2 viewportSize The size of the viewport in pixels. Defines
         *                           the aspect ratio for perspective projection.
         * @param float fov          The field of view angle in degrees
         * @param vec2  depthBounds  The distance of the near and far clipping
         *                           planes from the camera
         */
        void makePerspective(Viewport viewport, float fov, float zNear, float zFar);

        /**
         * @brief Set the camera's projection mode to orthogonal projection
         *
         * @param float left         The projection rectange's extent to the left
         * @param float right        The projection rectange's extent to the right
         * @param float bottom       The projection rectange's extent to the bottom
         * @param float top          The projection rectange's extent to the top
         */
        void makeOrthogonal(float left, float right, float bottom, float top, float zNear, float zFar);

    private:
        void calcViewMatrix();
        void calcProjMatrix();
        bool isOrtho{ false };

        // View things
        vec3 position{ 0.0f };
        vec3 forwardVector{ 0.0f, 0.0f, -1.0f };
        vec3 upVector{ 0.0f, 1.0f, 0.0f };

        mat4 viewMatrix{ 1.0f };

        // Projection things
        Viewport viewport;
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
