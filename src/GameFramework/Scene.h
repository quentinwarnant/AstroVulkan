
#pragma once

#include <Voxel/VoxelObject.h>
#include <memory>
//-----------------------

class Scene
{
  public:
	Scene();

	void Load();
	void Save();
	void ComputeFrame();

  private:
	std::unique_ptr<VoxelObject> m_testVoxelObject;
};