#pragma once

#define GLFW_INCLUDE_VULKAN //this will make glfw include the vulkan header
#include <GLFW/glfw3.h>

class AstroApp 
{
public:
    void Run();

private:
	void InitWindow();
    void InitVulkan();
	void CreateVkInstance();
	void CreateVkLogicalDevice();
    void MainLoop();
    void Shutdown();

	void CheckExtensions();
#ifndef NDEBUG
	bool CheckValidationLayers();
#endif
	void PickGPU();
	bool IsGPUSuitable(VkPhysicalDevice device);

private:
	GLFWwindow* m_window; 
	VkInstance m_instance;
	VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
	VkDevice m_logicalDevice;

	//Queues
	VkQueue graphicsQueue;

};
