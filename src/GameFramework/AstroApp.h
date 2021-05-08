#pragma once

#define GLFW_INCLUDE_VULKAN //this will make glfw include the vulkan header
#include <GLFW/glfw3.h>
#include <vector>


class AstroApp 
{
public:
    void Run();

private:

	void InitWindow();
    void InitVulkan();
	void CreateVkInstance();
	void CreateVkLogicalDevice();
	void CreateSurface();
	void CreateSwapchain();

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
	VkSurfaceKHR m_surface;

	// Queues
	VkQueue m_graphicsQueue;
	VkQueue m_presentQueue;

	// Swapchain
	VkFormat m_swapChainImageFormat;
	VkExtent2D m_swapChainExtent;
	VkSwapchainKHR m_swapChain;
	std::vector<VkImage> m_swapChainImages;



};
