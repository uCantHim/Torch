// Utilities for ray hit calculations

vec2 calcHitUv(vec3 baryCoords, uint geoIndex)
{
    uint baseIndex = gl_PrimitiveID * 3;
    uint indices[] = {
        indexBuffers[geoIndex].indices[baseIndex],
        indexBuffers[geoIndex].indices[baseIndex + 1],
        indexBuffers[geoIndex].indices[baseIndex + 2],
    };

    // Calculate an interpolated surface UV coordinate
    vec2 u1 = GET_VERTEX_UV(vertexBuffers[geoIndex].vertices[indices[0]]);
    vec2 u2 = GET_VERTEX_UV(vertexBuffers[geoIndex].vertices[indices[1]]);
    vec2 u3 = GET_VERTEX_UV(vertexBuffers[geoIndex].vertices[indices[2]]);
    vec2 uv = u1 * baryCoords.x + u2 * baryCoords.y + u3 * baryCoords.z;

    return uv;
}

vec3 calcHitNormal(vec3 baryCoords, uint geoIndex, uint matIndex)
{
    mat4 model = mat4(vec4(gl_ObjectToWorldEXT[0], 0),
                      vec4(gl_ObjectToWorldEXT[1], 0),
                      vec4(gl_ObjectToWorldEXT[2], 0),
                      vec4(gl_ObjectToWorldEXT[3], 1));

    // Query vertex indices
    uint baseIndex = gl_PrimitiveID * 3;
    uint indices[] = {
        indexBuffers[geoIndex].indices[baseIndex],
        indexBuffers[geoIndex].indices[baseIndex + 1],
        indexBuffers[geoIndex].indices[baseIndex + 2],
    };

    // Calculate an interpolated surface normal
    vec3 n1 = GET_VERTEX_NORMAL(vertexBuffers[geoIndex].vertices[indices[0]]);
    vec3 n2 = GET_VERTEX_NORMAL(vertexBuffers[geoIndex].vertices[indices[1]]);
    vec3 n3 = GET_VERTEX_NORMAL(vertexBuffers[geoIndex].vertices[indices[2]]);
    vec3 N = normalize(n1 * baryCoords.x + n2 * baryCoords.y + n3 * baryCoords.z);
    N = normalize((transpose(inverse(model)) * vec4(N, 0.0)).xyz);

    uint bumpMap = materials[matIndex].bumpTexture;
    if (bumpMap != NO_TEXTURE)
    {
        // Calculate an interpolated surface UV coordinate
        vec2 u1 = GET_VERTEX_UV(vertexBuffers[geoIndex].vertices[indices[0]]);
        vec2 u2 = GET_VERTEX_UV(vertexBuffers[geoIndex].vertices[indices[1]]);
        vec2 u3 = GET_VERTEX_UV(vertexBuffers[geoIndex].vertices[indices[2]]);
        vec2 uv = u1 * baryCoords.x + u2 * baryCoords.y + u3 * baryCoords.z;

        // Calculate an interpolated surface normal
        vec3 t1 = GET_VERTEX_TANGENT(vertexBuffers[geoIndex].vertices[indices[0]]);
        vec3 t2 = GET_VERTEX_TANGENT(vertexBuffers[geoIndex].vertices[indices[1]]);
        vec3 t3 = GET_VERTEX_TANGENT(vertexBuffers[geoIndex].vertices[indices[2]]);
        vec3 T = normalize(t1 * baryCoords.x + t2 * baryCoords.y + t3 * baryCoords.z);
        T = normalize((transpose(inverse(model)) * vec4(T, 0.0)).xyz);

        vec3 B = cross(N, T);

        // Apply bump map
        vec3 bumpNormal = texture(textures[bumpMap], uv).rgb * 2.0 - 1.0;

        N = normalize(mat3(T, B, N) * bumpNormal);
    }

    return N;
}
