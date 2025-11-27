#include "trc/assets/import/GltfImporter.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>
#include <trc_util/Timer.h>

#include "trc/base/Logging.h"

#include "FastgltfImportUtils.cpp"



namespace trc::import
{

namespace math = fastgltf::math;

namespace
{

constexpr
auto toPrimitiveTopology(fastgltf::PrimitiveType fastgltfType) -> vk::PrimitiveTopology
{
    switch (fastgltfType)
    {
    case fastgltf::PrimitiveType::Points:
        return vk::PrimitiveTopology::ePointList;
    case fastgltf::PrimitiveType::Lines:
        return vk::PrimitiveTopology::eLineList;
    case fastgltf::PrimitiveType::LineLoop:
        throw std::logic_error("Not implemented: primitive mode fastgltf::PrimitiveType::LineLoop");
    case fastgltf::PrimitiveType::LineStrip:
        return vk::PrimitiveTopology::eLineStrip;
    case fastgltf::PrimitiveType::Triangles:
        return vk::PrimitiveTopology::eTriangleList;
    case fastgltf::PrimitiveType::TriangleStrip:
        return vk::PrimitiveTopology::eTriangleStrip;
    case fastgltf::PrimitiveType::TriangleFan:
        return vk::PrimitiveTopology::eTriangleFan;
    }
    std::unreachable();
}



struct Loader
{
    auto load(const fs::path& filePath, bool binary) -> std::optional<ImportError>
    {
        fastgltf::GltfFileStream file{ filePath };
        auto asset = [&]{
            const auto options = fastgltf::Options::DontRequireValidAssetMember
                               | fastgltf::Options::AllowDouble
                               //| fastgltf::Options::LoadExternalBuffers
                               | fastgltf::Options::GenerateMeshIndices;
            fastgltf::Parser parser;
            if (binary) {
                return parser.loadGltfBinary(file, filePath.parent_path(), options);
            }
            else {
                return parser.loadGltfJson(file, filePath.parent_path(), options);
            }
        }();

        if (asset.error() != fastgltf::Error::None)
        {
            log::error << "[GltfImporter] Error(s) during file import.";
            return ImportError{
                filePath,
                ImportError::Code::eSyntaxError,
                "[GltfImporter] Error(s) during file import."
            };
        }
        model = std::move(asset.get());

        // Pre-process
        computeGlobalNodeTransforms();

        return std::nullopt;
    }

    struct MeshSkinAssociationInfo
    {
        // Maps skins to meshes that are affected by that skin.
        std::unordered_map<MeshID, std::vector<SkinID>> skinsByMesh;

        // The node to which the mesh and the skin indices are attached.
        std::unordered_map<SkinID, NodeID> parentNode;
    };

    struct SkinInfo
    {
        MeshID geoIndex;

        // Records which node in the GLTF scene tree corresponds to which bone
        // in the skin's bone list.
        std::unordered_map<NodeID, size_t> jointNodeToJoint;
    };

    struct AnimationImportInfo
    {
        std::vector<SkinInfo> skins;
        std::vector<Skeleton> skeletons;

        // Records which joints are part of which rig
        std::unordered_map<NodeID, SkinID> jointNodeToRig;
    };

    struct AnimationImportResult
    {
        std::vector<AnimationImport> animations;

        // Maps animations to skins whose bones are affected by the animation.
        //
        // This association is found by mapping glTF scene nodes that are
        // modified by animation channels to their joint indices (if they do
        // represent joints) and the corresponding rig to which the joint
        // belongs.
        std::unordered_map<AnimID, std::vector<SkinID>> skinsByAnimation;
    };

    void computeGlobalNodeTransforms();
    auto findMeshSkinAssociations() const -> MeshSkinAssociationInfo;

    auto loadAll() const -> ThirdPartyImport;

    auto loadMeshes() const -> std::vector<GeometryImport>;
    auto loadSkins() const -> std::vector<RigImport>;
    auto loadAnimations(const AnimationImportInfo& info) const -> AnimationImportResult;

    auto loadGeometry(const fastgltf::Mesh& mesh) const -> GeometryData;
    auto loadAnimation(const fastgltf::Animation& skin, const AnimationImportInfo& info) const
        -> std::pair<AnimationData, std::vector<SkinID>>;

    fastgltf::Asset model;

    std::unordered_map<const fastgltf::Node*, mat4> globalNodeTransforms;
};

}

auto GltfImporter::loadFromAsciiFile(const fs::path& filePath)
    -> std::expected<ThirdPartyImport, ImportError>
{
    return load(filePath, false);
}

auto GltfImporter::loadFromBinaryFile(const fs::path& filePath)
    -> std::expected<ThirdPartyImport, ImportError>
{
    return load(filePath, true);
}

auto GltfImporter::load(const fs::path& filePath, bool binary)
    -> std::expected<ThirdPartyImport, ImportError>
{
    Timer timer;

    // Load the file
    Loader loader;
    if (auto err = loader.load(filePath, binary)) {
        return std::unexpected(*err);
    }

    // Import data
    auto res = loader.loadAll();

    // Post process
    std::unordered_set<std::string> names;
    for (ui32 i = 0; auto& mesh : res.geometries)
    {
        if (!names.emplace(mesh.name).second) {
            mesh.name = mesh.name + "_" + std::to_string(i++);
        }
    }

    // Log results
    const auto time = timer.reset();
    log::info << "[GltfImporter] Imported data from " << filePath << ": "
              << res.geometries.size() << " mesh(es)"
              << " (" << time << "ms)";

    return res;
}



////////////////////////////////////////////////////////////////////////////////
///  The loader implementation

void Loader::computeGlobalNodeTransforms()
{
    if (model.scenes.size() > 1)
    {
        log::warn << "[GltfImporter] GLTF file contains " << model.scenes.size() << " scenes."
                  << " Only the first scene (\"" << model.scenes.front().name << "\""
                  << " will be processed.";
    }

    for (const auto& [idx, scene] : std::views::enumerate(model.scenes))
    {
        fastgltf::iterateSceneNodes(
            model, idx, math::fmat4x4{},
            [&](fastgltf::Node& node, math::fmat4x4 mat)
            {
                this->globalNodeTransforms[&node] = toMat4(mat);
            }
        );

        // TODO: Handle multiple scenes
        break;
    }
}

auto Loader::findMeshSkinAssociations() const -> MeshSkinAssociationInfo
{
    MeshSkinAssociationInfo res;
    for (const auto& [nodeIdx, node] : std::views::enumerate(model.nodes))
    {
        if (node.meshIndex && node.skinIndex)
        {
            auto [it, _] = res.skinsByMesh.try_emplace(MeshID{ *node.meshIndex });
            it->second.emplace_back(SkinID{ *node.skinIndex });

            res.parentNode.try_emplace(SkinID{ *node.skinIndex }, nodeIdx);
        }
    }

    // Check for multi-skin association (which I don't know how to deal with, or
    // even if it can occur at all) and issue a warning.
    for (const auto& [mesh, skins] : res.skinsByMesh)
    {
        if (skins.size() > 1)
        {
            auto entry = log::warn.startEntry();
            entry << "[GltfImporter] Mesh \"" << model.meshes[mesh].name << "\""
                  << " is associated with more than one skin: [";
            for (auto skin : skins) {
                entry << "\"" << model.skins[skin].name << "\", ";
            }
            entry << "]. This will probably break something as I don't know what it means.";
        }
    }

    return res;
}

auto Loader::loadAll() const -> ThirdPartyImport
{
    auto meshes = loadMeshes();
    auto skins = loadSkins();
    auto meshToSkinInfo = findMeshSkinAssociations();

    // Collect animation import helper data
    AnimationImportInfo animHelperInfo;
    animHelperInfo.skins.resize(skins.size());
    for (const auto& [meshIdx, skinIdxs] : meshToSkinInfo.skinsByMesh)
    {
        const SkinID skinIdx = skinIdxs.front();  // TODO
        const auto& skin = model.skins.at(skinIdx);

        // Populate helper info to associate animations with rigs later on
        auto& rigInfo = animHelperInfo.skins[skinIdx];
        rigInfo.geoIndex = meshIdx;
        for (const auto& [jointIdx, jointNodeIdx] : std::views::enumerate(skin.joints))
        {
            rigInfo.jointNodeToJoint.try_emplace(NodeID{jointNodeIdx}, jointIdx);
            animHelperInfo.jointNodeToRig.try_emplace(NodeID{jointNodeIdx}, skinIdx);
        }
    }

    auto anims = loadAnimations(animHelperInfo);

    // TODO: I may not want to bake rig bind poses into the animation.
    for (auto [animIdx, anim] : std::views::enumerate(anims.animations))
    {
        const SkinID skinIdx = anims.skinsByAnimation.at(AnimID{animIdx}).front();
        const NodeID parentNodeIdx = meshToSkinInfo.parentNode.at(skinIdx);
        const mat4 globalTransform = globalNodeTransforms.at(&model.nodes[parentNodeIdx]);

        bakeSkinBindPose(skins[skinIdx].data, anim.data, globalTransform);
    }

    ThirdPartyImport::Associations refs;

    // Associate meshes with rigs
    for (const auto& [meshIdx, skinIdxs] : meshToSkinInfo.skinsByMesh)
    {
        const SkinID skinIdx = skinIdxs.front();
        log::debug << "[GltfImporter] Associating skin \"" << skins[skinIdx].name
                   << "\" with mesh \"" << meshes[meshIdx].name << "\"";

        refs.geoToRig.try_emplace(meshIdx, skinIdx);
    }

    // Associate animations with rigs
    for (const auto& [animIdx, anim] : std::views::enumerate(anims.animations))
    {
        const auto& skins = anims.skinsByAnimation.at(AnimID{ animIdx });
        for (const SkinID skinIdx : skins) {
            refs.rigToAnimations[skinIdx].emplace_back(animIdx);
        }
    }

    // Create result
    ThirdPartyImport res{
        .filePath{},
        .geometries = std::move(meshes),
        .rigs       = std::move(skins),
        .animations = std::move(anims.animations),
        .materials{},
        .textures{},
        .refs=std::move(refs),
    };

    // Log result
    log::info << "[GltfImporter] Imported assets from glTF file:";
    log::info << "    " << res.geometries.size() << " geometries";
    log::info << "    " << res.rigs.size() << " rigs";
    log::info << "    " << res.animations.size() << " animations";

    // DEBUG: Testing bind poses and animations
#define bakeBindPose false
#define bakeAnimation false

#if (bakeBindPose)
    for (const auto& [meshIdx, skinIdxs] : meshToSkinInfo.skinsByMesh)
    {
        const SkinID skinIdx = skinIdxs.front();

        auto& geo = res.geometries[meshIdx].data;
        const auto& skin = res.rigs[skinIdx].data;
        for (auto [vert, skelVert] : std::views::zip(geo.vertices, geo.skeletalVertices))
        {
            const auto boneIdxs = skelVert.boneIndices;
            const auto weights = skelVert.boneWeights;

            const auto boneMat = weights.x * skin.jointTransform[boneIdxs.x] * skin.inverseBindPoseMat[boneIdxs.x]
                               + weights.y * skin.jointTransform[boneIdxs.y] * skin.inverseBindPoseMat[boneIdxs.y]
                               + weights.z * skin.jointTransform[boneIdxs.z] * skin.inverseBindPoseMat[boneIdxs.z]
                               + weights.w * skin.jointTransform[boneIdxs.w] * skin.inverseBindPoseMat[boneIdxs.w]
                               ;
            vert.position = boneMat * vec4(vert.position, 1.0f);
            vert.normal = boneMat * vec4(vert.normal, 0.0f);
        }
    }
#endif

#if (bakeAnimation)
    for (auto [_meshIdx, mesh] : std::views::enumerate(res.geometries))
    {
        const MeshID meshIdx{ _meshIdx };
        if (!res.refs.geoToRig.contains(meshIdx)) continue;
        const SkinID skinIdx = res.refs.geoToRig.at(meshIdx);
        if (!res.refs.rigToAnimations.contains(skinIdx)) continue;

        const auto& animIds = res.refs.rigToAnimations.at(skinIdx);
        const auto& anim = res.animations[animIds.front()].data;
        const auto& kf = anim.keyframes.at(anim.keyframes.size() / 2);
        log::debug << "[GltfImporter] Baking animation \"" << anim.name
                   << "\" at keyframe #" << anim.keyframes.size() / 2
                   << " into geometry \"" << mesh.name << "\".";

        auto& skel = res.rigs[skinIdx].data;
        auto& geo = res.geometries[meshIdx].data;

        // Adjust skeleton local transforms
        for (auto [jointIdx, localTransform] : skel.localJointTransform | std::views::enumerate) {
            localTransform = kf.boneMatrices[jointIdx];
        }

        // Apply animation to vertex positions
        for (auto [vert, skelVert] : std::views::zip(geo.vertices, geo.skeletalVertices))
        {
            const auto boneIdxs = skelVert.boneIndices;
            const auto weights = skelVert.boneWeights;

            mat4 boneMat{ 1.0f };
            for (size_t i = 0; i < 4; ++i)
            {
                const auto boneIdx = boneIdxs[i];
                const mat4 jointMat = skel.calcJointTransform(boneIdx);
                boneMat += weights[i] * jointMat * skel.inverseBindPoseMat[boneIdx];
            }

            vert.position = boneMat * vec4(vert.position, 1.0f);
            vert.normal = boneMat * vec4(vert.normal, 0.0f);
        }
    }
#endif

    return res;
}

auto Loader::loadMeshes() const -> std::vector<GeometryImport>
{
    std::vector<GeometryImport> meshes;
    for (const auto& mesh : model.meshes)
    {
        meshes.emplace_back(GeometryImport{
            .name{ mesh.name },
            .data=loadGeometry(mesh)
        });
    }

    return meshes;
}

auto Loader::loadSkins() const -> std::vector<RigImport>
{
    std::vector<RigImport> skels;
    for (const auto& skin : model.skins) {
        skels.push_back({ .name{skin.name}, .data=loadSkeleton(model, skin) });
    }

    return skels;
}

auto Loader::loadGeometry(const fastgltf::Mesh& mesh) const -> GeometryData
{
    GeometryData geo;

    // Load primitives
    for (const auto& prim : mesh.primitives)
    {
        // Load geometry info
        geo.primitiveTopology = toPrimitiveTopology(prim.type);

        // Load indices
        if (prim.indicesAccessor)
        {
            const auto& indices = model.accessors[*prim.indicesAccessor];
            geo.indices.resize(indices.count);
            fastgltf::iterateAccessorWithIndex<uint32_t>(model, indices, [&](ui32 index, ui32 i) {
                geo.indices[i] = index;
            });
        }

        // Positions are mandated by GLTF, so we resize the geo.vertices array in here.
        if (auto positions = prim.findAttribute("POSITION"); positions != prim.attributes.end())
        {
            const auto& accessor = model.accessors[positions->accessorIndex];
            geo.vertices.resize(accessor.count);
            fastgltf::iterateAccessorWithIndex<math::fvec3>(
                model, accessor, [&](math::fvec3 pos, ui32 idx) {
                    geo.vertices[idx].position = { pos.x(), pos.y(), pos.z() };
                }
            );
        }
        else {
            log::warn << "[GltfImporter] Standard vertex attribute POSITION not found in mesh.";
        }
        if (auto normals = prim.findAttribute("NORMAL"); normals != prim.attributes.end())
        {
            const auto& accessor = model.accessors[normals->accessorIndex];
            fastgltf::iterateAccessorWithIndex<math::fvec3>(
                model, accessor, [&](math::fvec3 normal, ui32 idx) {
                    geo.vertices[idx].normal = { normal.x(), normal.y(), normal.z() };
                }
            );
        }
        else {
            log::warn << "[GltfImporter] Standard vertex attribute NORMAL not found in mesh.";
        }
        if (auto texcoords = prim.findAttribute("TEXCOORD_0"); texcoords != prim.attributes.end())
        {
            const auto& accessor = model.accessors[texcoords->accessorIndex];
            fastgltf::iterateAccessorWithIndex<math::fvec2>(
                model, accessor, [&](math::fvec2 uv, ui32 idx) {
                    geo.vertices[idx].uv = { uv.x(), uv.y() };
                }
            );
        }
        else {
            log::warn << "[GltfImporter] Standard vertex attribute TEXCOORD_0 not found in mesh.";
        }
        if (auto tangents = prim.findAttribute("TANGENT"); tangents != prim.attributes.end())
        {
            const auto& accessor = model.accessors[tangents->accessorIndex];
            fastgltf::iterateAccessorWithIndex<math::fvec3>(
                model, accessor, [&](math::fvec3 tangent, ui32 idx) {
                    geo.vertices[idx].tangent = { tangent.x(), tangent.y(), tangent.z() };
                }
            );
        }
        if (auto joints = prim.findAttribute("JOINTS_0"); joints != prim.attributes.end())
        {
            const auto& accessor = model.accessors[joints->accessorIndex];
            geo.skeletalVertices.resize(accessor.count);
            fastgltf::iterateAccessorWithIndex<math::u32vec4>(
                model, accessor, [&](math::u32vec4 i, ui32 idx) {
                    geo.skeletalVertices[idx].boneIndices = { i.x(), i.y(), i.z(), i.w() };
                }
            );
        }
        if (auto weights = prim.findAttribute("WEIGHTS_0"); weights != prim.attributes.end())
        {
            const auto& accessor = model.accessors[weights->accessorIndex];
            geo.skeletalVertices.resize(accessor.count);
            fastgltf::iterateAccessorWithIndex<math::fvec4>(
                model, accessor, [&](math::fvec4 w, ui32 idx) {
                    geo.skeletalVertices[idx].boneWeights = { w.x(), w.y(), w.z(), w.w() };
                }
            );
        }

        // TODO: Figure out what to do with multiple primitives
        break;
    }

    if (mesh.primitives.size() > 1)
    {
        log::debug << "[GltfImporter] Mesh " << mesh.name << " has more than one primitive"
                   << " description (total: " << mesh.primitives.size() << " primitives)."
                   << " I don't know what to do with that, so they will be ignored.";
        for (const auto& [i, prim] : std::views::enumerate(mesh.primitives))
        {
            log::debug << "  Primitive #" << i << ": Mode " << vk::to_string(toPrimitiveTopology(prim.type));
            for (const auto& [name, idx] : prim.attributes)
            {
                auto& accessor = model.accessors[idx];
                log::debug << "   - Attribute " << name << " (count: " << accessor.count << ")";
            }
        }
    }

    return geo;
}

auto Loader::loadAnimations(const AnimationImportInfo& info) const -> AnimationImportResult
{
    AnimationImportResult res;
    for (const auto& [idx, anim] : std::views::enumerate(model.animations))
    {
        auto [data, affectedSkins] = loadAnimation(anim, info);
        res.animations.emplace_back(std::string{anim.name}, std::move(data));
        res.skinsByAnimation.try_emplace(AnimID{ idx }, std::move(affectedSkins));
    }

    return res;
}

/**
 * The animation returned contains keyframes of *local transformations of joints*!
 * You may want to use `bakeSkinBindPose` to make joint transforms global with
 * respect to the skin root and bake a skin's bind pose matrices into them.
 *
 * @return The animation and a list of rigs to which it applies
 */
auto Loader::loadAnimation(const fastgltf::Animation& anim, const AnimationImportInfo& info) const
    -> std::pair<AnimationData, std::vector<SkinID>>
{
    if (anim.channels.empty() || anim.samplers.empty()) {
        return {};
    }

    auto nodeToPath = collectsPaths(model, anim);
    auto meta = expandChannels(nodeToPath);

    AnimationData res{
        .name{ anim.name },
        .frameCount = meta.numKeyframes,
        .durationMs = meta.duration * 1000,
        .frameTimeMs = (meta.duration / meta.numKeyframes) * 1000,
        .keyframes{}
    };
    std::unordered_set<SkinID> transformedRigs;

    // Create keyframe matrices
    for (const auto& [nodeIdx, path] : nodeToPath)
    {
        if (!info.jointNodeToRig.contains(nodeIdx))
        {
            // Animated node has no rig associated with it. Ignore it.
            continue;
        }

        const auto rigIdx = info.jointNodeToRig.at(NodeID{nodeIdx});
        transformedRigs.emplace(rigIdx);

        const auto& rigInfo = info.skins[rigIdx];
        const size_t jointIdx = rigInfo.jointNodeToJoint.at(nodeIdx);

        res.keyframes.resize(glm::max(res.keyframes.size(), path.size()));
        for (size_t i = 0; i < path.size(); ++i)
        {
            auto& jointMats = res.keyframes[i].boneMatrices;
            jointMats.resize(glm::max(jointMats.size(), rigInfo.jointNodeToJoint.size()));

            jointMats[jointIdx] = path.getKeyframeMatrix(i);
        }
    }

    return { res, { transformedRigs.begin(), transformedRigs.end() } };
}

} // namespace trc::import
