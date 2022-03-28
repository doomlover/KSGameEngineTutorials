#include "engine_pch.h"
#include "Core/MeshAsset.h"

namespace ks
{

	void FStaticMeshAsset::PostLoad()
	{
		InitRenderData();
	}

	void FStaticMeshAsset::InitRenderData()
	{
		RenderData.reset(new FMeshRenderData(MeshData));
	}
}