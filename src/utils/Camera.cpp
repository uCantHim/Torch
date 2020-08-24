#include "utils/Camera.h"

#include <glm/gtc/matrix_transform.hpp>



trc::Camera::Camera(float aspect, float fovDegrees, float zNear, float zFar)
    :
    depthBounds({ zNear, zFar }),
    fov(fovDegrees),
    aspect(aspect)
{
	calcViewMatrix();
    makePerspective(aspect, fovDegrees, depthBounds.x, depthBounds.y);
}

trc::Camera::Camera(float left, float right, float bottom, float top, float zNear, float zFar)
    :
    depthBounds({ zNear, zFar })
{
    calcViewMatrix();
    makeOrthogonal(left, right, bottom, top, depthBounds.x, depthBounds.y);
}

mat4 trc::Camera::getViewMatrix() const noexcept
{
	return viewMatrix;
}

mat4 trc::Camera::getProjectionMatrix() const noexcept
{
	return projectionMatrix;
}

vec3 trc::Camera::getPosition() const noexcept
{
	return position;
}

vec3 trc::Camera::getForwardVector() const noexcept
{
	return forwardVector;
}

vec3 trc::Camera::getUpVector() const noexcept
{
	return upVector;
}

void trc::Camera::setPosition(vec3 newPos)
{
	position = newPos;
    calcViewMatrix();
}

void trc::Camera::setForwardVector(vec3 forward)
{
	forwardVector = forward;
	calcViewMatrix();
}

void trc::Camera::setUpVector(vec3 up)
{
	upVector = up;
	calcViewMatrix();
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

void trc::Camera::calcViewMatrix()
{
	viewMatrix = glm::lookAt(position, position + forwardVector, upVector);
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
