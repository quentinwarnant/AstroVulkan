#include <Voxel/VoxelObject.h>

VoxelObject::VoxelObject( glm::vec3 position, int8_t* voxelData )
  : m_position( position )
  , m_voxelData( voxelData )
{
}

void VoxelObject::Render()
{
}

void VoxelObject::ComputeFrame()
{
}
