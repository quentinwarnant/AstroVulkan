#include <GameFramework/AstroApp.h>

#include <stdexcept>
#include <set>
#include <iostream>
#include <optional>

#include <GameFramework/QueueFamilyIndices.h>
#include <GameFramework/SwapchainHelpers.h>

constexpr uint16_t WIDTH = 800;
constexpr uint16_t HEIGHT = 600;
const std::vector<const char*> Validation_Layers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> Required_Device_Extensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifdef NDEBUG
constexpr bool EnableValidationLayers = false;
#else
constexpr bool EnableValidationLayers = true;
#endif

#pragma region Helpers

QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface)
{
	QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	int i = 0;
	for (const auto& queueFamily : queueFamilies) {
		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphicsFamily = i;

			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
			if( presentSupport)
			{
				indices.presentFamily = i;
			}
		}

		if (indices.IsComplete()) {
			break;
		}

		i++;
	}

	return indices;
}

bool CheckDeviceExtensionSupport(VkPhysicalDevice device) {
	
	uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(Required_Device_Extensions.begin(), Required_Device_Extensions.end());

    for (const auto& extension : availableExtensions) 
	{
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();

}

SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) 
{
    SwapChainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

	// Check format support
	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
	if (formatCount != 0) 
	{
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
	}

	// Check presentation modes
	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
	if (presentModeCount != 0) 
	{
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
	}

    return details;
}


#pragma endregion //Helpers


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
	CreateVkInstance();
	CreateSurface();
	PickGPU();
	CreateVkLogicalDevice();
	CreateSwapchain();
}

void AstroApp::CreateVkInstance()
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

	if(EnableValidationLayers)
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>(Validation_Layers.size());
		createInfo.ppEnabledLayerNames = Validation_Layers.data();
	}
	else
	{
		createInfo.enabledLayerCount = 0;
	}

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
	vkDestroySwapchainKHR(m_logicalDevice, m_swapChain, nullptr);
	vkDestroyDevice(m_logicalDevice,nullptr);
	vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
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

bool AstroApp::IsGPUSuitable(VkPhysicalDevice device)
{
	//name, type and supported Vulkan version
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(device, &deviceProperties);

	//support for optional features like texture compression, 64 bit floats and multi viewport rendering (VR)
	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

	const bool deviceSupportsRequiredFeatures = 
		deviceProperties.limits.maxComputeSharedMemorySize > 0;

	const bool deviceSupportsRequiredExtensions = CheckDeviceExtensionSupport(device);

	bool swapChainSupported = false;
	if( deviceSupportsRequiredExtensions )
	{
		const SwapChainSupportDetails swapchainSupportDetails = QuerySwapChainSupport(device, m_surface);
		swapChainSupported = swapchainSupportDetails.formats.empty() == false &&
							swapchainSupportDetails.presentModes.empty() == false;
	}

	return deviceSupportsRequiredFeatures
		&& deviceSupportsRequiredExtensions
		&& FindQueueFamilies(device, m_surface).IsComplete()
		&& swapChainSupported;
}

void AstroApp::CreateVkLogicalDevice()
{
	QueueFamilyIndices indices = FindQueueFamilies(m_physicalDevice, m_surface);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

	float queuePriority = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFamilies) 
	{
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = indices.graphicsFamily.value();
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}
	//Declare which features we will be using (these should've been checked in "IsGPUSuitable")


	//TODO: Add required features
	VkPhysicalDeviceFeatures deviceFeatures{};

	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pEnabledFeatures = &deviceFeatures;

	createInfo.enabledExtensionCount = static_cast<uint32_t>(Required_Device_Extensions.size());
	createInfo.ppEnabledExtensionNames = Required_Device_Extensions.data();

	if (EnableValidationLayers)
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>(Validation_Layers.size());
		createInfo.ppEnabledLayerNames = Validation_Layers.data();
	}
	else
	{
		createInfo.enabledLayerCount = 0;
	}

	if (vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_logicalDevice) != VK_SUCCESS) 
	{
		throw std::runtime_error("failed to create logical device!");
	}

	vkGetDeviceQueue(m_logicalDevice, indices.graphicsFamily.value(), 0, &m_graphicsQueue);
	vkGetDeviceQueue(m_logicalDevice, indices.presentFamily.value(), 0, &m_presentQueue);


}

void AstroApp::CreateSurface()
{
	if (glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
}

void AstroApp::CreateSwapchain()
{
	SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(m_physicalDevice, m_surface);

	VkSurfaceFormatKHR surfaceFormat = SwapchainHelpers::ChooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = SwapchainHelpers::ChooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = SwapchainHelpers::ChooseSwapExtent( m_window, swapChainSupport.capabilities);

	// Keep for later reference
	m_swapChainImageFormat = surfaceFormat.format;
	m_swapChainExtent = extent;

	// minimum required images for the swapchain to function
	// +1 : to avoid stalling whilst waiting for the driver to complete internal operations
	// before we can acquire another image to render to.
	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

	// Clamp image count to maximum
	//    0 is special case meaning no maximum is defined
	if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) 
	{
    	imageCount = swapChainSupport.capabilities.maxImageCount;
	}

	// Start creating swapchain
	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = m_surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1; //  is always 1 unless you are developing a stereoscopic 3D application
	// we're going to render directly to this swapchain image, for multi-stage rendering, eg: with post process effect, could use
	// VK_IMAGE_USAGE_TRANSFER_DST_BIT instead and use a memory operation to transfer the rendered image to a swap chain image.
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;


	QueueFamilyIndices indices = FindQueueFamilies(m_physicalDevice, m_surface);
	uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

	if (indices.graphicsFamily != indices.presentFamily)
	{
		//Images can be used across multiple queue families without explicit ownership transfers.
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	} 
	else
	{
		//An image is owned by one queue family at a time and ownership must be explicitly 
		//transferred before using it in another queue family. This option offers the best performance.
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0; // Optional
		createInfo.pQueueFamilyIndices = nullptr; // Optional
	}

	// rotation of swapchain (eg: 90 degrees) - we're keeping it to the current original transform
	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;

	// We don't want to blend with other windows
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE; // if another window is obstructing part of this window, we are happy to clip those pixels

	createInfo.oldSwapchain = VK_NULL_HANDLE; // if we want to reconstruct the swapchain on events like resizing, we'd use this

	if (vkCreateSwapchainKHR(m_logicalDevice, &createInfo, nullptr, &m_swapChain) != VK_SUCCESS) 
	{
		throw std::runtime_error("failed to create swap chain!");
	}

	// Get handles to the swapchain images
	vkGetSwapchainImagesKHR(m_logicalDevice, m_swapChain, &imageCount, nullptr);
	m_swapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(m_logicalDevice, m_swapChain, &imageCount, m_swapChainImages.data());
}
