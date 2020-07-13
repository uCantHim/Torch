#include "utils/Transformation.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>



trc::Transformation::Transformation(glm::vec3 translation, vec3 scale, glm::quat rotation)
	:
	translation(translation),
	scaling(scale),
	rotation(rotation)
{
}

auto trc::Transformation::data() const noexcept -> const uint8_t*
{
    return reinterpret_cast<const uint8_t*>(&matrixRepresentation[0][0]);
}

auto trc::Transformation::setFromMatrix(const mat4& t) -> self&
{
    vec3 skew;
    vec4 perspective;

    glm::decompose(t, scaling, rotation, translation, skew, perspective);
    matrixDirty = true;

    return *this;
}

void trc::Transformation::setFromMatrixTemporary(const mat4& t)
{
	matrixRepresentation = t;
}


// Translation
auto trc::Transformation::translate(float x, float y, float z) -> self&
{
	translation.x += x;
	translation.y += y;
	translation.z += z;

	matrixDirty = true;
    return *this;
}

auto trc::Transformation::translate(glm::vec3 t) -> self&
{
	translation += t;

	matrixDirty = true;
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

	matrixDirty = true;
    return *this;
}

auto trc::Transformation::setTranslation(glm::vec3 t) -> self&
{
	translation = t;

	matrixDirty = true;
    return *this;
}

auto trc::Transformation::setTranslationX(float x) -> self&
{
	translation.x = x;

	matrixDirty = true;
    return *this;
}

auto trc::Transformation::setTranslationY(float y) -> self&
{
	translation.y = y;

	matrixDirty = true;
    return *this;
}

auto trc::Transformation::setTranslationZ(float z) -> self&
{
	translation.z = z;

	matrixDirty = true;
    return *this;
}


// Scale
auto trc::Transformation::addScale(float s) -> self&
{
	scaling += s;

	matrixDirty = true;
    return *this;
}

auto trc::Transformation::addScale(vec3 s) -> self&
{
	scaling += s;

	matrixDirty = true;
    return *this;
}

auto trc::Transformation::setScale(float s) -> self&
{
	scaling = vec3(s);

	matrixDirty = true;
    return *this;
}

auto trc::Transformation::setScale(vec3 s) -> self&
{
    scaling = s;

    matrixDirty = true;
    return *this;
}

auto trc::Transformation::setScaleX(float s) -> self&
{
    scaling.x = s;

    matrixDirty = true;
    return *this;
}

auto trc::Transformation::setScaleY(float s) -> self&
{
    scaling.y = s;

    matrixDirty = true;
    return *this;
}

auto trc::Transformation::setScaleZ(float s) -> self&
{
    scaling.z = s;

    matrixDirty = true;
    return *this;
}


// Rotation
auto trc::Transformation::rotate(float angleRad, glm::vec3 axis) -> self&
{
	rotation = glm::angleAxis(angleRad, axis) * rotation;

	matrixDirty = true;
    return *this;
}

auto trc::Transformation::rotate(const glm::quat& rot) -> self&
{
	rotation = rot * rotation;

	matrixDirty = true;
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

	matrixDirty = true;
    return *this;
}

auto trc::Transformation::setRotation(const glm::quat& rot) -> self&
{
	rotation = rot;

	matrixDirty = true;
    return *this;
}



const glm::mat4& trc::Transformation::getTransformationMatrix() const
{
	if (matrixDirty)
	{
		matrixRepresentation =
            glm::translate(glm::mat4(1.0f), translation)
            * glm::mat4(rotation)
            * getScaleAsMatrix();
		matrixDirty = false;
	}

	return matrixRepresentation;
}

vec3 trc::Transformation::getTranslation() const
{
     return translation;
}

mat4 trc::Transformation::getTranslationAsMatrix() const
{
     return glm::translate(mat4(1.0f), translation);
}

vec3 trc::Transformation::getScale() const
{
     return scaling;
}

mat4 trc::Transformation::getScaleAsMatrix() const
{
     return mat4(scaling.x, 0.0f, 0.0f, 0.0f,
                 0.0f, scaling.y, 0.0f, 0.0f,
                 0.0f, 0.0f, scaling.z, 0.0f,
                 0.0f, 0.0f, 0.0f, 1.0f);
}

quat trc::Transformation::getRotation() const
{
     return rotation;
}

auto trc::Transformation::getRotationAngleAxis() const -> std::pair<float, vec3>
{
    return { glm::angle(rotation), glm::axis(rotation) };
}

mat4 trc::Transformation::getRotationAsMatrix() const
{
     return glm::mat4_cast(rotation);
}
