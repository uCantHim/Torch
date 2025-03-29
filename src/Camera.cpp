#include "trc/Camera.h"

#include <glm/gtc/matrix_transform.hpp>

#include <trc_util/TypeUtils.h>



trc::Camera::Camera(float aspect, float fovDegrees, float zNear, float zFar)
{
    makePerspective(aspect, fovDegrees, zNear, zFar);
}

trc::Camera::Camera(float left, float right, float bottom, float top, float zNear, float zFar)
{
    makeOrthogonal(left, right, bottom, top, zNear, zFar);
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
    if (auto p = std::get_if<Perspective>(&projectionParams))
    {
        p->depthBounds = vec2(minDepth, maxDepth);
        calcProjMatrix();
    }
}

void trc::Camera::setFov(float newFov)
{
    if (auto p = std::get_if<Perspective>(&projectionParams))
    {
        p->fov = newFov;
        calcProjMatrix();
    }
}

void trc::Camera::setAspect(float aspectRatio)
{
    if (auto p = std::get_if<Perspective>(&projectionParams))
    {
        p->aspect = aspectRatio;
        calcProjMatrix();
    }
}

void trc::Camera::makePerspective(float _aspect, float _fov, float zNear, float zFar)
{
    projectionParams = Perspective{
        .depthBounds = vec2(zNear, zFar),
        .fov = _fov,
        .aspect = _aspect,
    };
    calcProjMatrix();
}

void trc::Camera::makeOrthogonal(float left, float right, float bottom, float top, float zNear, float zFar)
{
    projectionParams = Orthogonal{
        .left = left,
        .right = right,
        .bottom = bottom,
        .top = top,
        .front = zNear,
        .back = zFar,
    };
    calcProjMatrix();
}

void trc::Camera::setProjectionMatrix(mat4 proj) noexcept
{
    projectionMatrix = proj;
}

auto trc::Camera::project(vec3 worldPos) const -> vec4
{
    const vec4 proj = getProjectionMatrix() * getViewMatrix() * vec4(worldPos, 1.0f);
    return (proj / proj.w) * vec4(0.5f, 0.5f, 1, 1) + vec4(0.5f, 0.5f, 0, 0);
}

auto trc::Camera::unproject(
    const vec2 screenPos,
    const float screenDepth,
    const uvec2 viewportSize) const -> vec3
{
    const vec4 clipSpace = vec4(screenPos / vec2(viewportSize) * 2.0f - 1.0f, screenDepth, 1.0);
    const vec4 viewSpace = glm::inverse(getProjectionMatrix()) * clipSpace;
    const vec4 worldSpace = glm::inverse(getViewMatrix()) * (viewSpace / viewSpace.w);

    return worldSpace;
}

auto trc::Camera::calcScreenDepth(const vec3 worldPos) const -> float
{
    return project(worldPos).z;
}

void trc::Camera::calcProjMatrix()
{
    projectionMatrix = std::visit(util::VariantVisitor{
        [](const Perspective& p) {
            return glm::perspective(glm::radians(p.fov), p.aspect, p.depthBounds.x, p.depthBounds.y);
        },
        [](const Orthogonal& o) {
            return glm::ortho(o.left, o.right, o.bottom, o.top, o.front, o.back);
        },
    }, projectionParams);
}
