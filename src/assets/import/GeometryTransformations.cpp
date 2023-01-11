#include "trc/assets/import/GeometryTransformations.h"

#include "trc/base/Logging.h"



namespace trc
{

void computeTangents(GeometryData& result)
{
    if (result.indices.empty() || result.indices.size() % 3 != 0)
    {
        log::error << "Failed to compute tangents. Mesh has no indices or the mesh is not triangulated.\n";
        return;
    }
    if (result.vertices.empty())
    {
        log::error << "Failed to compute tangents. The mesh has no vertices.\n";
        return;
    }

    // Calculate tangents for individual triangles
    for (unsigned int i = 0; i < result.indices.size(); i += 3)
    {
        vec3 A = result.vertices[result.indices[i + 0]].position;
        vec3 B = result.vertices[result.indices[i + 1]].position;
        vec3 C = result.vertices[result.indices[i + 2]].position;

        vec2 H = result.vertices[result.indices[i + 0]].uv;
        vec2 K = result.vertices[result.indices[i + 1]].uv;
        vec2 L = result.vertices[result.indices[i + 2]].uv;

        vec3 D = B - A;
        vec3 E = C - A;

        vec2 F = K - H;
        vec2 G = L - H;

        glm::mat3x2 DE_mat = glm::mat3x2(D.x, E.x, D.y, E.y, D.z, E.z);
        mat2 FG_mat = mat2(F, G);

        glm::mat3x2 TU_mat = (determinant(FG_mat) != 0 ? inverse(FG_mat) : mat2(1.0f)) * DE_mat;

        vec3 T = vec3(TU_mat[0][0], TU_mat[1][0], TU_mat[2][0]); // Tangent
        // vec3 U = vec3(TU_mat[0][1], TU_mat[1][1], TU_mat[2][1]); // Bitangent
        //      ^ currently unused
        vec3 N = normalize(result.vertices[result.indices[i]].normal);

        // Orthonormalize (Gram-Schmidt)
        vec3 T_ = T - dot(N, T) * N;                        // T_ represents the mathematical T'
        //vec3 _U = U - dot(N, U) * N - dot(_T, U) * _T;
        //     ^^ currently unused

        // Because we orthonormalized the T-B-N-Matrix, we can use the *transpose*
        //        | T'.x  T'.y  T'.z |
        //        | U'.x  U'.y  U'.z |
        //        | N.x   N.y   N.z  |
        // as the inverse of the calculated Matrix.
        //
        // This is necessary because the calculated T, U and N vectors are columns of the
        // Tangentspace-to-Objectspace-Matrix. Since we need to transform from object- to
        // tangentspace, we would have to calculate the inverse matrix inside of the shader,
        // which we want to avoid.

        result.vertices[result.indices[i + 0]].tangent = T_;
        result.vertices[result.indices[i + 1]].tangent = T_;
        result.vertices[result.indices[i + 2]].tangent = T_;

        // I don't actually use the bitangent since I can just calculate it in the shader
        // with a simple cross product. Seems better than an additional vertex attribute.
    }

    log::info << result.indices.size() << " tangents and bitangents computed.\n";
}

} // namespace trc
