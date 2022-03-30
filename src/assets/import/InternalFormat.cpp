#include "assets/import/InternalFormat.h"

#include <fstream>

#include "assets/import/PNGConvert.h"



namespace trc
{

FileInputError::FileInputError(const fs::path& path)
    : Exception("Unable to read from file " + path.string())
{
}

FileOutputError::FileOutputError(const fs::path& path)
    : Exception("Unable to write to file " + path.string())
{
}



auto convert(vec4 vec) -> serial::vec4
{
    serial::vec4 res;
    res.set_x(vec.x);
    res.set_y(vec.y);
    res.set_z(vec.z);
    res.set_w(vec.w);

    return res;
}

auto convert(serial::vec4 vec) -> vec4
{
    return { vec.x(), vec.y(), vec.z(), vec.w() };
}

auto convert(vec3 vec) -> serial::vec3
{
    serial::vec3 res;
    res.set_x(vec.x);
    res.set_y(vec.y);
    res.set_z(vec.z);

    return res;
}

auto convert(serial::vec3 vec) -> vec3
{
    return { vec.x(), vec.y(), vec.z() };
}

void readMatrix(const google::protobuf::RepeatedField<float>& buf, mat4* mat)
{
    assert(buf.size() == 16);
    assert(buf.size() * sizeof(float) == sizeof(mat4));

    memcpy(mat, buf.data(), sizeof(mat4));
}

void writeMatrix(mat4 mat, google::protobuf::RepeatedField<float>* out)
{
    out->Resize(16, 0.0f);
    assert(out->size() * sizeof(float) == sizeof(mat4));

    memcpy(out->mutable_data(), &mat, sizeof(mat4));
}



template<typename T>
void assignRef(serial::AssetReference* dst, const AssetReference<T>& src)
{
    assert(src.hasAssetPath());
    *dst->mutable_unique_path() = src.getAssetPath().getUniquePath();
}

template<typename T>
auto toRef(const serial::AssetReference& src)
{
    return AssetReference<T>(AssetPath(src.unique_path()));
}



auto serializeAssetData(const GeometryData& data) -> trc::serial::Geometry
{
    assert(data.skeletalVertices.empty()
           || data.skeletalVertices.size() == data.vertices.size());

    trc::serial::Geometry geo;
    for (uint32_t idx : data.indices)
    {
        geo.add_indices(idx);
    }
    for (const trc::MeshVertex& v : data.vertices)
    {
        trc::serial::Geometry::Vertex* newVert = geo.add_vertices();
        newVert->mutable_position()->set_x(v.position.x);
        newVert->mutable_position()->set_y(v.position.y);
        newVert->mutable_position()->set_z(v.position.z);
        newVert->mutable_normal()->set_x(v.normal.x);
        newVert->mutable_normal()->set_y(v.normal.y);
        newVert->mutable_normal()->set_z(v.normal.z);
        newVert->mutable_tangent()->set_x(v.tangent.x);
        newVert->mutable_tangent()->set_y(v.tangent.y);
        newVert->mutable_tangent()->set_z(v.tangent.z);
        newVert->mutable_uv()->set_x(v.uv.x);
        newVert->mutable_uv()->set_y(v.uv.y);
    }
    for (const trc::SkeletalVertex& v : data.skeletalVertices)
    {
        trc::serial::Geometry::SkelVertex* newVert = geo.add_skeletal_vertices();
        newVert->mutable_bone_indices()->set_x(v.boneIndices.x);
        newVert->mutable_bone_indices()->set_y(v.boneIndices.y);
        newVert->mutable_bone_indices()->set_z(v.boneIndices.z);
        newVert->mutable_bone_indices()->set_w(v.boneIndices.w);
        newVert->mutable_bone_weights()->set_x(v.boneWeights.x);
        newVert->mutable_bone_weights()->set_y(v.boneWeights.y);
        newVert->mutable_bone_weights()->set_z(v.boneWeights.z);
        newVert->mutable_bone_weights()->set_w(v.boneWeights.w);
    }

    if (data.rig.hasAssetPath()) {
        assignRef(geo.mutable_rig(), data.rig);
    }

    return geo;
}

auto deserializeAssetData(const trc::serial::Geometry& geo) -> GeometryData
{
    GeometryData data;

    // Get vertices
    data.vertices.reserve(geo.vertices().size());
    for (const auto& in : geo.vertices())
    {
        auto& v = data.vertices.emplace_back();
        v.position    = { in.position().x(), in.position().y(), in.position().z() };
        v.normal      = { in.normal().x(), in.normal().y(), in.normal().z() };
        v.tangent     = { in.tangent().x(), in.tangent().y(), in.tangent().z() };
        v.uv          = { in.uv().x(), in.uv().y() };
    }

    // Get skeletal vertices
    data.skeletalVertices.reserve(geo.skeletal_vertices_size());
    for (const auto& in : geo.skeletal_vertices())
    {
        auto& v = data.skeletalVertices.emplace_back();
        v.boneIndices = { in.bone_indices().x(), in.bone_indices().y(),
                          in.bone_indices().z(), in.bone_indices().w() };
        v.boneWeights = { in.bone_weights().x(), in.bone_weights().y(),
                          in.bone_weights().z(), in.bone_weights().w() };
    }

    // Get indices
    data.indices.reserve(geo.indices().size());
    data.indices.assign(geo.indices().begin(), geo.indices().end());

    // Get rig reference
    if (geo.has_rig()) {
        data.rig = toRef<Rig>(geo.rig());
    }

    assert(data.skeletalVertices.empty()
           || data.skeletalVertices.size() == data.vertices.size());

    return data;
}

auto serializeAssetData(const TextureData& data) -> trc::serial::Texture
{
    trc::serial::Texture tex;

    auto image = tex.mutable_image();
    image->set_width(data.size.x);
    image->set_height(data.size.y);
    const auto png = toPNG(data);
    image->set_pixel_data_png(png.data(), png.size());

    return tex;
}

auto deserializeAssetData(const trc::serial::Texture& tex) -> TextureData
{
    TextureData data;

    const std::string& png = tex.image().pixel_data_png();
    data = fromPNG(png.data(), png.size());
    assert(data.size.x == tex.image().width() && data.size.y == tex.image().height());

    data.size = { tex.image().width(), tex.image().height() };

    return data;
}

auto serializeAssetData(const MaterialData& data) -> trc::serial::Material
{
    trc::serial::Material mat;

    *mat.mutable_color() = convert(data.color);

    *mat.mutable_ambient_koefficient() = convert(data.ambientKoefficient);
    *mat.mutable_diffuse_koefficient() = convert(data.diffuseKoefficient);
    *mat.mutable_specular_koefficient() = convert(data.specularKoefficient);

    mat.set_shininess(data.shininess);
    mat.set_opacity(data.opacity);
    mat.set_reflectivity(data.reflectivity);

    mat.set_do_perform_lighting(data.doPerformLighting);

    if (data.albedoTexture.hasAssetPath()) {
        assignRef(mat.mutable_albedo_texture(), data.albedoTexture);
    }
    if (data.normalTexture.hasAssetPath()) {
        assignRef(mat.mutable_normal_texture(), data.normalTexture);
    }

    return mat;
}

auto deserializeAssetData(const trc::serial::Material& mat) -> MaterialData
{
    MaterialData data{
        .color = convert(mat.color()),
        .ambientKoefficient = convert(mat.ambient_koefficient()),
        .diffuseKoefficient = convert(mat.diffuse_koefficient()),
        .specularKoefficient = convert(mat.specular_koefficient()),
        .shininess = mat.shininess(),
        .opacity = mat.opacity(),
        .reflectivity = mat.reflectivity(),
        .doPerformLighting = mat.do_perform_lighting(),

        .albedoTexture = {},
        .normalTexture = {},
    };

    if (mat.has_albedo_texture()) {
        data.albedoTexture = toRef<Texture>(mat.albedo_texture());
    }
    if (mat.has_normal_texture()) {
        data.normalTexture = toRef<Texture>(mat.normal_texture());
    }

    return data;
}

auto serializeAssetData(const RigData& data)  -> serial::Rig
{
    serial::Rig out;

    for (const RigData::Bone& bone : data.bones)
    {
        auto& newBone = *out.add_bones();
        newBone.set_name(bone.name);
        writeMatrix(bone.inverseBindPoseMat, newBone.mutable_inv_bind_pose_matrix());
    }

    for (const auto& ref : data.animations)
    {
        if (ref.hasAssetPath()) {
            assignRef(out.add_animations(), ref);
        }
        else {
            std::cout << "Warning: During rig asset serialization: RigData contains reference to"
                " an asset without an asset path - the reference will not be included in the"
                " serialized output!\n";
        }
    }

    return out;
}

auto deserializeAssetData(const serial::Rig& rig) -> RigData
{
    RigData out;

    for (const auto& bone : rig.bones())
    {
        auto& newBone = out.bones.emplace_back();
        newBone.name = bone.name();
        readMatrix(bone.inv_bind_pose_matrix(), &newBone.inverseBindPoseMat);
    }

    for (const auto& ref : rig.animations())
    {
        out.animations.emplace_back(toRef<Animation>(ref));
    }

    return out;
}

auto serializeAssetData(const AnimationData& anim)  -> serial::Animation
{
    assert(anim.frameCount == anim.keyframes.size());

    serial::Animation out;

    out.set_frame_count(anim.frameCount);
    out.set_duration(anim.durationMs);
    out.set_time_per_frame(anim.frameTimeMs);

    for (const auto& kf : anim.keyframes)
    {
        auto dst = out.add_keyframes()->mutable_bone_transform_matrices();
        dst->Resize(kf.boneMatrices.size() * 16, 0.0f);

        const size_t copySize = sizeof(mat4) * kf.boneMatrices.size();
        assert(dst->size() * sizeof(float) == copySize);
        memcpy(dst->mutable_data(), kf.boneMatrices.data(), copySize);
    }

    return out;
}

auto deserializeAssetData(const serial::Animation& tex) -> AnimationData
{
    assert(tex.keyframes_size() == static_cast<i32>(tex.frame_count()));

    AnimationData out;

    out.frameCount = tex.frame_count();
    out.durationMs = tex.duration();
    out.frameTimeMs = tex.time_per_frame();

    for (const auto& kf : tex.keyframes())
    {
        assert((kf.bone_transform_matrices_size() % 16) == 0);

        const size_t boneCount = kf.bone_transform_matrices_size() / 16;
        auto& newKf = out.keyframes.emplace_back();
        newKf.boneMatrices.resize(boneCount);

        memcpy(newKf.boneMatrices.data(),
               kf.bone_transform_matrices().data(),
               boneCount * sizeof(mat4));
    }

    return out;
}



/**
 * Internal utility to write arbitrary Message types
 */
template<std::derived_from<google::protobuf::Message> T>
void writeToFile(const fs::path& filePath, const T& msg)
{
    std::ofstream out(filePath, std::ios::binary | std::ios::trunc);
    if (!out.is_open())
    {
        throw FileOutputError(filePath);
    }

    [[maybe_unused]]
    const bool success = msg.SerializeToOstream(&out);
    assert(success && "The engine should only write valid data to file.");
}

void writeAssetToFile(const fs::path& path, const serial::Asset& asset)
{
    writeToFile<serial::Asset>(path, asset);
}

/**
 * Internal utility to load arbitrary Message types
 */
template<std::derived_from<google::protobuf::Message> T>
auto loadFromFile(const fs::path& filePath) -> T
{
    std::ifstream in(filePath, std::ios::binary);
    if (!in.is_open())
    {
        throw FileInputError(filePath);
    }

    T result;

    [[maybe_unused]]
    const bool success = result.ParseFromIstream(&in);
    assert(success && "The engine should only read valid data from file.");

    return result;
}

auto loadAssetFromFile(const fs::path& path) -> serial::Asset
{
    return loadFromFile<trc::serial::Asset>(path);
}

} // namespace trc
