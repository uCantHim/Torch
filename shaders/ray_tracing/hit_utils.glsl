// Utilities for ray hit calculations

vec3 calcHitWorldPos()
{
    return gl_WorldRayOriginEXT + gl_HitTEXT * gl_WorldRayDirectionEXT;
}

vec2 calcHitUv(vec3 baryCoords, uint geoIndex, uint primitiveIndex)
{
    const uint baseIndex = primitiveIndex * 3;
    const uint indices[] = {
        indexBuffers[geoIndex].indices[baseIndex],
        indexBuffers[geoIndex].indices[baseIndex + 1],
        indexBuffers[geoIndex].indices[baseIndex + 2],
    };

    // Calculate an interpolated surface UV coordinate
    const vec2 u1 = GET_VERTEX_UV(vertexBuffers[geoIndex].vertices[indices[0]]);
    const vec2 u2 = GET_VERTEX_UV(vertexBuffers[geoIndex].vertices[indices[1]]);
    const vec2 u3 = GET_VERTEX_UV(vertexBuffers[geoIndex].vertices[indices[2]]);
    const vec2 uv = u1 * baryCoords.x + u2 * baryCoords.y + u3 * baryCoords.z;

    return uv;
}

vec3 calcHitNormal(vec3 baryCoords, uint geoIndex, uint primitiveIndex)
{
    const mat4 model = mat4(vec4(gl_ObjectToWorldEXT[0], 0),
                            vec4(gl_ObjectToWorldEXT[1], 0),
                            vec4(gl_ObjectToWorldEXT[2], 0),
                            vec4(gl_ObjectToWorldEXT[3], 1));

    // Query vertex indices
    const uint baseIndex = primitiveIndex * 3;
    const uint indices[] = {
        indexBuffers[geoIndex].indices[baseIndex],
        indexBuffers[geoIndex].indices[baseIndex + 1],
        indexBuffers[geoIndex].indices[baseIndex + 2],
    };

    // Calculate an interpolated surface normal
    const vec3 n1 = GET_VERTEX_NORMAL(vertexBuffers[geoIndex].vertices[indices[0]]);
    const vec3 n2 = GET_VERTEX_NORMAL(vertexBuffers[geoIndex].vertices[indices[1]]);
    const vec3 n3 = GET_VERTEX_NORMAL(vertexBuffers[geoIndex].vertices[indices[2]]);
    vec3 N = normalize(n1 * baryCoords.x + n2 * baryCoords.y + n3 * baryCoords.z);
    N = normalize((transpose(inverse(model)) * vec4(N, 0.0)).xyz);

    return N;
}

vec3 calcHitTangent(vec3 baryCoords, uint geoIndex, uint primitiveIndex)
{
    const mat4 model = mat4(vec4(gl_ObjectToWorldEXT[0], 0),
                            vec4(gl_ObjectToWorldEXT[1], 0),
                            vec4(gl_ObjectToWorldEXT[2], 0),
                            vec4(gl_ObjectToWorldEXT[3], 1));

    // Query vertex indices
    const uint baseIndex = primitiveIndex * 3;
    const uint indices[] = {
        indexBuffers[geoIndex].indices[baseIndex],
        indexBuffers[geoIndex].indices[baseIndex + 1],
        indexBuffers[geoIndex].indices[baseIndex + 2],
    };

    vec3 t1 = GET_VERTEX_TANGENT(vertexBuffers[geoIndex].vertices[indices[0]]);
    vec3 t2 = GET_VERTEX_TANGENT(vertexBuffers[geoIndex].vertices[indices[1]]);
    vec3 t3 = GET_VERTEX_TANGENT(vertexBuffers[geoIndex].vertices[indices[2]]);
    vec3 T = normalize(t1 * baryCoords.x + t2 * baryCoords.y + t3 * baryCoords.z);
    T = normalize((transpose(inverse(model)) * vec4(T, 0.0)).xyz);

    return T;
}
