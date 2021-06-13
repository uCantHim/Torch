#include "utils/Transformation.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>



trc::Transformation::Transformation()
{
    matrices.set(matrixIndex, mat4(1.0f));
}

trc::Transformation::Transformation(glm::vec3 translation, vec3 scale, glm::quat rotation)
	:
	translation(translation),
	scaling(scale),
	rotation(rotation)
{
}

trc::Transformation::Transformation(const Transformation& other)
    :
    translation(other.translation),
    scaling(other.scaling),
    rotation(other.rotation)
{
    updateMatrix();
}

trc::Transformation::Transformation(Transformation&& other) noexcept
    :
    translation(other.translation),
    scaling(other.scaling),
    rotation(other.rotation)
{
    other.translation = vec3(0.0f);
    other.scaling = vec3(1.0f);
    other.rotation = glm::angleAxis(0.0f, vec3(0.0f, 1.0f, 0.0f));
    std::swap(matrixIndex, other.matrixIndex);
}

trc::Transformation::~Transformation()
{
    matrices.free(matrixIndex);
}

auto trc::Transformation::operator=(const Transformation& rhs) -> Transformation&
{
    translation = rhs.translation;
    scaling = rhs.scaling;
    rotation = rhs.rotation;
    updateMatrix();

    return *this;
}

auto trc::Transformation::operator=(Transformation&& rhs) noexcept -> Transformation&
{
    translation = rhs.translation;
    rhs.translation = vec3(0.0f);
    scaling = rhs.scaling;
    rhs.scaling = vec3(1.0f);
    rotation = rhs.rotation;
    rhs.rotation = glm::angleAxis(0.0f, vec3(0.0f, 1.0f, 0.0f));
    std::swap(matrixIndex, rhs.matrixIndex);

    return *this;
}

auto trc::Transformation::data() const noexcept -> const uint8_t*
{
    return reinterpret_cast<const uint8_t*>(matrices.getPtr(matrixIndex));
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
    matrices.set(matrixIndex, t);
}

void trc::Transformation::clearTransformation()
{
    setTranslation(0.0f, 0.0f, 0.0f);
    setScale(1.0f);
    setRotation(glm::angleAxis(0.0f, vec3(0.0f, 1.0f, 0.0f)));
    matrices.set(matrixIndex, mat4(1.0f));
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

auto trc::Transformation::setScale(float s) -> self&
{
	scaling = vec3(s);

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



auto trc::Transformation::getTransformationMatrix() const -> const glm::mat4&
{
	return matrices.get(matrixIndex);
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

auto trc::Transformation::getMatrixId() const -> ui32
{
    return matrixIndex;
}

auto trc::Transformation::getMatrix(ui32 id) -> mat4
{
    return matrices.get(id);
}

void trc::Transformation::updateMatrix()
{
    matrices.set(matrixIndex,
        getTranslationAsMatrix()
        * getRotationAsMatrix()
        * getScaleAsMatrix()
    );
}



// ------------------------ //
//      Matrix storage      //
// ------------------------ //

auto trc::Transformation::MatrixStorage::create() -> ui32
{
    const ui32 id = ui32(idGenerator.generate());
    if (matrices.size() <= id)
    {
        std::lock_guard lk(lock);
        matrices.resize(id + 1);
    }
    matrices[id] = mat4(1.0f);

    return id;
}

void trc::Transformation::MatrixStorage::free(ui32 id)
{
    idGenerator.free(id);
}

auto trc::Transformation::MatrixStorage::get(ui32 id) -> const mat4&
{
    assert(matrices.size() > id);
    return matrices[id];
}

auto trc::Transformation::MatrixStorage::getPtr(ui32 id) -> const void*
{
    assert(matrices.size() > id);
    return &matrices[id];
}

void trc::Transformation::MatrixStorage::set(ui32 id, mat4 mat)
{
    assert(matrices.size() > id);
    matrices[id] = mat;
}
