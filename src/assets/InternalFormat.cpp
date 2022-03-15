#include "assets/InternalFormat.h"

#include <fstream>

#include "assets/PNGConvert.h"



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



auto serializeAssetData(const GeometryData& data) -> trc::serial::Geometry
{
    assert(data.skeletalVertices.size() == 0
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

    if (data.rig.has_value()) {
        geo.mutable_rig()->set_unique_path(data.rig.value().getUniquePath());
    }

    return geo;
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
        data.rig = AssetPath(geo.rig().unique_path());
    }

    assert(data.skeletalVertices.size() == 0
           || data.skeletalVertices.size() == data.vertices.size());

    return data;
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
