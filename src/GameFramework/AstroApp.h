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
	void SetupDebugMessenger();
	void CreateVkLogicalDevice();
	void CreateSurface();
	void CreateSwapchain();
	void CreateImageViews();
	void CreateRenderPass();
	void CreateGraphicsPipeline();
	void CreateFramebuffers();
	void CreateCommandPool();
	void CreateCommandBuffers();
	void CreateSemaphores();

	void MainLoop();
	void Shutdown();

	void DrawFrame();

	void PopulateDebugMessengerCreateInfo( VkDebugUtilsMessengerCreateInfoEXT& createInfo );

	std::vector<const char*> GetRequiredExtensions();
	void CheckExtensions();
#ifndef NDEBUG
	bool CheckValidationLayers();
#endif
	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
	  VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	  VkDebugUtilsMessageTypeFlagsEXT messageType,
	  const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	  void* pUserData );

	void PickGPU();
	bool IsGPUSuitable( VkPhysicalDevice device );

  private:
	GLFWwindow* m_window;
	VkInstance m_instance;
	VkDebugUtilsMessengerEXT m_debugMessenger;
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
	std::vector<VkImageView> m_swapChainImageViews; // Image views describes how we access an image (eg: 2D depth tex )

	// Pipeline
	VkRenderPass m_renderPass;
	VkPipelineLayout m_pipelineLayout;
	VkPipeline m_graphicsPipeline;

	// Framebuffer
	std::vector<VkFramebuffer> m_swapChainFramebuffers;

	// Commands
	VkCommandPool m_commandPool;
	std::vector<VkCommandBuffer> m_commandBuffers;

	// Rendering / Presenting
	std::vector<VkSemaphore> m_imageAvailableSemaphores;
	std::vector<VkSemaphore> m_renderFinishedSemaphores;
	std::vector<VkFence> m_inFlightFences;
	std::vector<VkFence> m_imagesInFlight;
	size_t m_currentFrame = 0;
};
