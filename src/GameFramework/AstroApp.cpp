#include <GameFramework/AstroApp.h>

#include <stdexcept>
#include <vector>
#include <iostream>
#include <optional>

constexpr uint16_t WIDTH = 800;
constexpr uint16_t HEIGHT = 600;
const std::vector<const char*> Validation_Layers = {
    "VK_LAYER_KHRONOS_validation"
};
#ifdef NDEBUG
constexpr bool EnableValidationLayers = false;
#else
constexpr bool EnableValidationLayers = true;
#endif

namespace AstroAppSetupPrivates
{
	struct QueueFamilyIndices {
		std::optional<uint32_t> graphicsFamily;

		bool IsComplete() {
			return graphicsFamily.has_value();
		}
	};
}


void AstroApp::Run() 
{
	InitWindow();
	InitVulkan();
	MainLoop();
	Shutdown();
}

void AstroApp::InitWindow()
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // Tell glfw to not create an openGL context
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); // disable resizing for now

	m_window = glfwCreateWindow(WIDTH, HEIGHT, "Astro", nullptr, nullptr);
}

void AstroApp::InitVulkan() 
{
	CheckExtensions();
	CreateVulkanInstance();
	PickGPU();
}

void AstroApp::CreateVulkanInstance()
{
	#ifndef NDEBUG
	if(EnableValidationLayers && !CheckValidationLayers() )
	{
    	throw std::runtime_error("not all required validation layers were available!");
	}
#endif

	VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Astro";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
#ifndef NDEBUG
	createInfo.enabledLayerCount = static_cast<uint32_t>(Validation_Layers.size());
	createInfo.ppEnabledLayerNames = Validation_Layers.data();
#else
	createInfo.enabledLayerCount = 0;
#endif


	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	createInfo.enabledExtensionCount = glfwExtensionCount;
	createInfo.ppEnabledExtensionNames = glfwExtensions;
	createInfo.enabledLayerCount = 0;

	if( vkCreateInstance(&createInfo, nullptr, &m_instance) != VK_SUCCESS )
	{
		throw std::runtime_error("failed to create instance!");
	}
}

void AstroApp::MainLoop() 
{
	while (!glfwWindowShouldClose(m_window)) 
	{
        glfwPollEvents();
    }
}

void AstroApp::Shutdown()
{
	vkDestroyInstance(m_instance, nullptr);
	glfwDestroyWindow(m_window);
    glfwTerminate();
}

void AstroApp::CheckExtensions()
{
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
	std::cout << "available extensions:\n";

	for (const auto& extension : extensions) {
		std::cout << '\t' << extension.extensionName << '\n';
	}
}

#ifndef NDEBUG
bool AstroApp::CheckValidationLayers()
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	bool foundAllRequiredValidationLayers = true;
	for(const auto& requiredValidationLayer : Validation_Layers)
	{
		for(const auto& availableValidationLayer : availableLayers)
		{
			if( strcmp(requiredValidationLayer, availableValidationLayer.layerName) )
			{
				// Found required layer in available layers, skip to the next one
				break;
			}

			foundAllRequiredValidationLayers = false;
		}
	}

    return foundAllRequiredValidationLayers;
}
#endif


void AstroApp::PickGPU()
{

	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);
	if (deviceCount == 0) 
	{
		throw std::runtime_error("failed to find GPUs with Vulkan support!");
	}
	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

	for(const auto& device : devices)
	{
		// Find first suitablel GPU physical device
		if( IsGPUSuitable(device) )
		{
			m_physicalDevice = device;
			break;
		}
	}

	if( m_physicalDevice == VK_NULL_HANDLE)
	{
		throw std::runtime_error("None of the GPU's available are suitable for this application");
	}
}

AstroAppSetupPrivates::QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device)
{
	AstroAppSetupPrivates::QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	int i = 0;
	for (const auto& queueFamily : queueFamilies) {
		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphicsFamily = i;
		}

		if (indices.IsComplete()) {
			break;
		}

		i++;
	}

	return indices;
}

bool AstroApp::IsGPUSuitable(VkPhysicalDevice device)
{
	//name, type and supported Vulkan version
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(device, &deviceProperties);

	//support for optional features like texture compression, 64 bit floats and multi viewport rendering (VR)
	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

	const bool deviceSupportsRequiredFeatures = deviceProperties.limits.maxComputeSharedMemorySize > 0;


	return deviceSupportsRequiredFeatures && FindQueueFamilies(device).IsComplete();
}