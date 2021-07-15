#pragma once

#include <glm/glm.hpp>

class VoxelObject
{
  public:
	VoxelObject( glm::vec3 position, int8_t* voxelData );

	void Render();
	void ComputeFrame();

  private:
	glm::vec3 m_position;
	int8_t* m_voxelData;
};