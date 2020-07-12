#include "utils/FBXLoader.h"

#include <iostream>
#include <chrono>
using namespace std::chrono;



mat4 fbxToGlm(const FbxAMatrix& fm);

trc::FBXLoader::FBXLoader()
{
	if (!initialized)
	{
		fbx_memory_manager = FbxManager::Create();
		fbx_io_settings = FbxIOSettings::Create(fbx_memory_manager, IOSROOT);
		fbx_memory_manager->SetIOSettings(fbx_io_settings);

		fbx_io_settings->SetBoolProp(IMP_ANIMATION, true);

        initialized = true;
	}
}


auto trc::FBXLoader::loadFBXFile(const std::string& path) -> FileImportData
{
	time_point<system_clock, milliseconds> start = time_point_cast<milliseconds>(system_clock::now());

	FileImportData result;

	if (!loadSceneFromFile(path)) {
        std::cout << "Unable to load scene: " << path << "\n";
        return result;
	}

	for (FbxMesh* mesh : meshes)
	{
		std::cout << "Loading mesh " << mesh->GetName() << ":\n";

        Mesh newMesh = {
            mesh->GetName(),
            loadMesh(mesh),
            {},
            {}
        };

		// if (skeletonRoots.size() > meshIndex)
		// {
		// 	result.rig = AnimRigConstrParams();
		// 	loadSkeleton(mesh, &result);
		// }

		result.meshes.push_back(std::move(newMesh));
	} // Per-mesh end

	auto elapsed_milliseconds = duration_cast<milliseconds>(system_clock::now() - start).count();
	std::cout << "\nFile loaded in " << elapsed_milliseconds << "ms.\n";
	std::cout << "++++++++++++++++++++++++++++++++++\n\n";

	scene->Destroy();
	nameToBoneIndex.clear();
	meshes.clear();
	skeletonRoots.clear();
	boneNodes.clear();

	return result;
}


bool trc::FBXLoader::loadSceneFromFile(const std::string& path)
{
	if (path.length() < 3
		|| !(path[path.length() - 3] == 'f'
		&& path[path.length() - 2] == 'b'
		&& path[path.length() - 1] == 'x'))
	{
		std::cout << path << "is not a valid FBX-file.\n";
		return false;
	}

	std::cout << "\n++++++++++++++++++++++++++++++++++\n";
	std::cout << "Loading mesh from file " << path << "...\n";

	FbxImporter* importer = FbxImporter::Create(fbx_memory_manager, "");

	if (!importer->Initialize(path.c_str(), -1, fbx_memory_manager->GetIOSettings()))
	{
		std::cout << "Call to FbxImporter::Initialize() failed.\n";
		std::cout << "Error returned: " << importer->GetStatus().GetErrorString() << "\n\n";
		return false;
	}

	scene = FbxScene::Create(fbx_memory_manager, "");
	importer->Import(scene);
	importer->Destroy();

	// Convert axis system because either fbx or blender is retarded
	//FbxAxisSystem newSystem = FbxAxisSystem(FbxAxisSystem::EUpVector::eYAxis, FbxAxisSystem::eParityOdd, FbxAxisSystem::ECoordSystem::eRightHanded);
	//newSystem.ConvertScene(scene);

	if (scene == nullptr)
	{
		std::cout << "File " << path << "could not be loaded.\n";
		return false;
	}

	FbxNodeAttribute* attrib;
	for (int i = 0; i < scene->GetNodeCount(); i++)
	{
		attrib = scene->GetNode(i)->GetNodeAttribute();
		if (attrib == nullptr) { continue; }
		switch (attrib->GetAttributeType())
		{
		case FbxNodeAttribute::eMesh:
			meshes.push_back(static_cast<FbxMesh*>(attrib));
			std::cout << "Mesh found: " << attrib->GetName() << "\n";
			break;
		case FbxNodeAttribute::eSkeleton:
			if ((static_cast<FbxSkeleton*>(attrib))->IsSkeletonRoot())
			{
				skeletonRoots.push_back(static_cast<FbxSkeleton*>(attrib));
				std::cout << "Skeleton root found: " << attrib->GetName() << "\n";
			}
			break;
        default:
            break;
		}
	}

	return true;
}


auto trc::FBXLoader::loadMesh(FbxMesh* mesh) -> MeshData
{
    MeshData result;

    // Load indices
	int indexCount = mesh->GetPolygonVertexCount();
    assert(indexCount >= 0);
	int* indices = mesh->GetPolygonVertices();

    result.vertices.resize(indexCount);
    result.indices.reserve(indexCount);
    for (int i = 0; i < indexCount; i++)
    {
        result.indices.push_back(static_cast<uint32_t>(indices[i]));
    }

    loadVertices(mesh, result);
    loadUVs(mesh, result);
    loadNormals(mesh, result);
    loadTangents(mesh, result);

	std::cout << "\t" << result.vertices.size() << " vertices have been loaded.\n"
		      << "\t" << result.indices.size() << " indices have been loaded.\n";

    return result;
}


void trc::FBXLoader::loadVertices(FbxMesh* mesh, MeshData& result)
{
	// Load individual vertices
	int vertCount = mesh->GetControlPointsCount();
	FbxVector4* verts = mesh->GetControlPoints();

	for (int i = 0; i < vertCount; i++)
    {
		result.vertices[i].position = vec3(verts[i].mData[0], verts[i].mData[1], verts[i].mData[2]);
	}
}


void trc::FBXLoader::loadUVs(FbxMesh* mesh, MeshData& result)
{
	FbxStringList uvSetNameList;
	mesh->GetUVSetNames(uvSetNameList);
	if (uvSetNameList.GetCount() <= 0)
	{
		std::cout << "\tMesh doesn't have any uv sets!\n";
		return;
	}
    std::cout << "\tMesh has " << uvSetNameList.GetCount() << " UV sets. Using the first one.\n";

	FbxGeometryElementUV* uvElement = mesh->GetElementUV(uvSetNameList.GetStringAt(0));

	// Mapping mode is by control-point
	if (uvElement->GetMappingMode() == FbxGeometryElement::eByControlPoint)
	{
		std::cout << "ATTENTION! UV mapping mode is by control-point, this could cause undefined behaviour.\n";
        assert(false);

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
			result.vertices[uvIndex].uv = vec2(uv[0], uv[1]);
		}
	}
}


void trc::FBXLoader::loadNormals(FbxMesh* mesh, MeshData& result)
{
	FbxGeometryElementNormal* normalElement = mesh->GetElementNormal();

	// Mapping mode is by control-point (vertex)
	if (normalElement->GetMappingMode() == FbxGeometryElement::eByControlPoint)
	{
		std::cout << "ATTENTION! Normal mapping mode is by control-point, this could cause undefined behaviour.\n";
        assert(false);

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
		int indexCount = mesh->GetPolygonVertexCount();

		for (int i = 0; i < indexCount; i++)
		{
			int normalIndex = i;

			FbxVector4 normal = normalElement->GetDirectArray().GetAt(normalIndex);
			result.vertices[normalIndex].normal = vec3(normal[0], normal[1], normal[2]);
		}
	}
}


void trc::FBXLoader::loadTangents(FbxMesh* mesh, MeshData& result)
{
	FbxGeometryElementTangent* tangentElement = mesh->GetElementTangent();
	if (tangentElement == nullptr)
	{
		std::cout << "\tNo precomputed tangents found. Computing tangents...\n";
		computeTangents(result);

		return;
	}

	if (tangentElement->GetMappingMode() == FbxGeometryElement::eByControlPoint)
	{
		std::cout << "ATTENTION! Tangent mapping mode is by control point, this is not implemented!\n";
        assert(false);
	}

	if (tangentElement->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)
	{
		int indexCount = mesh->GetPolygonVertexCount();

		for (int i = 0; i < indexCount; i++)
		{
			FbxVector4 tangent = tangentElement->GetDirectArray().GetAt(i);
			result.vertices[i].tangent = vec3(tangent[0], tangent[1], tangent[2]);
		}
	}
}


void trc::FBXLoader::computeTangents(MeshData& result)
{
	if (result.indices.empty() || result.indices.size() % 3 != 0)
	{
		std::cout << "Failed to compute tangents. Indices are either not existing or the mesh is not triangulated.\n";
		return;
	}
	if (result.vertices.empty())
	{
		std::cout << "Failed to compute tangents. No vertices loaded for the mesh.\n";
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

		mat3x2 DE_mat = mat3x2(D.x, E.x, D.y, E.y, D.z, E.z);
		mat2 FG_mat = mat2(F, G);

		mat3x2 TU_mat = (determinant(FG_mat) != 0 ? inverse(FG_mat) : mat2(1.0f)) * DE_mat;

		vec3 T = vec3(TU_mat[0][0], TU_mat[1][0], TU_mat[2][0]); // Tangent
		// vec3 U = vec3(TU_mat[0][1], TU_mat[1][1], TU_mat[2][1]); // Bitangent
		//      ^ currently unused
		vec3 N = normalize(result.vertices[result.indices[i]].normal);

		// Orthonormalize (Gram-Schmidt)
		vec3 T_ = T - dot(N, T) * N;						// T_ represents the mathematical T'
		//vec3 _U = U - dot(N, U) * N - dot(_T, U) * _T;
		//     ^^ currently unused

		// Because we orthonormalized the T-B-N-Matrix, we can use the transpose
		//		| T'.x  T'.y  T'.z |
		//		| U'.x  U'.y  U'.z |
		//		| N.x   N.y   N.z  |
		// as the inverse of the calculated Matrix.
		//
		// This is necessary because the calculated T, U and N vectors are columns of the Tangentspace-to-Objectspace-Matrix.
		// Since we need to transform from object- to tangentspace, we would have to calculate the inverse matrix
		// inside of the shader, which we want to avoid.

		result.vertices[result.indices[i + 0]].tangent = T_;
		result.vertices[result.indices[i + 1]].tangent = T_;
		result.vertices[result.indices[i + 2]].tangent = T_;

		// I don't actually use the bitangent since I can just calculate it in the shader
		// with a simple cross product. Seems better than an additional vertex attribute.
	}

	std::cout << "\t" << result.indices.size() << " tangents and bitangents computed.\n";
}


// void trc::FBXLoader::loadMaterials(FbxMesh* mesh, MeshConstructionParams*)
// {
// 	FbxNode* meshNode = mesh->GetNode();
// 	int materialCount = meshNode->GetMaterialCount();
// 	std::cout << "\t" << materialCount << " materials found.\n";
//
// 	FbxSurfaceMaterial* mat = meshNode->GetMaterial(0);
// 	if (mat != nullptr)
// 	{
// 		if (GetImplementation(mat, FBXSDK_IMPLEMENTATION_HLSL) != nullptr
// 			|| GetImplementation(mat, FBXSDK_IMPLEMENTATION_CGFX) != nullptr) {
// 			std::cout << "Material " << mat->GetName() << " is some strange hardware shader, I don't know what to do with that.\n";
// 		}
// 		// Material is a Phong material
// 		else if (mat->GetClassId().Is(FbxSurfacePhong::ClassId))
// 		{
// 			std::cout << "\tLoading Phong material " << mat->GetName() << ".\n";
// 			auto phongMat = dynamic_cast<FbxSurfacePhong*>(mat);
// 			Material newMaterial;
//
// 			FbxDouble3 colorAmbient = phongMat->Ambient;
// 			FbxDouble3 colorDiffuse = phongMat->Diffuse;
// 			FbxDouble3 colorSpecular = phongMat->Specular;
// 			newMaterial.ka = vec4(colorAmbient.mData[0], colorAmbient.mData[1], colorAmbient.mData[2], 1.0f);
// 			newMaterial.ka = vec4(1.0f); // FBX does not fully export materials. Ambient color, for example, gets lost
// 			newMaterial.kd = vec4(colorDiffuse.mData[0], colorDiffuse.mData[1], colorDiffuse.mData[2], 1.0f);
// 			newMaterial.ks = vec4(colorSpecular.mData[0], colorSpecular.mData[1], colorSpecular.mData[2], 1.0f);
// 			newMaterial.exponent = static_cast<float>(phongMat->Shininess);
// 			//newMaterial.opacity = 1.0f - (float)phongMat->TransparencyFactor.Get();
//
// 			//newMeshParams->material = newMaterial;
// 		}
// 		// Material is a Lambert material
// 		else if (mat->GetClassId().Is(FbxSurfaceLambert::ClassId))
// 		{
// 			std::cout << "\tLoading Lambert material " << mat->GetName() << " with standard values for specular color and shinyness.\n";
// 			auto lambertMat = dynamic_cast<FbxSurfaceLambert*>(mat);
// 			Material newMaterial;
//
// 			FbxDouble3 colorAmbient = lambertMat->Ambient;
// 			FbxDouble3 colorDiffuse = lambertMat->Diffuse;
// 			newMaterial.ka = vec4(colorAmbient.mData[0], colorAmbient.mData[1], colorAmbient.mData[2], 1.0f);
// 			newMaterial.ka = vec4(1.0f); // FBX does not fully export materials. Ambient color, for example, gets lost
// 			newMaterial.kd = vec4(colorDiffuse.mData[0], colorDiffuse.mData[1], colorDiffuse.mData[2], 1.0f);
// 			newMaterial.ks = vec4(1.0f); // Standard value
// 			const float STANDARD_EXPONENT = 16.0f;
// 			newMaterial.exponent = STANDARD_EXPONENT; // Standard value
// 			//newMaterial.opacity = 1.0f - (float)lambertMat->TransparencyFactor.Get();
//
// 			//newMeshParams->material = newMaterial;
// 		}
// 		else {
// 			std::cout << "Material " << mat->GetName() << " is an unknown material type.\n";
// 		}
// 	}
// 	else {// if mat == nullptr
// 		std::cout << "\tNo materials were loaded for mesh! Standard material will be used.\n";
// 	}
// }
//
//
// void trc::FBXLoader::loadSkeleton(FbxMesh* mesh, ImportResult* newMeshParams)
// {
// 	createBonesFromSkeleton(skeletonRoots[0]->GetNode(), nullptr, &newMeshParams->rig.value());
// 	fillBoneData(mesh, newMeshParams);
// }
//
//
// void trc::FBXLoader::createBonesFromSkeleton(FbxNode* currentBoneNode, AnimationBone* parent, AnimRigConstrParams* newRigParams)
// {
// 	static int boneIndex = 0;
//     // Reset the bone index in case other meshes are imported with the same fbxloader instance
//     if (parent == nullptr) boneIndex = 0;
//
// 	AnimationBone* newBone = nullptr;
// 	if (currentBoneNode->GetChildCount() > 0) // Ignore 'end' bones added by fbx
// 	{
// 		newBone = &newRigParams->bones.emplace_back(AnimationBone(currentBoneNode->GetName(), parent));
// 		boneNodes.push_back(currentBoneNode); // Save nodes for transformation evaluation
// 		nameToBoneIndex[newBone->getName()] = boneIndex++; // Allows indexing into the non-hierarchical bones array
// 	}
//
// 	for (int i = 0; i < currentBoneNode->GetChildCount(); i++) {
// 		createBonesFromSkeleton(currentBoneNode->GetChild(i), newBone, newRigParams);
// 	}
// }
//
//
// void trc::FBXLoader::fillBoneData(FbxMesh* mesh, ImportResult* newMeshParams)
// {
// 	// I don't know what this does but apparently it's important
// 	FbxVector4 T = mesh->GetNode()->GetGeometricTranslation(FbxNode::eSourcePivot);
// 	FbxVector4 R = mesh->GetNode()->GetGeometricRotation(FbxNode::eSourcePivot);
// 	FbxVector4 S = mesh->GetNode()->GetGeometricScaling(FbxNode::eSourcePivot);
// 	FbxAMatrix geometryTransformMat = FbxAMatrix(T, R, S);
//
// 	auto newRigParams = &newMeshParams->rig.value();
// 	newRigParams->vertexWeights = std::vector<vec4>(newMeshParams->mesh.vertices.size());
// 	newRigParams->vertexBoneIndices = std::vector<ivec4>(newMeshParams->mesh.vertices.size());
//
// 	std::vector<glm::length_t> weightsFilledHelper(newMeshParams->mesh.vertices.size());
// 	int deformerCount = mesh->GetDeformerCount();
//
// 	// Deformers are whole skeletons of a mesh (probably)
// 	for (int deformerIndex = 0; deformerIndex < deformerCount; deformerIndex++)
// 	{
// 		std::cout << "\nSkin " << deformerIndex << ":\n\t";
// 		auto skin = static_cast<FbxSkin*>(mesh->GetDeformer(deformerIndex, FbxDeformer::eSkin));
// 		if (skin == nullptr) { continue; }
//
// 		////////////////
// 		// Get bone data
// 		size_t clusterCount = skin->GetClusterCount();
// 		std::vector<FbxCluster*> extractedClusters(clusterCount); // Store the clusters for animation extraction
//
// 		unsigned int addedWeights = 0; // For logging only
// 		unsigned int correctedWeights = 0; // For logging only
// 		// Clusters hold bones, which are links (probably)
// 		for (size_t clusterIndex = 0; clusterIndex < clusterCount; clusterIndex++)
// 		{
// 			auto cluster = skin->GetCluster(clusterIndex);
// 			std::string currBoneName = cluster->GetLink()->GetName();
// 			size_t currBoneIndex = static_cast<size_t>(nameToBoneIndex[currBoneName]);
// 			extractedClusters[currBoneIndex] = cluster; // For animations
//
// 			// Calculate bind pose inverse matrix
// 			FbxAMatrix meshTransformMat;
// 			FbxAMatrix boneBindPoseMat;
// 			FbxAMatrix bindPoseInvMat;
//
// 			cluster->GetTransformMatrix(meshTransformMat);
// 			cluster->GetTransformLinkMatrix(boneBindPoseMat);
// 			bindPoseInvMat = boneBindPoseMat.Inverse() * meshTransformMat * geometryTransformMat;
//
// 			newRigParams->bones[currBoneIndex].setBindPoseInverseMatrix(fbxToGlm(bindPoseInvMat));
// 			newRigParams->bones[currBoneIndex].setAnimationTransformMatrix(fbxToGlm(boneBindPoseMat));
//
// 			// Fill vertex weight data
// 			int indexCount = cluster->GetControlPointIndicesCount();
// 			int* indices = cluster->GetControlPointIndices();
// 			double* weights = cluster->GetControlPointWeights();
//
// 			for (int i = 0; i < indexCount; i++)
// 			{
// 				size_t vertIndex = static_cast<size_t>(indices[i]);
// 				if (weightsFilledHelper[vertIndex] < MAX_WEIGHTS_PER_VERTEX)
// 				{
// 					newRigParams->vertexWeights[vertIndex][weightsFilledHelper[vertIndex]] = static_cast<float>(weights[i]);
// 					newRigParams->vertexBoneIndices[vertIndex][weightsFilledHelper[vertIndex]] = static_cast<int>(currBoneIndex);
// 					weightsFilledHelper[vertIndex]++;
// 				}
//
// 				// Weight sum correction
// 				vec4& weight = newRigParams->vertexWeights[vertIndex];
// 				double weightSum = static_cast<double>(weight.x) + static_cast<double>(weight.y)
// 								   + static_cast<double>(weight.z) + static_cast<double>(weight.w);
// 				if (weightSum > 1.0001)
// 				{
// 					double overflow = weightSum - 1.0;
// 					// Ratio-sustaining method
// 					for (int j = 0; j < 4; j++) {
// 						weight[j] -= static_cast<float>(overflow * (weight[j] / weightSum));
// 					}
// 					//std::cout << "Weight sum corrected at bone " << currBoneName << "\n";
// 					correctedWeights++;
// 				}
// 				if (weightSum < 0.9999)
// 				{
// 					double missing = 1.0 - weightSum;
// 					for (int j = 0; j < 4; j++) {
// 						weight[j] += static_cast<float>(missing * (weight[j] / weightSum));
// 					}
// 					correctedWeights++;
// 				}
// 				addedWeights++;
// 			}
// 		} // Per-bone end
//
// 		std::cout << newRigParams->bones.size() << " bones loaded.\n\t"
// 			<< addedWeights << " weights loaded.\n\t"
// 			<< correctedWeights << " weights corrected.\n";
//
// 		/////////////////////
// 		// Get animation data
// 		int animStackCount = scene->GetSrcObjectCount<FbxAnimStack>();
// 		std::cout << "Animation count: " << animStackCount << "\n";
//
// 		for (int animStackIndex = 0; animStackIndex < animStackCount; animStackIndex++)
// 		{
// 			auto animStack = scene->GetSrcObject<FbxAnimStack>(animStackIndex);
//
// 			std::cout << "Animation " << animStackIndex << ": "
// 				<< animStack->GetName() << "\n";
//
// 			// Animation time
// 			FbxTime::EMode timeMode = FbxTime::EMode::eFrames24;
// 			FbxTimeSpan timespan = animStack->GetLocalTimeSpan();
// 			size_t totalFrames = timespan.GetDuration().GetFrameCount(timeMode);
// 			std::cout << "\tDuration: " << totalFrames << " frames\n";
//
// 			AnimationData newAnimData;
// 			newAnimData.frameCount = totalFrames;
// 			newAnimData.animationTime = static_cast<float>(timespan.GetDuration().GetMilliSeconds()), animStack->GetName();
// 			newAnimData.frameTime = newAnimData.animationTime / newAnimData.frameCount;
//
// 			// Create keyframes
// 			newAnimData.keyframes.resize(totalFrames);
// 			for (size_t frame = 0; frame < totalFrames; frame++)
// 			{
// 				Keyframe newKey;
// 				newKey.frame = frame;
// 				// Fill the keyframe's bones
// 				for (size_t clusterIndex = 0; clusterIndex < clusterCount; clusterIndex++)
// 				{
// 					FbxTime frameTime = FbxTime();
// 					frameTime.SetFrame(frame, timeMode);
// 					FbxAMatrix boneTransform = boneNodes[clusterIndex]->EvaluateGlobalTransform(frameTime);
//
// 					// Precompute boneMatrix * bindposeInverseMatrix for per-frame-bones
// 					mat4 boneAnimationTransformMatrix = fbxToGlm(boneTransform) * newRigParams->bones[clusterIndex].getBindPoseInverseMatrix();
// 					newKey.bones.push_back(boneAnimationTransformMatrix);
// 				}
//
// 				newAnimData.keyframes[frame] = newKey;
// 			}
//
// 			newRigParams->animations.emplace_back(std::move(newAnimData));
// 		}
//
// 		// Fill weight data array
// 		newRigParams->weightArray.clear();
// 		newRigParams->boneIndexArray.clear();
// 		for (size_t indicesIndex = 0; indicesIndex < newMeshParams->mesh.indices.size(); indicesIndex++)
// 		{
// 			newRigParams->boneIndexArray.emplace_back(vec4(0));
// 			newRigParams->weightArray.emplace_back(vec4(0));
// 			for (size_t vec4Index = 0; vec4Index < 4; vec4Index++)
// 			{
// 				size_t index = static_cast<size_t>(newMeshParams->mesh.indices[indicesIndex]);
// 				newRigParams->weightArray[indicesIndex][vec4Index] = newRigParams->vertexWeights[index][vec4Index];
// 				newRigParams->boneIndexArray[indicesIndex][vec4Index] = newRigParams->vertexBoneIndices[index][vec4Index];
// 			}
// 		}
// 	} // Per-skin end
// }




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
