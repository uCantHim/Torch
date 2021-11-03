// Vertex data in ray tracing descriptors

struct Vertex
{
    float vals[19];
};

#define GET_VERTEX_POS(v) vec3(v.vals[0], v.vals[1], v.vals[2])
#define GET_VERTEX_NORMAL(v) vec3(v.vals[3], v.vals[4], v.vals[5])
#define GET_VERTEX_UV(v) vec2(v.vals[6], v.vals[7])
#define GET_VERTEX_TANGENT(v) vec3(v.vals[8], v.vals[9], v.vals[10])
