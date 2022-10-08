#pragma once

#include <mutex>
#include <iostream>

#include <glm/gtc/quaternion.hpp>

#include "trc/Types.h"
#include "trc_util/data/ExternalStorage.h"

namespace trc
{
    /**
     * @brief A transformation in 3D-space
     *
     * The rotation is implemented with quaternions.
     */
    class Transformation
    {
    public:
        using ID = data::ExternalStorage<mat4>::ID;
        using self = Transformation;

        Transformation();
        Transformation(vec3 translation, vec3 scale, quat rotation);

        Transformation(const Transformation& other) = default;
        Transformation(Transformation&& other) noexcept = default;
        virtual ~Transformation() = default;

        auto operator=(const Transformation& rhs) -> Transformation& = default;
        auto operator=(Transformation&& rhs) noexcept -> Transformation& = default;

        /**
         * @brief Set translation, rotation, and scaling from a matrix
         */
        auto setFromMatrix(const mat4& t) -> self&;

        /**
         * This method is way faster than setFromMatrix(), but the transform
         * information is lost when calling any other method.
         *
         * This is intended to be used when the transformation in the matrix
         * is never changed manually. A typical use case for this is to get the
         * transformation of a physics object and copy its whole transformation
         * to this one.
         */
        void setFromMatrixTemporary(const mat4& t);

        /**
         * @brief Reset the transformation to the identity
         */
        void clearTransformation();

        auto translate(float x, float y, float z) -> self&;
        auto translate(vec3 t) -> self&;
        auto translateX(float x) -> self&;
        auto translateY(float y) -> self&;
        auto translateZ(float z) -> self&;

        auto setTranslation(float x, float y, float z) -> self&;
        auto setTranslation(vec3 t) -> self&;
        auto setTranslationX(float x) -> self&;
        auto setTranslationY(float y) -> self&;
        auto setTranslationZ(float z) -> self&;

        auto scale(float s) -> self&;
        auto scale(float x, float y, float z) -> self&;
        auto scale(vec3 s) -> self&;
        auto addScale(float s) -> self&;
        auto addScale(vec3 s) -> self&;

        auto setScale(float s) -> self&;
        auto setScale(float x, float y, float z) -> self&;
        auto setScale(vec3 s) -> self&;
        auto setScaleX(float s) -> self&;
        auto setScaleY(float s) -> self&;
        auto setScaleZ(float s) -> self&;

        auto rotate(float x, float y, float z) -> self&;
        auto rotate(float angleRad, vec3 axis) -> self&;
        auto rotate(const quat& rot) -> self&;
        auto rotateX(float angleRad) -> self&;
        auto rotateY(float angleRad) -> self&;
        auto rotateZ(float angleRad) -> self&;

        auto setRotation(const quat& rot) -> self&;
        auto setRotation(float angleRad, vec3 axis) -> self&;

        /**
         * @return All transformations combined into one matrix
         *
         * The order of application is scale -> rotation -> translation
         */
        auto getTransformationMatrix() const -> mat4;

        /**
         * @return vec3 The total translation in all three directions
         */
        auto getTranslation() const -> vec3;

        /**
         * @return mat4 A matrix that contains only translation information.
         */
        auto getTranslationAsMatrix() const -> mat4;

        /**
         * @brief The total uniform scaling
         */
        auto getScale() const -> vec3;

        /**
         * @return mat4 A matrix that contains only scaling information.
         */
        auto getScaleAsMatrix() const -> mat4;

        /**
         * @return quat The total rotation as quaternion
         */
        auto getRotation() const -> quat;

        /**
         * @return <float, vec3> The rotation represented as an axis and an
         *                       angle is radians.
         */
        auto getRotationAngleAxis() const -> std::pair<float, vec3>;

        /**
         * @return mat4 A matrix that contains only rotation information.
         */
        auto getRotationAsMatrix() const -> mat4;

        auto getMatrixId() const -> ID;

    protected:
        /**
         * Used in Node to decide whether the local matrix or the global
         * matrix should be stored in the MatrixStorage slot.
         *
         * I tried a lot of things that didn't use a virtual call, but in
         * the end, this now seems to be the best option. It is not called
         * every frame, only every time the transformation changes.
         */
        virtual void onLocalMatrixUpdate() {};

    private:
        void updateMatrix();

        data::ExternalStorage<mat4> matrixIndex;

        vec3 translation{ 0.0f };
        vec3 scaling{ 1.0f };
        quat rotation{ glm::angleAxis(0.0f, vec3(0.0f, 1.0f, 0.0f)) };
    };
} // namespace trc
