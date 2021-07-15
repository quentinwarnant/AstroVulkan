
#include <GameFramework/Scene.h>

Scene::Scene()
  : m_testVoxelObject{}
{
}

void Scene::Load()
{
	//Imagine loading from a serialised scene file
	int8_t* voxelData = nullptr;

	m_testVoxelObject = std::make_unique<VoxelObject>( glm::vec3( 0, 0, 0 ), voxelData );
}

void Scene::Save()
{
}

void Scene::ComputeFrame()
{
	m_testVoxelObject->ComputeFrame();
}
