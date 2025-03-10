#ifdef TRC_USE_FBX_SDK

#include "trc/assets/import/FBXImporter.h"

#include <chrono>
#include <sstream>
using namespace std::chrono;

#include "trc/assets/import/GeometryTransformations.h"
#include "trc/base/Logging.h"



class FbxLogger
{
public:
    void startLog() {
        ss.clear();
    }
    void endLog() {
        trc::log::info << "FBX import information:\n" << ss.rdbuf();
    }

    template<typename T>
    inline auto operator<<(T&& t) -> FbxLogger&
    {
        ss << std::forward<T>(t);
        return *this;
    }

    std::stringstream ss;
};

namespace trc
{
    mat4 fbxToGlm(const FbxAMatrix& fm)
    {
        mat4 mat;
        mat[0][0] = static_cast<float>(fm[0][0]);
        mat[0][1] = static_cast<float>(fm[0][1]);
        mat[0][2] = static_cast<float>(fm[0][2]);
        mat[0][3] = static_cast<float>(fm[0][3]);
        mat[1][0] = static_cast<float>(fm[1][0]);
        mat[1][1] = static_cast<float>(fm[1][1]);
        mat[1][2] = static_cast<float>(fm[1][2]);
        mat[1][3] = static_cast<float>(fm[1][3]);
        mat[2][0] = static_cast<float>(fm[2][0]);
        mat[2][1] = static_cast<float>(fm[2][1]);
        mat[2][2] = static_cast<float>(fm[2][2]);
        mat[2][3] = static_cast<float>(fm[2][3]);
        mat[3][0] = static_cast<float>(fm[3][0]);
        mat[3][1] = static_cast<float>(fm[3][1]);
        mat[3][2] = static_cast<float>(fm[3][2]);
        mat[3][3] = static_cast<float>(fm[3][3]);
        return mat;
    }

    FbxLogger fbxLog;
} // namespace trc



auto trc::FBXImporter::load(const fs::path& path) -> ThirdPartyFileImportData
{
    fbxLog.startLog();
    init();

    time_point<system_clock, milliseconds> start = time_point_cast<milliseconds>(system_clock::now());

    ThirdPartyFileImportData result;
    result.filePath = path;

    auto sceneImportOpt = loadSceneFromFile(path);
    if (!sceneImportOpt.has_value())
    {
        fbxLog << "Unable to load scene: " << path << "\n";
        return result;
    }
    auto sceneImport = std::move(sceneImportOpt.value());
    FbxScene* scene = sceneImport.scene;

    for (ui32 meshIndex = 0; auto& [mesh, name, transform] : sceneImport.meshes)
    {
        fbxLog << "Loading mesh " << name << ":\n";

        ThirdPartyMeshImport newMesh{
            .name=name,
            .globalTransform=transform,
            .geometry=loadMesh(mesh),
            .materials=loadMaterials(mesh),
            .rig=std::nullopt,
            .animations={}
        };

        if (sceneImport.skeletonRoots.size() > meshIndex)
        {
            auto skeleton = sceneImport.skeletonRoots[meshIndex];
            fbxLog << "Loading skeleton \"" << skeleton->GetName() << "\" for mesh \""
                << name << "\"..\n";

            auto [rig, boneNodes] = loadRig(mesh, newMesh.geometry);
            newMesh.animations = loadAnimations(scene, rig, boneNodes);
            newMesh.rig = std::move(rig);
        }

        result.meshes.push_back(std::move(newMesh));
        meshIndex++;
    } // Per-mesh end

    auto elapsed_milliseconds = duration_cast<milliseconds>(system_clock::now() - start).count();
    fbxLog << "\nFile loaded in " << elapsed_milliseconds << "ms.\n";
    fbxLog << "++++++++++++++++++++++++++++++++++";

    scene->Destroy();
    fbxLog.endLog();

    return result;
}


void trc::FBXImporter::init()
{
    if (!initialized)
    {
        fbx_memory_manager = FbxManager::Create();
        fbx_io_settings = FbxIOSettings::Create(fbx_memory_manager, IOSROOT);
        fbx_io_settings->SetBoolProp(IMP_FBX_MATERIAL,        true);
        fbx_io_settings->SetBoolProp(IMP_FBX_TEXTURE,         true);
        fbx_io_settings->SetBoolProp(IMP_FBX_LINK,            true);
        fbx_io_settings->SetBoolProp(IMP_FBX_SHAPE,           true);
        fbx_io_settings->SetBoolProp(IMP_FBX_GOBO,            false);
        fbx_io_settings->SetBoolProp(IMP_FBX_ANIMATION,       true);
        fbx_io_settings->SetBoolProp(IMP_FBX_GLOBAL_SETTINGS, true);
        fbx_memory_manager->SetIOSettings(fbx_io_settings);

        fbx_io_settings->SetBoolProp(IMP_ANIMATION, true);

        initialized = true;
    }
}


auto trc::FBXImporter::loadSceneFromFile(const std::string& path) -> std::optional<SceneImport>
{
    if (path.length() < 3
        || !(path[path.length() - 3] == 'f'
        && path[path.length() - 2] == 'b'
        && path[path.length() - 1] == 'x'))
    {
        fbxLog << path << "is not a valid FBX-file.\n";
        return std::nullopt;
    }

    fbxLog << "++++++++++++++++++++++++++++++++++\n";
    fbxLog << "Loading scene from file " << path << "...\n";

    FbxImporter* importer = FbxImporter::Create(fbx_memory_manager, "");

    if (!importer->Initialize(path.c_str(), -1, fbx_memory_manager->GetIOSettings()))
    {
        fbxLog << "Call to FbxImporter::Initialize() failed.\n";
        fbxLog << "Error returned: " << importer->GetStatus().GetErrorString() << "\n\n";
        return std::nullopt;
    }

    FbxScene* scene = FbxScene::Create(fbx_memory_manager, "");
    importer->Import(scene);
    importer->Destroy();

    // Convert axis system because either fbx or blender is retarded
    //FbxAxisSystem newSystem = FbxAxisSystem(FbxAxisSystem::EUpVector::eYAxis, FbxAxisSystem::eParityOdd, FbxAxisSystem::ECoordSystem::eRightHanded);
    //newSystem.ConvertScene(scene);

    if (scene == nullptr)
    {
        fbxLog << "File " << path << "could not be loaded.\n";
        return std::nullopt;
    }

    SceneImport importResult{
        .scene = scene,
        .meshes = {},
        .skeletonRoots = {},
    };
    FbxAnimEvaluator* evaluator = scene->GetAnimationEvaluator();

    for (int i = 0; i < scene->GetNodeCount(); i++)
    {
        FbxNode* node = scene->GetNode(i);
        FbxNodeAttribute* attrib = node->GetNodeAttribute();
        if (attrib == nullptr) {
            continue;
        }

        switch (attrib->GetAttributeType())
        {
        case FbxNodeAttribute::eMesh:
            importResult.meshes.emplace_back(
                static_cast<FbxMesh*>(attrib),
                attrib->GetName(),
                fbxToGlm(evaluator->GetNodeGlobalTransform(node))
            );

            fbxLog << "Mesh found: " << attrib->GetName() << "\n";
            break;
        case FbxNodeAttribute::eSkeleton:
            if ((static_cast<FbxSkeleton*>(attrib))->IsSkeletonRoot())
            {
                importResult.skeletonRoots.push_back(static_cast<FbxSkeleton*>(attrib));
                fbxLog << "Skeleton root found: " << attrib->GetName() << "\n";
            }
            break;
        default:
            break;
        }
    }

    return importResult;
}


auto trc::FBXImporter::loadMesh(FbxMesh* mesh) -> GeometryData
{
    GeometryData result;

    // Load indices
    int indexCount = mesh->GetPolygonVertexCount();
    assert(indexCount >= 0);
    int* indices = mesh->GetPolygonVertices();

    result.indices.reserve(indexCount);
    for (int i = 0; i < indexCount; i++)
    {
        result.indices.push_back(static_cast<ui32>(indices[i]));
    }

    loadVertices(mesh, result);
    loadUVs(mesh, result);
    loadNormals(mesh, result);
    loadTangents(mesh, result);

    fbxLog << "\t" << result.vertices.size() << " vertices have been loaded.\n"
              << "\t" << result.indices.size() << " indices have been loaded.\n";

    return result;
}


void trc::FBXImporter::loadVertices(FbxMesh* mesh, GeometryData& result)
{
    // Load individual vertices
    int vertCount = mesh->GetControlPointsCount();
    FbxVector4* verts = mesh->GetControlPoints();

    result.vertices.resize(vertCount);

    for (int i = 0; i < vertCount; i++)
    {
        result.vertices[i].position = vec3(verts[i].mData[0], verts[i].mData[1], verts[i].mData[2]);
    }
}


void trc::FBXImporter::loadUVs(FbxMesh* mesh, GeometryData& result)
{
    FbxStringList uvSetNameList;
    mesh->GetUVSetNames(uvSetNameList);
    if (uvSetNameList.GetCount() <= 0)
    {
        fbxLog << "\tMesh doesn't have any uv sets!\n";
        return;
    }
    fbxLog << "\tMesh has " << uvSetNameList.GetCount() << " UV sets. Using the first one.\n";

    FbxGeometryElementUV* uvElement = mesh->GetElementUV(uvSetNameList.GetStringAt(0));

    // Mapping mode is by control-point
    if (uvElement->GetMappingMode() == FbxGeometryElement::eByControlPoint)
    {
        fbxLog << "ATTENTION! UV mapping mode is by control-point, this might cause errors\n";

        for (int vertIndex = 0; vertIndex < mesh->GetControlPointsCount(); vertIndex++)
        {
            int uvIndex = 0;

            if (uvElement->GetReferenceMode() == FbxGeometryElement::eDirect) {
                uvIndex = vertIndex;
            }
            if (uvElement->GetReferenceMode() == FbxGeometryElement::eIndexToDirect) {
                uvIndex = uvElement->GetIndexArray().GetAt(vertIndex);
            }

            FbxVector2 uv = uvElement->GetDirectArray().GetAt(uvIndex);
            result.vertices[uvIndex].uv = vec2(uv[0], uv[1]);
        }
    }

    // Mapping mode is by polygon-vertex
    if (uvElement->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
    {
        int indexCount = mesh->GetPolygonVertexCount();

        for (int i = 0; i < indexCount; i++)
        {
            int uvIndex = i;

            FbxVector2 uv = uvElement->GetDirectArray().GetAt(uvElement->GetIndexArray().GetAt(uvIndex));
            result.vertices[result.indices[uvIndex]].uv = vec2(uv[0], uv[1]);
        }
    }
}


void trc::FBXImporter::loadNormals(FbxMesh* mesh, GeometryData& result)
{
    FbxGeometryElementNormal* normalElement = mesh->GetElementNormal();

    // Mapping mode is by control-point (vertex)
    if (normalElement->GetMappingMode() == FbxGeometryElement::eByControlPoint)
    {
        fbxLog << "ATTENTION! Normal mapping mode is by control-point, this might cause errors\n";

        for (int vertIndex = 0; vertIndex < mesh->GetControlPointsCount(); vertIndex++)
        {
            int normalIndex = 0;

            if (normalElement->GetReferenceMode() == FbxGeometryElement::eDirect) {
                normalIndex = vertIndex;
            }
            if (normalElement->GetReferenceMode() == FbxGeometryElement::eIndexToDirect) {
                normalIndex = normalElement->GetIndexArray().GetAt(vertIndex);
            }

            FbxVector4 normal = normalElement->GetDirectArray().GetAt(normalIndex);
            result.vertices[normalIndex].normal = vec3(normal[0], normal[1], normal[2]);
        }
    }

    // Mapping mode is by polygon-vertex
    if (normalElement->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
    {
        // Polygon vertices are logical vertices (as many as there are indices)
        int indexCount = mesh->GetPolygonVertexCount();

        for (int i = 0; i < indexCount; i++)
        {
            FbxVector4 normal = normalElement->GetDirectArray().GetAt(i);
            result.vertices[result.indices[i]].normal = vec3(normal[0], normal[1], normal[2]);
        }
    }
}


void trc::FBXImporter::loadTangents(FbxMesh* mesh, GeometryData& result)
{
    FbxGeometryElementTangent* tangentElement = mesh->GetElementTangent();
    if (tangentElement == nullptr)
    {
        fbxLog << "\tNo precomputed tangents found. Computing tangents...\n\t";
        computeTangents(result);

        return;
    }

    if (tangentElement->GetMappingMode() == FbxGeometryElement::eByControlPoint)
    {
        fbxLog << "ATTENTION! Tangent mapping mode is by control point, this is not implemented!\n";
        assert(false);
    }

    if (tangentElement->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
    {
        int indexCount = mesh->GetPolygonVertexCount();

        for (int i = 0; i < indexCount; i++)
        {
            FbxVector4 tangent = tangentElement->GetDirectArray().GetAt(i);
            result.vertices[result.indices[i]].tangent = vec3(tangent[0], tangent[1], tangent[2]);
        }
    }
}


auto trc::FBXImporter::loadMaterials(FbxMesh* mesh) -> std::vector<ThirdPartyMaterialImport>
{
    FbxNode* meshNode = mesh->GetNode();
    const int materialCount = meshNode->GetMaterialCount();
    fbxLog << "\t" << materialCount << " materials found.\n";

    std::vector<ThirdPartyMaterialImport> result;

    for (int i = 0; i < materialCount; i++)
    {
        FbxSurfaceMaterial* mat = meshNode->GetMaterial(i);
        if (mat == nullptr) {
            continue;
        }

        if (GetImplementation(mat, FBXSDK_IMPLEMENTATION_HLSL) != nullptr
            || GetImplementation(mat, FBXSDK_IMPLEMENTATION_CGFX) != nullptr)
        {
            fbxLog << "Material " << mat->GetName()
                << " is some strange hardware shader, I don't know what to do with that.\n";
            continue;
        }

        // Material is a Phong material
        if (mat->GetClassId().Is(FbxSurfacePhong::ClassId))
        {
            fbxLog << "\tLoading Phong material " << mat->GetName() << ".\n";

            auto phongMat = dynamic_cast<FbxSurfacePhong*>(mat);
            auto& newMat = result.emplace_back();

            FbxDouble3 amb = phongMat->Ambient;
            FbxDouble3 diff = phongMat->Diffuse;
            FbxDouble3 spec = phongMat->Specular;
            newMat.data.specularCoefficient = phongMat->SpecularFactor;
            newMat.data.roughness = static_cast<float>(phongMat->Shininess);
            newMat.data.opacity = static_cast<float>(1.0 - phongMat->TransparencyFactor);
            newMat.name = phongMat->GetName();
        }
        // Material is a Lambert material
        else if (mat->GetClassId().Is(FbxSurfaceLambert::ClassId))
        {
            fbxLog << "\tLoading Lambert material " << mat->GetName()
                << " with standard values for specular color and shinyness.\n";

            auto lambertMat = dynamic_cast<FbxSurfaceLambert*>(mat);
            auto& newMat = result.emplace_back();

            FbxDouble3 amb = lambertMat->Ambient;
            FbxDouble3 diff = lambertMat->Diffuse;
            newMat.data.specularCoefficient = 1.0f;
            newMat.data.roughness = 1.0f; // Standard value
            newMat.data.opacity = static_cast<float>(1.0 - lambertMat->TransparencyFactor);
            newMat.name = lambertMat->GetName();
        }
        else {
            fbxLog << "Material " << mat->GetName() << " is an unknown material type.\n";
            continue;
        }
    }

    return result;
}


auto trc::FBXImporter::loadSkeleton(FbxSkeleton* skeleton)
    -> std::tuple<RigData, std::vector<FbxNode*>, std::unordered_map<std::string, ui32>>
{
    RigData rig;
    std::vector<FbxNode*> boneNodes;
    std::unordered_map<std::string, ui32> boneNamesToIndices;

    std::function<void(FbxNode*)> collectNodeRecursive = [&](FbxNode* node)
    {
        assert(node != nullptr);

        // Ignore 'end' bones created by FBX
        if (node->GetChildCount() == 0) {
            return;
        }

        rig.bones.emplace_back();
        boneNodes.push_back(node);
        boneNamesToIndices[node->GetName()] = rig.bones.size() - 1;

        for (int i = 0; i < node->GetChildCount(); i++)
        {
            collectNodeRecursive(node->GetChild(i));
        }
    };

    collectNodeRecursive(skeleton->GetNode());

    return { rig, boneNodes, boneNamesToIndices };
}


auto trc::FBXImporter::loadRig(FbxMesh* mesh, GeometryData& result)
    -> std::pair<RigData, std::vector<FbxNode*>>
{
    // I don't know what this does but apparently it's important
    FbxVector4 T = mesh->GetNode()->GetGeometricTranslation(FbxNode::eSourcePivot);
    FbxVector4 R = mesh->GetNode()->GetGeometricRotation(FbxNode::eSourcePivot);
    FbxVector4 S = mesh->GetNode()->GetGeometricScaling(FbxNode::eSourcePivot);
    FbxAMatrix geometryTransformMat = FbxAMatrix(T, R, S);

    const int deformerCount = mesh->GetDeformerCount();
    if (deformerCount > 0)
    {
        fbxLog << "\t--- Holy crap, the mesh " << mesh->GetName() << " has more than one "
            << "deformer (skin). I don't know what to do with that, so it'll be ignored.\n";
    }

    // Create skeletal vertices if a deformer (skeleton) is present
    if (deformerCount > 0) {
        result.skeletalVertices.resize(result.vertices.size());
    }

    // Deformers are whole skeletons of a mesh (probably)
    // This loop is exited after the first iteration. It only exists for completeness's sake.
    for (int deformerIndex = 0; deformerIndex < deformerCount; deformerIndex++)
    {
        auto skin = static_cast<FbxSkin*>(mesh->GetDeformer(deformerIndex, FbxDeformer::eSkin));
        if (skin == nullptr) { continue; }

        fbxLog << "\tSkin #" << deformerIndex << " \"" << skin->GetName() << "\"" << ":\n";

        ////////////
        // Build rig
        auto skeletonRoot = skin->GetCluster(0)->GetLink()->GetNodeAttribute();
        /**
         * If this assertion fails, the assumption that the first cluster in a skin is
         * always its root is false.
         */
        assert(skeletonRoot->GetAttributeType() == FbxNodeAttribute::eSkeleton);
        auto [rig, boneNodes, boneNamesToIndices] = loadSkeleton(static_cast<FbxSkeleton*>(skeletonRoot));
        rig.name = skin->GetName();

        ////////////////
        // Get bone data
        const size_t clusterCount = skin->GetClusterCount();

        /** Stores how many bone indices have been set for each vertex */
        std::vector<ui32> weightsFilledHelper(result.indices.size());

        // Clusters hold bones, which are links (probably)
        for (size_t clusterIndex = 0; clusterIndex < clusterCount; clusterIndex++)
        {
            auto cluster = skin->GetCluster(clusterIndex);
            const std::string currBoneName = cluster->GetLink()->GetName();
            const ui32 currBoneIndex = boneNamesToIndices[currBoneName];

            /**
             * TODO: *If* this assertion can never fail, we can remove the
             * boneNamesToIndices map.
             */
            assert(clusterIndex == currBoneIndex);

            // Calculate bind pose inverse matrix
            FbxAMatrix meshTransformMat;
            FbxAMatrix boneBindPoseMat;
            FbxAMatrix bindPoseInvMat;

            cluster->GetTransformMatrix(meshTransformMat);
            cluster->GetTransformLinkMatrix(boneBindPoseMat);
            bindPoseInvMat = boneBindPoseMat.Inverse() * meshTransformMat * geometryTransformMat;

            rig.bones[currBoneIndex].inverseBindPoseMat = fbxToGlm(bindPoseInvMat);

            // Fill vertex weight data
            const int indexCount = cluster->GetControlPointIndicesCount();
            const int* indices = cluster->GetControlPointIndices();
            const double* weights = cluster->GetControlPointWeights();

            for (int i = 0; i < indexCount; i++)
            {
                size_t vertIndex = static_cast<size_t>(indices[i]);
                if (weightsFilledHelper[vertIndex] < MAX_WEIGHTS_PER_VERTEX)
                {
                    auto& vertex = result.skeletalVertices[vertIndex];
                    vertex.boneWeights[weightsFilledHelper[vertIndex]] = float(weights[i]);
                    vertex.boneIndices[weightsFilledHelper[vertIndex]] = float(currBoneIndex);

                    weightsFilledHelper[vertIndex]++;
                }
            }
        } // Per-bone end

        fbxLog << "\tCorrecting bone weights...\n";
        correctBoneWeights(result);

        return { rig, boneNodes };
    } // per-skin

    return {};
}


void trc::FBXImporter::correctBoneWeights(GeometryData& mesh)
{
    ui32 totalWeights = 0; // For logging only
    ui32 correctedWeights = 0; // For logging only
    for (SkeletalVertex& vert : mesh.skeletalVertices)
    {
        // Weight sum correction
        vec4& weight = vert.boneWeights;
        double weightSum = static_cast<double>(weight.x) + static_cast<double>(weight.y)
                           + static_cast<double>(weight.z) + static_cast<double>(weight.w);
        if (weightSum > 1.0001)
        {
            double overflow = weightSum - 1.0;
            // Ratio-sustaining method
            for (int j = 0; j < 4; j++) {
                weight[j] -= static_cast<float>(overflow * (weight[j] / weightSum));
            }
            correctedWeights++;
        }
        if (weightSum < 0.9999)
        {
            double missing = 1.0 - weightSum;
            for (int j = 0; j < 4; j++) {
                weight[j] += static_cast<float>(missing * (weight[j] / weightSum));
            }
            correctedWeights++;
        }
        totalWeights++;
    }

    fbxLog << "\t\t" << correctedWeights << "/" << totalWeights << " weights corrected\n";
}


auto trc::FBXImporter::loadAnimations(
    FbxScene* scene,
    const RigData& rig,
    const std::vector<FbxNode*>& boneNodes)
    -> std::vector<AnimationData>
{
    std::vector<AnimationData> animations;

    int animStackCount = scene->GetSrcObjectCount<FbxAnimStack>();
    for (int animStackIndex = 0; animStackIndex < animStackCount; animStackIndex++)
    {
        FbxAnimStack* animStack = scene->GetSrcObject<FbxAnimStack>(animStackIndex);
        fbxLog << "Animation " << animStackIndex << ": "
            << animStack->GetName() << "\n";

        /**
         * RANT INCOMING
         *
         * what
         * the
         * fresh
         * hell
         * is
         * this
         * api
         * design
         * are
         * you
         * people
         * brain
         * damaged
         */
        scene->SetCurrentAnimationStack(animStack);  // <-- WHAT???

        // Animation time
        const FbxTime::EMode timeMode = FbxTime::EMode::eFrames24;
        const FbxTimeSpan timespan = animStack->GetLocalTimeSpan();
        const size_t totalFrames = timespan.GetDuration().GetFrameCount(timeMode);

        AnimationData& animation = animations.emplace_back();
        animation.name = animStack->GetName();
        animation.frameCount = totalFrames;
        animation.durationMs = static_cast<float>(timespan.GetDuration().GetMilliSeconds());
        animation.frameTimeMs = animation.durationMs / static_cast<float>(animation.frameCount);

        fbxLog << "\tDuration: " << animation.durationMs << " ms ("
            << animation.frameCount << " frames)\n";

        // Create keyframes
        for (size_t frame = 0; frame < totalFrames; frame++)
        {
            auto& keyframe = animation.keyframes.emplace_back();

            FbxTime frameTime;
            frameTime.SetFrame(frame, timeMode);

            // Fill the keyframe's bones
            for (ui32 boneIndex = 0; boneIndex < rig.bones.size(); boneIndex++)
            {
                FbxNode* bone = boneNodes[boneIndex];
                FbxAMatrix boneTransform = bone->EvaluateGlobalTransform(frameTime);

                // Precompute boneMatrix * bindposeInverseMatrix for per-frame bones
                keyframe.boneMatrices.emplace_back(
                    fbxToGlm(boneTransform) * rig.bones[boneIndex].inverseBindPoseMat
                );
            } // per-bone end
        } // per-keyframe end
    } // per-anim stack end

    return animations;
}

#endif // #ifdef TRC_USE_FBX_SDK
