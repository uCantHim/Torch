#include "Transformation.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>



trc::Transformation::Transformation()
{
    matrixIndex.set(mat4(1.0f));
}

trc::Transformation::Transformation(glm::vec3 translation, vec3 scale, glm::quat rotation)
	:
	translation(translation),
	scaling(scale),
	rotation(rotation)
{
}

auto trc::Transformation::setFromMatrix(const mat4& t) -> self&
{
    vec3 skew;
    vec4 perspective;

    glm::decompose(t, scaling, rotation, translation, skew, perspective);
    updateMatrix();

    return *this;
}

void trc::Transformation::setFromMatrixTemporary(const mat4& t)
{
    matrixIndex.set(t);
}

void trc::Transformation::clearTransformation()
{
    setTranslation(0.0f, 0.0f, 0.0f);
    setScale(1.0f);
    setRotation(glm::angleAxis(0.0f, vec3(0.0f, 1.0f, 0.0f)));
    matrixIndex.set(mat4(1.0f));
}


// Translation
auto trc::Transformation::translate(float x, float y, float z) -> self&
{
	translation.x += x;
	translation.y += y;
	translation.z += z;

	updateMatrix();
    return *this;
}

auto trc::Transformation::translate(glm::vec3 t) -> self&
{
	translation += t;

	updateMatrix();
    return *this;
}

auto trc::Transformation::translateX(float x) -> self&
{
    return translate(x, 0, 0);
}

auto trc::Transformation::translateY(float y) -> self&
{
    return translate(0, y, 0);
}

auto trc::Transformation::translateZ(float z) -> self&
{
    return translate(0, 0, z);
}

auto trc::Transformation::setTranslation(float x, float y, float z) -> self&
{
	translation.x = x;
	translation.y = y;
	translation.z = z;

	updateMatrix();
    return *this;
}

auto trc::Transformation::setTranslation(glm::vec3 t) -> self&
{
	translation = t;

	updateMatrix();
    return *this;
}

auto trc::Transformation::setTranslationX(float x) -> self&
{
	translation.x = x;

	updateMatrix();
    return *this;
}

auto trc::Transformation::setTranslationY(float y) -> self&
{
	translation.y = y;

	updateMatrix();
    return *this;
}

auto trc::Transformation::setTranslationZ(float z) -> self&
{
	translation.z = z;

	updateMatrix();
    return *this;
}


// Scale
auto trc::Transformation::addScale(float s) -> self&
{
	scaling += s;

	updateMatrix();
    return *this;
}

auto trc::Transformation::addScale(vec3 s) -> self&
{
	scaling += s;

	updateMatrix();
    return *this;
}

auto trc::Transformation::scale(float s) -> self&
{
    return setScale(scaling * s);
}

auto trc::Transformation::scale(float x, float y, float z) -> self&
{
    return setScale(scaling * vec3(x, y, z));
}

auto trc::Transformation::scale(vec3 s) -> self&
{
    return setScale(scaling * s);
}

auto trc::Transformation::setScale(float s) -> self&
{
	scaling = vec3(s);

	updateMatrix();
    return *this;
}

auto trc::Transformation::setScale(float x, float y, float z) -> self&
{
    scaling = vec3(x, y, z);

    updateMatrix();
    return *this;
}

auto trc::Transformation::setScale(vec3 s) -> self&
{
    scaling = s;

    updateMatrix();
    return *this;
}

auto trc::Transformation::setScaleX(float s) -> self&
{
    scaling.x = s;

    updateMatrix();
    return *this;
}

auto trc::Transformation::setScaleY(float s) -> self&
{
    scaling.y = s;

    updateMatrix();
    return *this;
}

auto trc::Transformation::setScaleZ(float s) -> self&
{
    scaling.z = s;

    updateMatrix();
    return *this;
}


// Rotation
auto trc::Transformation::rotate(float x, float y, float z) -> self&
{
    rotation = quat(vec3(x, y, z));

    updateMatrix();
    return *this;
}

auto trc::Transformation::rotate(float angleRad, glm::vec3 axis) -> self&
{
	rotation = glm::angleAxis(angleRad, axis) * rotation;

	updateMatrix();
    return *this;
}

auto trc::Transformation::rotate(const glm::quat& rot) -> self&
{
	rotation = rot * rotation;

	updateMatrix();
    return *this;
}

auto trc::Transformation::rotateX(float angleRad) -> self&
{
    return rotate(angleRad, vec3(1, 0, 0));
}

auto trc::Transformation::rotateY(float angleRad) -> self&
{
    return rotate(angleRad, vec3(0, 1, 0));
}

auto trc::Transformation::rotateZ(float angleRad) -> self&
{
    return rotate(angleRad, vec3(0, 0, 1));
}

auto trc::Transformation::setRotation(float angleRad, glm::vec3 axis) -> self&
{
	rotation = glm::angleAxis(angleRad, axis);

	updateMatrix();
    return *this;
}

auto trc::Transformation::setRotation(const glm::quat& rot) -> self&
{
	rotation = rot;

	updateMatrix();
    return *this;
}



auto trc::Transformation::getTransformationMatrix() const -> mat4
{
    return matrixIndex.get();
}

auto trc::Transformation::getTranslation() const -> vec3
{
    return translation;
}

auto trc::Transformation::getTranslationAsMatrix() const -> mat4
{
    return glm::translate(mat4(1.0f), translation);
}

auto trc::Transformation::getScale() const -> vec3
{
    return scaling;
}

auto trc::Transformation::getScaleAsMatrix() const -> mat4
{
    return mat4(scaling.x, 0.0f, 0.0f, 0.0f,
                0.0f, scaling.y, 0.0f, 0.0f,
                0.0f, 0.0f, scaling.z, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f);
}

auto trc::Transformation::getRotation() const -> quat
{
    return rotation;
}

auto trc::Transformation::getRotationAngleAxis() const -> std::pair<float, vec3>
{
    return { glm::angle(rotation), glm::axis(rotation) };
}

auto trc::Transformation::getRotationAsMatrix() const -> mat4
{
    return glm::mat4_cast(rotation);
}

auto trc::Transformation::getMatrixId() const -> ID
{
    return matrixIndex.getDataId();
}

void trc::Transformation::updateMatrix()
{
    matrixIndex.set(
        getTranslationAsMatrix()
        * getRotationAsMatrix()
        * getScaleAsMatrix()
    );

    onLocalMatrixUpdate();
}
