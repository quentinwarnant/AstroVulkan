#pragma once

#define GLFW_INCLUDE_VULKAN //this will make glfw include the vulkan header
#include <GLFW/glfw3.h>
#include <GameFramework/Scene.h>
#include <vector>

//------------------------------

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
	void CreateComputePipeline();
	void CreateFramebuffers();
	void CreateCommandPool();
	void CreateCommandBuffers();
	void CreateComputeCommandBuffers();
	void CreateSemaphores();

	void LoadScene();
	void MainLoop();
	void Shutdown();

	void ComputeFrame( uint32_t imageIndex );
	void DrawFrame( uint32_t imageIndex );

	void SetComputeCommandsToBuffer( VkCommandBuffer& commandBuffer );

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
	void PrintComputeBufferData();

	GLFWwindow* m_window;
	VkInstance m_instance;
	VkDebugUtilsMessengerEXT m_debugMessenger;
	VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
	VkDevice m_logicalDevice;
	VkSurfaceKHR m_surface;

	// Queues
	VkQueue m_graphicsQueue;
	VkQueue m_presentQueue;
	VkQueue m_computeQueue;

	// Swapchain
	VkFormat m_swapChainImageFormat;
	VkExtent2D m_swapChainExtent;
	VkSwapchainKHR m_swapChain;
	std::vector<VkImage> m_swapChainImages;
	std::vector<VkImageView> m_swapChainImageViews; // Image views describes how we access an image (eg: 2D depth tex )

	// Pipeline
	VkRenderPass m_renderPass;
	VkPipelineLayout m_graphicsPipelineLayout;
	VkPipeline m_graphicsPipeline;
	VkPipelineLayout m_computePipelineLayout;
	VkPipeline m_computePipeline;

	VkDescriptorPool m_computeDescriptorPool;
	VkDescriptorSetLayout m_computeDescriptorSetLayout;
	VkDescriptorSet m_computeDescriptorSet;

	// Framebuffer
	std::vector<VkFramebuffer> m_swapChainFramebuffers;

	// Memory
	std::vector<VkDeviceMemory> m_deviceMemories;
	std::vector<VkBuffer> m_computeDataBuffers;


	// Commands
	VkCommandPool m_commandPool;
	std::vector<VkCommandBuffer> m_commandBuffers;
	std::vector<VkCommandBuffer> m_computeCommandBuffers;

	// Rendering / Presenting
	std::vector<VkSemaphore> m_computeReadySemaphores;
	std::vector<VkSemaphore> m_imageAvailableSemaphores;
	std::vector<VkSemaphore> m_renderFinishedSemaphores;
	std::vector<VkFence> m_inFlightFences;
	std::vector<VkFence> m_imagesInFlight;
	size_t m_currentFrame = 0;


	// Scene data
	std::unique_ptr<Scene> m_scene;
};
