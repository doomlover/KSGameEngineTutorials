#include "engine_pch.h"
#include "Core/Asset/Assets.h"
#include "Core/Asset/MeshAsset.h"
#include "Core/Asset/MaterialData.h"
#include "Core/Asset/AssetManager.h"
#include "Core/Scene.h"

namespace ks {

	namespace gltf {
		struct FRawAttributeData
		{
			std::string Name;
			EELEM_TYPE ElemType{ EELEM_TYPE::INVALID };
			EDATA_TYPE DataType{ EDATA_TYPE::INVALID };
			uint32 Count{ 0 };
			std::vector<uint8> Data;
			size_t GetStride() const {
				return util::GetStride(ElemType, DataType);
			}
		};
		namespace {
			/* load bin file to the RawData */
			int LoadBuffer(const FScene& Scene, FBuffer& Buffer);
			/* load all bin files to RawData */
			void LoadBuffers(FScene& Scene);
			/* load mesh data form buffer */
			void LoadMeshData(const FScene& Scene, const FMesh& Mesh, FMeshData& MeshData);
			/* get accessor */
			const FAccessor& GetAccessor(const FScene& Scene, const int32 AccessorIndex);
			/* get elem type enum by the type name */
			EELEM_TYPE GetElemType(const std::string& TypeName);
			/* key name used by asset manager */
			std::string GetAssetKeyName(const FScene& Scene, const std::string& MeshName) {
				return Scene.root + "/" + MeshName;
			}
			/* set node parent id */
			void SetNodeParent(FScene& Scene) {
				int32 node_id = 0;
				for(auto& node_info : Scene.nodes)
				{
					for(auto& child_id : node_info.children) {
						Scene.nodes.at(child_id).parent = node_id;
					}
					++node_id;
				}
			}


			struct FBufferLoadingHelper
			{
				FScene& Scene;
				FBufferLoadingHelper(FScene& _Scene):Scene(_Scene) {
					LoadBuffers(Scene);
				}

				~FBufferLoadingHelper() {
					auto& Buffers{Scene.buffers};
					std::for_each(Buffers.begin(), Buffers.end(), [](FBuffer& Buffer) {
						Buffer.RawData.clear();
					});
				}
			};

			ECameraType GetCameraType(const std::string& type)
			{
				ECameraType Type = ECameraType::INVALID;
				if (type == "perspective")
				{
					Type = ECameraType::PERSPECTIVE;
				}
				else
				{
					Type = ECameraType::ORTHOGRAPHIC;
				}
				return Type;
			}

			void CopyRawData(const FScene& Scene, const FAccessor& Accessor, std::vector<uint8>& RawData)
			{
				const gltf::FBufferView& BufferView = Scene.bufferViews.at(Accessor.bufferView);
				// get copy start point
				const gltf::FBuffer& Buffer = Scene.buffers.at(BufferView.buffer);
				const auto SrcPoint = Buffer.RawData.data() + BufferView.byteOffset;

				// get copy length
				const int32& SrcLength{ BufferView.byteLength };

				// copy to raw data
				RawData.resize(SrcLength);
				auto DestPoint = RawData.data();
				size_t DestSize = RawData.size() * sizeof(uint8);
				assert(DestSize == SrcLength);

				bool error = memcpy_s(DestPoint, DestSize, SrcPoint, SrcLength);
				assert(!error);
			}

			void CopyRawDataByAccessor(FRawAttributeData& AttributeData, const FScene& Scene, int32 AccessorIndex) {
				auto Accessor = GetAccessor(Scene, AccessorIndex);
				AttributeData.ElemType = GetElemType(Accessor.type);
				AttributeData.DataType = static_cast<EDATA_TYPE>(Accessor.componentType);
				AttributeData.Count = Accessor.count;
				CopyRawData(Scene, Accessor, AttributeData.Data);
			}
		}
	}

	IAsset::~IAsset()
	{
		// check if any ref-asset needs to be released
		/*std::for_each(RefAssets.begin(), RefAssets.end(), [](FRefType& RefAsset) {
			int32 UseCount = RefAsset.use_count();
			if (UseCount == 2)
			{
				GAssetManager->UnloadAsset(RefAsset->GetPath());
			}
		});*/
	}

	FSceneAsset::FSceneAsset(const std::string& GLTFPath)
		:IAsset(GLTFPath)
	{
		LoadGLTF();
	}

	void FSceneAsset::LoadGLTF()
	{
		// open file stream
		std::string FilePath = util::GetContentPath(Path);
		std::ifstream InStream(FilePath);
		assert(InStream.is_open());

		// load gltf data
		json GLTFLevelJson;
		InStream >> GLTFLevelJson;
		GltfScene = GLTFLevelJson.get<gltf::FScene>();
		GltfScene.root = FString::GetRootPath(Path);
		gltf::SetNodeParent(GltfScene);

		// load contained assets
		LoadContainedAssets();
	}

	void FSceneAsset::LoadContainedAssets()
	{
		// load buffers
		gltf::FBufferLoadingHelper BufferLoader(GltfScene);

		// create static mesh asset from gltf mesh
		const std::vector<gltf::FMesh>& Meshes = GltfScene.meshes;
		std::for_each(Meshes.begin(), Meshes.end(), [&](const gltf::FMesh& Mesh) {
			FMeshData MeshData;
			gltf::LoadMeshData(GltfScene, Mesh, MeshData);
			std::shared_ptr<FStaticMeshAsset> StaticMeshAsset = GAssetManager->CreateStaticMeshAsset(MeshData);
			RefAssets.push_back(StaticMeshAsset);
			StaticMeshAsset->PostLoad();
		});

		int32 a = 0;
	}

	void FSceneAsset::GetSceneNodeInfo(int32 NodeIndex, FSceneNodeInfo& NodeInfo) const
	{
		const gltf::FNodeInfo& GLTF_NodeInfo = GltfScene.nodes.at(NodeIndex);

		NodeInfo.id = NodeIndex;
		NodeInfo.Name = GLTF_NodeInfo.GetName();
		NodeInfo.Translate = GLTF_NodeInfo.GetTranslation();
		NodeInfo.Rotation = GLTF_NodeInfo.GetRotation();
		NodeInfo.Scale = GLTF_NodeInfo.GetScale();
		NodeInfo.ChildrenIds = GLTF_NodeInfo.children;

		if (GLTF_NodeInfo.mesh != -1)
		{
			const gltf::FMesh& GLTF_Mesh = GltfScene.meshes.at(GLTF_NodeInfo.mesh);
			NodeInfo.MeshAssetKeyName = gltf::GetAssetKeyName(GltfScene, GLTF_Mesh.name);
		}

		if (GLTF_NodeInfo.camera != -1)
		{
			const gltf::FCamera GLTF_Camera = GltfScene.cameras.at(GLTF_NodeInfo.camera);
			NodeInfo.Camera.Type = gltf::GetCameraType(GLTF_Camera.type);
			const auto& Type{ NodeInfo.Camera.Type };
			switch (Type)
			{
			case ECameraType::PERSPECTIVE:
				NodeInfo.Camera.CameraData = {
					GLTF_Camera.perspective.yfov,
					GLTF_Camera.perspective.zfar,
					GLTF_Camera.perspective.znear
				};
				break;
			case ECameraType::ORTHOGRAPHIC:
				NodeInfo.Camera.CameraData = {
						GLTF_Camera.orthographic.xmag,
						GLTF_Camera.orthographic.ymag,
						GLTF_Camera.orthographic.zfar,
						GLTF_Camera.orthographic.znear
				};
				break;
			}
		}

		if (GLTF_NodeInfo.light != -1)
		{
			const gltf::FKHRLightsPunctual::FLight& GLTF_Light{ GltfScene.extensions.KHR_lights_punctual.lights.at(GLTF_NodeInfo.light) };
			NodeInfo.Light.Intensity = GLTF_Light.intensity;
			NodeInfo.Light.Type = util::GetLightType(GLTF_Light.type);
		}
	}

	std::vector<ks::FSceneNodeInfo> FSceneAsset::GetSceneNodeInfos() const
	{
		std::vector<FSceneNodeInfo> SceneNodeInfos;

		const gltf::FSceneInfo& SceneInfo{ GltfScene.scenes.at(GltfScene.scene) };
		std::deque<int32> NodeIds(SceneInfo.nodes.begin(), SceneInfo.nodes.end());
		while (!NodeIds.empty())
		{
			int32& NodeId{ NodeIds.front() };

			FSceneNodeInfo SceneNodeInfo;
			GetSceneNodeInfo(NodeId, SceneNodeInfo);
			SceneNodeInfos.push_back(SceneNodeInfo);

			for (auto& ChildId : SceneNodeInfo.ChildrenIds)
			{
				NodeIds.push_back(ChildId);
			}
			NodeIds.pop_front();
		}

		return SceneNodeInfos;
	}

	namespace gltf
	{
		namespace
		{
			void LoadBuffers(FScene& Scene)
			{
				std::for_each(Scene.buffers.begin(), Scene.buffers.end(), [&Scene](FBuffer& Buffer) {
					auto error = LoadBuffer(Scene, Buffer);
					assert(!error);
				});
			}

			int LoadBuffer(const FScene& Scene, FBuffer& Buffer)
			{
				std::string FilePath = util::GetContentPath(Scene.root + "/" + Buffer.uri);
				assert(Buffer.byteLength == std::filesystem::file_size(FilePath));

				std::ifstream BufferFileStream{ FilePath, std::ios::in | std::ios::binary };
				if (BufferFileStream.is_open())
				{
					auto& RawData{ Buffer.RawData };
					RawData.resize(Buffer.byteLength);
					char* data = reinterpret_cast<char*>(RawData.data());
					if (BufferFileStream.read(data, Buffer.byteLength))
					{
						return 0;
					}
				}

				return -1;
			}

			void LoadMeshData(const FScene& Scene, const FMesh& Mesh, FMeshData& MeshData)
			{
				// get key name
				MeshData.KeyName = GetAssetKeyName(Scene, Mesh.name);
				KS_INFOA(MeshData.KeyName.c_str());

				const gltf::FMesh::FPrimitive& Primitive = Mesh.primitives.at(0);

				// load index buffer
				{
					FRawAttributeData MeshIndexData;
					MeshIndexData.Name = "indices";
					gltf::CopyRawDataByAccessor(MeshIndexData, Scene, Primitive.indices);
					MeshData.IndexData.Count = MeshIndexData.Count;
					MeshData.IndexData.Stride = static_cast<uint32_t>(MeshIndexData.GetStride());
					MeshData.IndexData.DataType = MeshIndexData.DataType;
					MeshData.IndexData.Data = std::move(MeshIndexData.Data);
					auto& IndexData{MeshData.IndexData};
					if(IndexData.DataType == EDATA_TYPE::BYTE ||
					   IndexData.DataType == EDATA_TYPE::UNSIGNED_BYTE)
					{
						auto& IndexRawData{IndexData.Data};
						IndexData.DataType = EDATA_TYPE::UNSIGNED_SHORT;
						std::vector<uint16> RawData(IndexRawData.size());
						for (int32 i{0}; i < IndexRawData.size(); ++i)
						{
							RawData.at(i) = static_cast<uint16>(IndexRawData.at(i));
						}
						IndexRawData.resize(sizeof(uint16) * RawData.size());
						memcpy(IndexRawData.data(), RawData.data(), sizeof(uint16) * RawData.size());
					}
				}
				// load position buffer
				{
					FRawAttributeData MeshPositionData;
					MeshPositionData.Name = std::move(std::string("POSITION"));
					gltf::CopyRawDataByAccessor(MeshPositionData, Scene, Primitive.attributes.at(MeshPositionData.Name));
					MeshData.PositionData.Count = MeshPositionData.Count;
					MeshData.PositionData.Stride = static_cast<uint32>(MeshPositionData.GetStride());
					MeshData.PositionData.DataType = MeshPositionData.DataType;
					MeshData.PositionData.Data = std::move(MeshPositionData.Data);
					const auto& Accessor = GetAccessor(Scene, Primitive.attributes.at(MeshPositionData.Name));
					memcpy(&MeshData.Min.x, Accessor.min.data(), Accessor.min.size() * sizeof(float));
					memcpy(&MeshData.Max.x, Accessor.max.data(), Accessor.max.size() * sizeof(float));
				}
				// load all other vertex attributes
				{
					std::vector<std::string> AttributeNames = {"NORMAL", "TEXCOORD_0"};
					std::vector<FRawAttributeData> AttriDataAry;
					for (const auto& AttributeName : AttributeNames)
					{
						AttriDataAry.push_back(FRawAttributeData());
						AttriDataAry.back().Name = AttributeName;
						gltf::CopyRawDataByAccessor(AttriDataAry.back(), Scene, Primitive.attributes.at(AttributeName));
					}
					
					// merge all vertex attributes into a single attribute buffer
					size_t ByteSize{0};
					size_t Stride{0};
					const uint32 ElemCount{ AttriDataAry.begin()->Count };
					for (const auto& AttriData : AttriDataAry)
					{
						ByteSize += AttriData.Data.size();
						Stride += util::GetStride(AttriData.ElemType, AttriData.DataType);
						assert(ElemCount == AttriData.Count);
					}
					assert(ElemCount * Stride == ByteSize);
					std::vector<uint8> Data(ByteSize);
					uint8* pData{Data.data()};
					size_t ByteOffset{0};
					for (const auto& AttriData : AttriDataAry)
					{
						size_t AttriStride = util::GetStride(AttriData.ElemType, AttriData.DataType);
						const uint8* pAttriData{AttriData.Data.data()};
						for (uint32 i{0}; i < ElemCount; ++i)
						{
							memcpy(pData + ByteOffset + Stride * i, pAttriData + AttriStride * i, AttriStride);
						}
						ByteOffset += AttriStride;
					}

					MeshData.AttributeData.Count = ElemCount;
					MeshData.AttributeData.Stride = static_cast<uint32>(Stride);
					MeshData.AttributeData.DataType = AttriDataAry.begin()->DataType;
					MeshData.AttributeData.Data = std::move(Data);
				}
				// load material data
				if (Primitive.material != -1)
				{
					FMaterialData& MaterialData{MeshData.MaterialData};
					const int32& MaterialId{Primitive.material};
					const FMaterial& Material{Scene.materials.at(MaterialId)};
					MaterialData.KeyName = GetAssetKeyName(Scene, Material.name);
					memcpy(&MaterialData.BaseColorFactor[0], &Material.pbrMetallicRoughness.baseColorFactor[0], sizeof(float)*_countof(MaterialData.BaseColorFactor));
					MaterialData.MetallicFactor = Material.pbrMetallicRoughness.metallicFactor;
					MaterialData.RoughnessFactor = Material.pbrMetallicRoughness.roughnessFactor;
				}
			}

			const FAccessor& GetAccessor(const FScene& Scene, const int32 AccessorIndex)
			{
				return Scene.accessors.at(AccessorIndex);
			}

			EELEM_TYPE GetElemType(const std::string& TypeName)
			{
				if (TypeName == "SCALAR")
				{
					return EELEM_TYPE::SCALAR;
				}
				else if (TypeName == "VEC2")
				{
					return EELEM_TYPE::VEC2;
				}
				else if (TypeName == "VEC3")
				{
					return EELEM_TYPE::VEC3;
				}
				else if (TypeName == "VEC4")
				{
					return EELEM_TYPE::VEC4;
				}
				else if (TypeName == "MAT2")
				{
					return EELEM_TYPE::MAT2;
				}
				else if (TypeName == "MAT3")
				{
					return EELEM_TYPE::MAT3;
				}
				else
				{
					assert(TypeName == "MAT4");
					return EELEM_TYPE::MAT4;
				}
			}
		}
	}

}