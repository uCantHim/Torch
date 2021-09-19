#include "Camera.h"

#include <glm/gtc/matrix_transform.hpp>



trc::Camera::Camera()
{
    makePerspective(16.0f / 9.0f, 45.0f, 0.1f, 100.0f);
}

trc::Camera::Camera(float aspect, float fovDegrees, float zNear, float zFar)
    :
    depthBounds({ zNear, zFar }),
    fov(fovDegrees),
    aspect(aspect)
{
    makePerspective(aspect, fovDegrees, depthBounds.x, depthBounds.y);
}

trc::Camera::Camera(float left, float right, float bottom, float top, float zNear, float zFar)
    :
    depthBounds({ zNear, zFar })
{
    makeOrthogonal(left, right, bottom, top, depthBounds.x, depthBounds.y);
}

auto trc::Camera::getViewMatrix() const noexcept -> mat4
{
	return getGlobalTransform();
}

auto trc::Camera::getProjectionMatrix() const noexcept -> const mat4&
{
	return projectionMatrix;
}

void trc::Camera::lookAt(vec3 position, vec3 point, vec3 upVector)
{
    setFromMatrix(glm::lookAt(position, point, upVector));
}

void trc::Camera::setDepthBounds(float minDepth, float maxDepth)
{
	depthBounds = vec2(minDepth, maxDepth);
	calcProjMatrix();
}

void trc::Camera::setFov(float newFov)
{
	fov = newFov;
	calcProjMatrix();
}

void trc::Camera::setAspect(float aspectRatio)
{
    aspect = aspectRatio;
    calcProjMatrix();
}

void trc::Camera::makePerspective(float _aspect, float _fov, float zNear, float zFar)
{
    isOrtho = false;
    aspect = _aspect;
    fov = _fov;
    depthBounds = vec2(zNear, zFar);
    calcProjMatrix();
}

void trc::Camera::makeOrthogonal(float left, float right, float bottom, float top, float zNear, float zFar)
{
    isOrtho = true;
    orthoLeft = left;
    orthoRight = right;
    orthoBottom = bottom;
    orthoTop = top;
    depthBounds = vec2(zNear, zFar);
    calcProjMatrix();
}

void trc::Camera::setProjectionMatrix(mat4 proj) noexcept
{
    projectionMatrix = proj;

#ifdef TRC_FLIP_Y_PROJECTION
    static const GLM_CONSTEXPR mat4 axisFlipMatrix(
        1, 0, 0, 0,
        0, -1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    );
    projectionMatrix *= axisFlipMatrix;
#endif
}

void trc::Camera::calcProjMatrix()
{
    if (isOrtho)
        projectionMatrix = glm::ortho(orthoLeft, orthoRight, orthoBottom, orthoTop, depthBounds.x, depthBounds.y);
    else
        projectionMatrix = glm::perspective(glm::radians(fov), aspect, depthBounds.x, depthBounds.y);

#ifdef TRC_FLIP_Y_PROJECTION
    static const GLM_CONSTEXPR mat4 axisFlipMatrix(
        1, 0, 0, 0,
        0, -1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    );
    projectionMatrix *= axisFlipMatrix;
#endif
}
