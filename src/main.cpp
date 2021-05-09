//#define GLFW_INCLUDE_VULKAN //this will make glfw include the vulkan header
//#include <GLFW/glfw3.h>
//#include <vulkan/vulkan.hpp>

#include <cstdlib>
#include <iostream>
#include <stdexcept>

// #define GLM_FORCE_RADIANS
// #define GLM_FORCE_DEPTH_ZERO_TO_ONE
// #include <glm/vec4.hpp>
// #include <glm/mat4x4.hpp>

#include <GameFramework/AstroApp.h>

int main()
{
	AstroApp app;

	try
	{
		app.Run();
	}
	catch( const std::exception& e )
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}