#include "engine_pch.h"
#include "Core/Assets.h"
#include "Core/MeshAsset.h"
#include "Core/Scene.h"

namespace ks {

	namespace gltf {
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

			std::string GetMeshKeyName(const FScene& Scene, const std::string& MeshName) {
				return Scene.root + "/" + MeshName;
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
		}
	}

	FAssetManager* GAssetManager = nullptr;

	void FAssetManager::Init()
	{
		KS_INFO(TEXT("FAssetManager::Init"));
		GAssetManager = this;
	}

	void FAssetManager::Shutdown()
	{
		KS_INFO(TEXT("\tFAssetManager::Shutdown"));
	}

	std::shared_ptr<FSceneAsset> FAssetManager::CreateSceneAsset(const std::string& InGLTFPath)
	{
		KS_INFO(TEXT("FAssetManager::Load GLTF Scene"));

		if (InGLTFPath.empty())
		{
			return nullptr;
		}
		assert(InGLTFPath.ends_with(".gltf"));
		assert(Assets.find(InGLTFPath) == Assets.end());
		std::shared_ptr<FSceneAsset> SceneAsset = std::make_shared<FSceneAsset>(InGLTFPath);
		Assets.insert({ InGLTFPath, SceneAsset });
		return SceneAsset;
	}

	std::shared_ptr<ks::FStaticMeshAsset> FAssetManager::CreateStaticMeshAsset(const FMeshData& MeshData)
	{
		std::shared_ptr<FStaticMeshAsset> Asset = std::make_shared<FStaticMeshAsset>(MeshData.KeyName, const_cast<FMeshData&>(MeshData));
		Assets.insert({Asset->GetPath(), Asset});
		return Asset;
	}

	std::shared_ptr<ks::IAsset> FAssetManager::GetAsset(const std::string& Path)
	{
		auto& Asset = Assets.at(Path);
		return Asset;
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
		std::string FilePath = "Content" + Path;
		std::ifstream InStream(FilePath);
		assert(InStream.is_open());

		// load gltf data
		json GLTFLevelJson;
		InStream >> GLTFLevelJson;
		GltfScene = GLTFLevelJson.get<gltf::FScene>();
		GltfScene.root = FString::GetRootPath(Path);
		//GltfScene.PrintToLog();

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
		});

		int32 a = 0;
	}

	void FSceneAsset::GetSceneNodeInfo(int32 NodeIndex, FSceneNodeInfo& NodeInfo) const
	{
		const gltf::FNodeInfo& GLTF_NodeInfo = GltfScene.nodes.at(NodeIndex);

		NodeInfo.Name = GLTF_NodeInfo.GetName();
		NodeInfo.Translate = GLTF_NodeInfo.GetTranslation();
		NodeInfo.Rotation = GLTF_NodeInfo.GetRotation();
		NodeInfo.Scale = GLTF_NodeInfo.GetScale();

		if (GLTF_NodeInfo.mesh != -1)
		{
			const gltf::FMesh& GLTF_Mesh = GltfScene.meshes.at(GLTF_NodeInfo.mesh);
			NodeInfo.MeshAssetKeyName = gltf::GetMeshKeyName(GltfScene, GLTF_Mesh.name);
		}
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
				std::string FilePath = "Content" + Scene.root + "/" + Buffer.uri;
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

			void LoadMeshData(const FScene& Scene, const FMesh& Mesh, FMeshData& MeshData)
			{
				// get key name
				MeshData.KeyName = GetMeshKeyName(Scene, Mesh.name);
				KS_INFOA(MeshData.KeyName.c_str());

				const gltf::FMesh::FPrimitive& Primitive = Mesh.primitives.at(0);

				// load index buffer
				{
					const auto& Accessor = gltf::GetAccessor(Scene, Primitive.indices);
					MeshData.IndexDataType = static_cast<EDATA_TYPE>(Accessor.componentType);
					gltf::CopyRawData(Scene, Accessor, MeshData.IndexRawData);
				}
				// load vertex buffer
				{
					const auto& Accessor = gltf::GetAccessor(Scene, Primitive.attributes.at("POSITION"));
					MeshData.PositionDataType = static_cast<EDATA_TYPE>(Accessor.componentType);
					assert(MeshData.PositionDataType == EDATA_TYPE::FLOAT);
					MeshData.PositionElemType = gltf::GetElemType(Accessor.type);
					assert(MeshData.PositionElemType == EELEM_TYPE::VEC3);
					gltf::CopyRawData(Scene, Accessor, MeshData.PositionRawData);
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