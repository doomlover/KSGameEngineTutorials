#pragma once

#include "Core/Math.h"
#include "Core/Json.h"

namespace ks
{
	struct FSceneNodeInfo;
	struct FMeshData;
	class FStaticMeshAsset;
	

	/* see LoadGLTF */
namespace gltf
{
	using json = nlohmann::json;

	template<typename _KeyT, typename _ValT>
	std::ostringstream& operator<<(std::ostringstream& _ostream, const std::pair<_KeyT, _ValT>& _pair)
		{
			_ostream << "{" << _pair.first << "," << _pair.second << "}";
			return _ostream;
		}

	template<typename ContainerT, typename ElemT = ContainerT::value_type, char _Delimiter = ' '>
	std::ostringstream& operator<<(std::ostringstream& _ostream, const ContainerT& _container)
		{
			std::for_each(_container.begin(), _container.end(), [&_ostream](const ElemT& elem) {
				_ostream << elem << _Delimiter;
			});
			return _ostream;
		}

	struct FSceneInfo
		{
			std::string name;
			std::vector<int32> nodes;
			friend std::ostringstream& operator<<(std::ostringstream& outstream, const FSceneInfo& obj)
			{
				outstream << "name : " << obj.name.c_str() << std::endl;
				outstream << "nodes : [";
				outstream << obj.nodes << "]";
				return outstream;
			}
			NLOHMANN_DEFINE_TYPE_INTRUSIVE(FSceneInfo, name, nodes)
		};

	struct FNodeInfo
	{
		inline const std::string& GetName() const { return name; }
		inline bool IsMeshNode() const { return mesh != -1; }
		inline glm::vec3 GetTranslation() const {
			return glm::vec3(translation[0], translation[1], translation[2]);
		}
		inline glm::quat GetRotation() const {
			return glm::quat(rotation[3], rotation[0], rotation[1], rotation[2]);
		}
		inline glm::vec3 GetScale() const {
			return glm::vec3(scale[0], scale[1], scale[2]);
		}

		std::string name;
		std::array<float, 3> translation{ 0, 0, 0 };
		std::array<float, 4> rotation{ 0, 0, 0, 1 };
		std::array<float, 3> scale{ 1, 1, 1 };
		std::vector<int32> children;
		int32 parent{ -1 };
		int32 mesh{ -1 };
		int32 camera{ -1 };
		int32 light{ -1 };
		friend std::ostringstream& operator<<(std::ostringstream& outstream, const FNodeInfo& obj)
		{
			outstream << "\nname : " << obj.name.c_str() << std::endl;
			outstream << "translation : [";
			outstream << obj.translation << "]\n";
			outstream << "rotation : [";
			outstream << obj.rotation << "]\n";
			outstream << "scale : [";
			outstream << obj.scale << "]\n";
			outstream << "children : [";
			outstream << obj.children << "]\n";
			outstream << "mesh : " << obj.mesh << std::endl;
			return outstream;
		}
		friend void to_json(json& j, const FNodeInfo& SceneNodeInfo)
		{
			assert(false);
		}
		friend void from_json(const json& j, FNodeInfo& SceneNodeInfo)
		{
			j.at("name").get_to(SceneNodeInfo.name);

			if (j.contains("translation"))
			{
				j.at("translation").get_to(SceneNodeInfo.translation);
			}
			if (j.contains("rotation"))
			{
				j.at("rotation").get_to(SceneNodeInfo.rotation);
			}
			if (j.contains("scale"))
			{
				j.at("scale").get_to(SceneNodeInfo.scale);
			}
			if (j.contains("children"))
			{
				j.at("children").get_to(SceneNodeInfo.children);
			}
			if (j.contains("mesh"))
			{
				j.at("mesh").get_to(SceneNodeInfo.mesh);
			}
			if (j.contains("camera"))
			{
				j.at("camera").get_to(SceneNodeInfo.camera);
			}
			if (j.contains("extensions"))
			{
				auto& extensions{ j.at("extensions") };
				if (extensions.contains("KHR_lights_punctual"))
				{
					extensions.at("KHR_lights_punctual").at("light").get_to(SceneNodeInfo.light);
				}
			}
		}
	};
	/*
	* attributes : <attribute_name, accessor_id>
	* attribute_name : POSITION, NORMAL, TANGENT, TEXCOORD_0, ...
	*/
	struct FMesh
	{
		std::string name;
		struct FPrimitive
		{
			std::map<std::string, int32> attributes;
			int32 indices = -1;
			int32 material = -1;
			friend std::ostringstream& operator<<(std::ostringstream& outstream, const FPrimitive& obj)
				{
					outstream << "\nattributes:{";
					outstream << obj.attributes << "}\n";
					outstream << "indices:";
					outstream << obj.indices << std::endl;
					outstream << "material:";
					outstream << obj.material;
					return outstream;
				}
			NLOHMANN_DEFINE_TYPE_INTRUSIVE(FPrimitive, attributes, indices, material)
		};
		std::vector<FPrimitive> primitives;
		friend std::ostringstream& operator<<(std::ostringstream& outstream, const FMesh& obj)
			{
				outstream << "\n[name:";
				outstream << obj.name.c_str() << std::endl;
				outstream << "primitives:[";
				outstream << obj.primitives << "]\n";
				return outstream;
			}
		NLOHMANN_DEFINE_TYPE_INTRUSIVE(FMesh, name, primitives)
	};
	/*
	* count:
	* number of elements
	* 
	* type:
	* SCALAR, VEC2, VEC3, VEC4, MAT2, MAT3, MAT4
	* 
	* componentType:
	* 5120 BYTE
	* 5121 UNSIGNED_BYTE
	* 5122 SHORT
	* 5123 UNSIGNED_SHORT
	* 5125 UNSIGNED_INT
	* 5126 FLOAT
	*/
	struct FAccessor
		{
			int32 bufferView = -1;
			int32 count = 0;
			std::string type;
			int32 componentType = -1;
			friend std::ostringstream& operator<<(std::ostringstream& _ostream, const FAccessor& accessor)
			{
				_ostream << "\n{";
				_ostream << "\n\tbufferView : " << accessor.bufferView;
				_ostream << "\n\tcount : " << accessor.count;
				_ostream << "\n\ttype : " << accessor.type.c_str();
				_ostream << "\n\tcomponentType : " << accessor.componentType;
				_ostream << "\n}";
				return _ostream;
			}
			NLOHMANN_DEFINE_TYPE_INTRUSIVE(FAccessor, bufferView, count, type, componentType)
		};
	/*
	* target:
	* 34962 ARRAY_BUFFER
	* 34963 ELEMENT_ARRAY_BUFFER
	*/
	struct FBufferView
		{
			int32 buffer{ -1 };
			int32 byteLength{ 0 };
			int32 byteOffset{ 0 };
			int32 target{ -1 };
			friend void to_json(json&, const FBufferView&) { assert(false); }
			friend void from_json(const json& j, FBufferView& BufferView)
			{
				j.at("buffer").get_to(BufferView.buffer);
				j.at("byteLength").get_to(BufferView.byteLength);
				if (j.contains("byteOffset"))
				{
					j.at("byteOffset").get_to(BufferView.byteOffset);
				}
				if (j.contains("target"))
				{
					j.at("target").get_to(BufferView.target);
				}
			}
			friend std::ostringstream& operator<<(std::ostringstream& _ostream, const FBufferView& BufferView)
			{
				_ostream << "\n{";
				_ostream << "\n\tbuffer : " << BufferView.buffer;
				_ostream << "\n\tbyteLength : " << BufferView.byteLength;
				_ostream << "\n\tbyteOffset : " << BufferView.byteOffset;
				_ostream << "\n\ttarget : " << BufferView.target;
				_ostream << "\n}";
				return _ostream;
			}
		};

	struct FBuffer
		{
			std::string uri;
			int32 byteLength = 0;
			std::vector<uint8> RawData;
			friend std::ostringstream& operator<<(std::ostringstream& _ostream, const FBuffer& Buffer)
			{
				_ostream << "\n{";
				_ostream << "\n\turi : " << Buffer.uri;
				_ostream << "\n\tbyteLength : " << Buffer.byteLength;
				_ostream << "\n}";
				return _ostream;
			}
			NLOHMANN_DEFINE_TYPE_INTRUSIVE(FBuffer, uri, byteLength)
		};

	struct FCamera
		{
			std::string name;
			std::string type;
			struct FPerspective
			{
				float yfov;
				float zfar;
				float znear;
				NLOHMANN_DEFINE_TYPE_INTRUSIVE(FPerspective, yfov, zfar, znear)
			} perspective{};

			struct FOrthographic
			{
				float xmag, ymag, zfar, znear;
				NLOHMANN_DEFINE_TYPE_INTRUSIVE(FOrthographic, xmag, ymag, zfar, znear)
			} orthographic{};

			friend void to_json(json&, const FCamera&) { assert(false); }
			friend void from_json(const json& j, FCamera& Camera) {
				j.at("name").get_to(Camera.name);
				j.at("type").get_to(Camera.type);
				if (Camera.type == "perspective")
				{
					j.at("perspective").get_to(Camera.perspective);
				}
				else
				{
					j.at("orthographic").get_to(Camera.orthographic);
				}
			}
		};

	struct FKHRLightsPunctual
	{
		struct FLight
		{
			std::string name;
			std::string type;
			float intensity{0.f};
			NLOHMANN_DEFINE_TYPE_INTRUSIVE(FLight, name, type, intensity)
		};
		std::vector<FLight> lights;
		NLOHMANN_DEFINE_TYPE_INTRUSIVE(FKHRLightsPunctual, lights)
	};

	struct FExtension
	{
		FKHRLightsPunctual KHR_lights_punctual;
		friend void to_json(json&, const FExtension&) { assert(false); }
		friend void from_json(const json& j, FExtension& extension) {
			if (j.contains("KHR_lights_punctual"))
			{
				j.at("KHR_lights_punctual").get_to(extension.KHR_lights_punctual);
			}
		}
	};

	struct FMaterial
	{
		std::string name;
		struct FPBRMetallicRoughness
		{
			float baseColorFactor[4]{0.f};
			float metallicFactor{0.f};
			float roughnessFactor{0.f};
			NLOHMANN_DEFINE_TYPE_INTRUSIVE(FPBRMetallicRoughness, baseColorFactor, metallicFactor, roughnessFactor)
		};
		FPBRMetallicRoughness pbrMetallicRoughness;
		NLOHMANN_DEFINE_TYPE_INTRUSIVE(FMaterial, name, pbrMetallicRoughness)
	};

	struct FScene
	{
		int32 scene = -1;
		std::string root;
		std::vector<FSceneInfo> scenes;
		std::vector<FNodeInfo> nodes;
		std::vector<FMesh> meshes;
		std::vector<FAccessor> accessors;
		std::vector<FBufferView> bufferViews;
		std::vector<FBuffer> buffers;
		std::vector<FCamera> cameras;
		std::vector<FMaterial> materials;
		FExtension extensions;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(FScene, scene, scenes, nodes, meshes,
			accessors, bufferViews, buffers, cameras, materials, extensions)
		friend std::ostringstream& operator<<(std::ostringstream& outstream, const FScene& obj)
		{
			outstream << "scene : ";
			outstream << obj.scene << "\n\n";
			outstream << "scenes : [";
			outstream << obj.scenes << "]\n\n";
			outstream << "nodes : [";
			outstream << obj.nodes << "]\n\n";
			outstream << "meshes : [";
			outstream << obj.meshes << "]\n\n";
			outstream << "accessors : [";
			outstream << obj.accessors << "]\n\n";
			outstream << "bufferViews : [";
			outstream << obj.bufferViews << "]\n\n";
			outstream << "buffers : [";
			outstream << obj.buffers << "]\n\n";
			return outstream;
		}

		void DebugPrint() const
		{
#ifdef KS_DEBUG_BUILD
			std::ostringstream _ostream;
			_ostream << *this;
			std::cout << _ostream.str();
#endif
		}
	};
} // ~gltf


	class IAsset
	{
	public:
		using FRefType = std::shared_ptr<IAsset>;
		IAsset(const std::string& InPath) : Path(InPath) {}
		virtual ~IAsset() = 0;
		const std::string& GetPath() const { return Path; }
		virtual void PostLoad() {}
	protected:
		std::string Path;
		std::vector<FRefType> RefAssets;
	};

	class FSceneAsset : public IAsset
	{
	public:
		FSceneAsset(const std::string& GLTFPath);
		virtual ~FSceneAsset() {}
		int32 GetNumNodes() const {
			return static_cast<int32>(GltfScene.nodes.size());
		}
		void GetSceneNodeInfo(int32 NodeIndex, FSceneNodeInfo& NodeInfo) const;
		std::vector<FSceneNodeInfo> GetSceneNodeInfos() const;
	private:
		/* loading point */
		void LoadGLTF();
		/* load all contained assets */
		void LoadContainedAssets();

		/* mesh infos */
		const std::string& GetMeshName(int32 MeshIndex) const {
			return GltfScene.meshes.at(MeshIndex).name;
		}

		gltf::FScene GltfScene;
	};
}