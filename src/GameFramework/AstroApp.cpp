#include <GameFramework/AstroApp.h>

#include <iostream>
#include <optional>
#include <set>
#include <stdexcept>

#include <GameFramework/QueueFamilyIndices.h>
#include <GameFramework/SwapchainHelpers.h>
#include <Helpers/FileHelpers.h>
#include <Helpers/VulkanHelpers.h>

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

constexpr int8_t MAX_FRAMES_IN_FLIGHT = 2;

#pragma region Helpers

QueueFamilyIndices FindQueueFamilies( VkPhysicalDevice device, VkSurfaceKHR surface )
{
	QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties( device, &queueFamilyCount, nullptr );

	std::vector<VkQueueFamilyProperties> queueFamilies( queueFamilyCount );
	vkGetPhysicalDeviceQueueFamilyProperties( device, &queueFamilyCount, queueFamilies.data() );

	int i = 0;
	for( const auto& queueFamily : queueFamilies )
	{
		if( queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT )
		{
			indices.graphicsFamily = i;

			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR( device, i, surface, &presentSupport );
			if( presentSupport )
			{
				indices.presentFamily = i;
			}
		}

		if( queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT )
		{
			indices.computeFamily = i;
		}

		if( indices.IsComplete() )
		{
			break;
		}

		i++;
	}

	return indices;
}

bool CheckDeviceExtensionSupport( VkPhysicalDevice device )
{

	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties( device, nullptr, &extensionCount, nullptr );
	std::vector<VkExtensionProperties> availableExtensions( extensionCount );
	vkEnumerateDeviceExtensionProperties( device, nullptr, &extensionCount, availableExtensions.data() );

	std::set<std::string> requiredExtensions( Required_Device_Extensions.begin(), Required_Device_Extensions.end() );

	for( const auto& extension : availableExtensions )
	{
		requiredExtensions.erase( extension.extensionName );
	}

	return requiredExtensions.empty();
}

SwapChainSupportDetails QuerySwapChainSupport( VkPhysicalDevice device, VkSurfaceKHR surface )
{
	SwapChainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR( device, surface, &details.capabilities );

	// Check format support
	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR( device, surface, &formatCount, nullptr );
	if( formatCount != 0 )
	{
		details.formats.resize( formatCount );
		vkGetPhysicalDeviceSurfaceFormatsKHR( device, surface, &formatCount, details.formats.data() );
	}

	// Check presentation modes
	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR( device, surface, &presentModeCount, nullptr );
	if( presentModeCount != 0 )
	{
		details.presentModes.resize( presentModeCount );
		vkGetPhysicalDeviceSurfacePresentModesKHR( device, surface, &presentModeCount, details.presentModes.data() );
	}

	return details;
}

VkShaderModule CreateShaderModule( const std::vector<char>& code, VkDevice device )
{
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>( code.data() );

	VkShaderModule shaderModule;
	if( vkCreateShaderModule( device, &createInfo, nullptr, &shaderModule ) != VK_SUCCESS )
	{
		throw std::runtime_error( "failed to create shader module!" );
	}

	return shaderModule;
}


uint32_t FindCompatibleMemoryTypeIndex( VkPhysicalDeviceMemoryProperties memoryProperties, const VkDeviceSize memorySize )
{
	for( uint32_t k = 0; k < memoryProperties.memoryTypeCount; k++ )
	{
		const VkMemoryType memoryType = memoryProperties.memoryTypes[k];
		if( ( VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT & memoryType.propertyFlags )
			&& ( VK_MEMORY_PROPERTY_HOST_COHERENT_BIT & memoryType.propertyFlags )
			&& ( memorySize < memoryProperties.memoryHeaps[memoryType.heapIndex].size ) )
		{
			return k;
		}
	}

	throw std::runtime_error( "failed to find memory type for requested memory size" );
	return 0;
}

#pragma endregion //Helpers


void AstroApp::Run()
{
	InitWindow();
	InitVulkan();

	LoadScene();
	MainLoop();
	Shutdown();
}

void AstroApp::InitWindow()
{
	glfwInit();
	glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API ); // Tell glfw to not create an openGL context
	glfwWindowHint( GLFW_RESIZABLE, GLFW_FALSE ); // disable resizing for now

	m_window = glfwCreateWindow( WIDTH, HEIGHT, "Astro", nullptr, nullptr );
}

void AstroApp::InitVulkan()
{
	CheckExtensions();
	CreateVkInstance();
	SetupDebugMessenger();
	CreateSurface();
	PickGPU();
	CreateVkLogicalDevice();
	CreateSwapchain();
	CreateImageViews();
	CreateRenderPass();
	CreateGraphicsPipeline();
	CreateComputePipeline();
	CreateFramebuffers();
	CreateCommandPool();
	CreateCommandBuffers();
	CreateComputeCommandBuffers();
	CreateSemaphores();
}

void AstroApp::CreateVkInstance()
{
#ifndef NDEBUG
	if( EnableValidationLayers && !CheckValidationLayers() )
	{
		throw std::runtime_error( "not all required validation layers were available!" );
	}
#endif

	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Astro";
	appInfo.applicationVersion = VK_MAKE_VERSION( 1, 0, 0 );
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION( 1, 0, 0 );
	appInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
	if( EnableValidationLayers )
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>( Validation_Layers.size() );
		createInfo.ppEnabledLayerNames = Validation_Layers.data();

		PopulateDebugMessengerCreateInfo( debugCreateInfo );
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
	}
	else
	{
		createInfo.enabledLayerCount = 0;
		createInfo.pNext = nullptr;
	}

	auto extensions = GetRequiredExtensions();
	createInfo.enabledExtensionCount = static_cast<uint32_t>( extensions.size() );
	createInfo.ppEnabledExtensionNames = extensions.data();

	if( vkCreateInstance( &createInfo, nullptr, &m_instance ) != VK_SUCCESS )
	{
		throw std::runtime_error( "failed to create instance!" );
	}
}

void AstroApp::LoadScene()
{
	m_scene = std::make_unique<Scene>();
	m_scene->Load();
}

void AstroApp::MainLoop()
{
	while( !glfwWindowShouldClose( m_window ) )
	{
		glfwPollEvents();

		vkWaitForFences( m_logicalDevice, 1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX );

		//Tell vulkan which semaphore to signal, when image is acquired
		uint32_t imageIndex;
		vkAcquireNextImageKHR( m_logicalDevice, m_swapChain, UINT64_MAX, m_imageAvailableSemaphores[m_currentFrame], VK_NULL_HANDLE, &imageIndex );


		ComputeFrame( imageIndex );
		DrawFrame( imageIndex );
	}

	//Wait till not busy (so we're not in the middle of rendering something when trying to destroy the resources)
	vkDeviceWaitIdle( m_logicalDevice );
}

//Acquire an image from the swap chain
//Execute the command buffer with that image as attachment in the framebuffer
//Return the image to the swap chain for presentation
void AstroApp::DrawFrame( uint32_t imageIndex )
{
	if( m_imagesInFlight[imageIndex] != VK_NULL_HANDLE )
	{
		// Check if a previous frame is using this image( i.e.there is its fence to wait on )
		vkWaitForFences( m_logicalDevice, 1, &m_imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX );
	}
	m_imagesInFlight[imageIndex] = m_inFlightFences[m_currentFrame];


	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	// Setup which semaphore we're waiting to be signaled before we can draw to the image & at which stage of the pipeline.
	VkSemaphore waitSemaphores[] = { m_computeReadySemaphores[m_currentFrame] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_commandBuffers[imageIndex];

	// which semaphore to signal once rendering is done
	VkSemaphore signalSemaphores[] = { m_renderFinishedSemaphores[m_currentFrame] }; // unused?
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	vkResetFences( m_logicalDevice, 1, &m_inFlightFences[m_currentFrame] );

	if( vkQueueSubmit( m_graphicsQueue, 1, &submitInfo, m_inFlightFences[m_currentFrame] ) != VK_SUCCESS )
	{
		throw std::runtime_error( "failed to submit draw command buffer!" );
	}

	// Present
	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;
	VkSwapchainKHR swapChains[] = { m_swapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr; // Optional

	vkQueuePresentKHR( m_presentQueue, &presentInfo );

	m_currentFrame = ( m_currentFrame + 1 ) % MAX_FRAMES_IN_FLIGHT;
}

void AstroApp::ComputeFrame( uint32_t imageIndex )
{
	vkWaitForFences( m_logicalDevice, 1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX );

	//m_scene->ComputeFrame();
	//SetComputeCommands( &m_computeCommandBuffer[imageIndex], /*delegate for scene to fill commands*/ );
	SetComputeCommandsToBuffer( m_computeCommandBuffers[imageIndex] );


	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { m_imageAvailableSemaphores[m_currentFrame] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_computeCommandBuffers[imageIndex];

	// which semaphore to signal once compute is done
	VkSemaphore signalSemaphores[] = { m_computeReadySemaphores[m_currentFrame] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;


	//vkResetFences( m_logicalDevice, 1, &m_inFlightFences[m_currentFrame] );
	//Hoping that the graphics fence is enough to sync the compute pass as well... ie, no fence reset or argument in Submit
	if( vkQueueSubmit( m_computeQueue, 1, &submitInfo, VK_NULL_HANDLE ) != VK_SUCCESS )
	{
		throw std::runtime_error( "failed to submit compute command buffer!" );
	}
}

void AstroApp::Shutdown()
{
	m_scene.reset();

	//--------------------------------
	// VULKAN
	//--------------------------------
	for( auto bufferMemory : m_deviceMemories )
	{
		vkFreeMemory( m_logicalDevice, bufferMemory, nullptr );
	}

	for( size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++ )
	{
		vkDestroySemaphore( m_logicalDevice, m_renderFinishedSemaphores[i], nullptr );
		vkDestroySemaphore( m_logicalDevice, m_imageAvailableSemaphores[i], nullptr );
		vkDestroySemaphore( m_logicalDevice, m_computeReadySemaphores[i], nullptr );
		vkDestroyFence( m_logicalDevice, m_inFlightFences[i], nullptr );
	}

	vkDestroyCommandPool( m_logicalDevice, m_commandPool, nullptr );

	for( auto framebuffer : m_swapChainFramebuffers )
	{
		vkDestroyFramebuffer( m_logicalDevice, framebuffer, nullptr );
	}

	vkDestroyPipeline( m_logicalDevice, m_computePipeline, nullptr );
	vkDestroyPipeline( m_logicalDevice, m_graphicsPipeline, nullptr );
	vkDestroyPipelineLayout( m_logicalDevice, m_computePipelineLayout, nullptr );
	vkDestroyPipelineLayout( m_logicalDevice, m_graphicsPipelineLayout, nullptr );
	vkDestroyRenderPass( m_logicalDevice, m_renderPass, nullptr );

	for( auto imageView : m_swapChainImageViews )
	{
		vkDestroyImageView( m_logicalDevice, imageView, nullptr );
	}

	vkDestroySwapchainKHR( m_logicalDevice, m_swapChain, nullptr );
	vkDestroyDevice( m_logicalDevice, nullptr );
	vkDestroySurfaceKHR( m_instance, m_surface, nullptr );
	if( EnableValidationLayers )
	{
		VulkanHelpers::DestroyDebugUtilsMessengerEXT( m_instance, m_debugMessenger, nullptr );
	}
	vkDestroyInstance( m_instance, nullptr );
	glfwDestroyWindow( m_window );
	glfwTerminate();
}


void AstroApp::SetComputeCommandsToBuffer( VkCommandBuffer& commandBuffer )
{
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = 0; // Optional
	beginInfo.pInheritanceInfo = nullptr; // Optional

	if( vkBeginCommandBuffer( commandBuffer, &beginInfo ) != VK_SUCCESS )
	{
		throw std::runtime_error( "failed to begin recording compute command buffer!" );
	}

	vkCmdBindPipeline( commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_computePipeline );

	glm::vec3 dispatchGroupSize = glm::vec3( 1, 1, 1 );
	vkCmdDispatch( commandBuffer, dispatchGroupSize.x, dispatchGroupSize.y, dispatchGroupSize.z );

	if( vkEndCommandBuffer( commandBuffer ) != VK_SUCCESS )
	{
		throw std::runtime_error( "failed to record command buffer!" );
	}
}


void AstroApp::PopulateDebugMessengerCreateInfo( VkDebugUtilsMessengerCreateInfoEXT& createInfo )
{
	createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = DebugCallback;
}

void AstroApp::SetupDebugMessenger()
{
	if( !EnableValidationLayers ) { return; }

	VkDebugUtilsMessengerCreateInfoEXT createInfo{};
	PopulateDebugMessengerCreateInfo( createInfo );
	createInfo.pUserData = nullptr; // Optional

	if( VulkanHelpers::CreateDebugUtilsMessengerEXT( m_instance, &createInfo, nullptr, &m_debugMessenger ) != VK_SUCCESS )
	{
		throw std::runtime_error( "failed to set up debug messenger!" );
	}
}

void AstroApp::CheckExtensions()
{
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties( nullptr, &extensionCount, nullptr );
	std::vector<VkExtensionProperties> extensions( extensionCount );
	vkEnumerateInstanceExtensionProperties( nullptr, &extensionCount, extensions.data() );
	std::cout << "available extensions:\n";

	for( const auto& extension : extensions )
	{
		std::cout << '\t' << extension.extensionName << '\n';
	}
}

std::vector<const char*> AstroApp::GetRequiredExtensions()
{
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions( &glfwExtensionCount );
	std::vector<const char*> extensions( glfwExtensions, glfwExtensions + glfwExtensionCount );
	if( EnableValidationLayers )
	{
		extensions.push_back( VK_EXT_DEBUG_UTILS_EXTENSION_NAME );
	}

	return extensions;
}

#ifndef NDEBUG
bool AstroApp::CheckValidationLayers()
{
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties( &layerCount, nullptr );

	std::vector<VkLayerProperties> availableLayers( layerCount );
	vkEnumerateInstanceLayerProperties( &layerCount, availableLayers.data() );

	bool foundAllRequiredValidationLayers = true;
	for( const auto& requiredValidationLayer : Validation_Layers )
	{
		for( const auto& availableValidationLayer : availableLayers )
		{
			if( strcmp( requiredValidationLayer, availableValidationLayer.layerName ) )
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


VKAPI_ATTR VkBool32 VKAPI_CALL AstroApp::DebugCallback(
  VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
  VkDebugUtilsMessageTypeFlagsEXT messageType,
  const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
  void* pUserData )
{
	std::cerr << "Validation layer: " << pCallbackData->pMessage << std::endl;

	return VK_FALSE;
}

void AstroApp::PickGPU()
{

	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices( m_instance, &deviceCount, nullptr );
	if( deviceCount == 0 )
	{
		throw std::runtime_error( "failed to find GPUs with Vulkan support!" );
	}
	std::vector<VkPhysicalDevice> devices( deviceCount );
	vkEnumeratePhysicalDevices( m_instance, &deviceCount, devices.data() );

	for( const auto& device : devices )
	{
		// Find first suitablel GPU physical device
		if( IsGPUSuitable( device ) )
		{
			m_physicalDevice = device;
			break;
		}
	}

	if( m_physicalDevice == VK_NULL_HANDLE )
	{
		throw std::runtime_error( "None of the GPU's available are suitable for this application" );
	}
}

bool AstroApp::IsGPUSuitable( VkPhysicalDevice device )
{
	//name, type and supported Vulkan version
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties( device, &deviceProperties );

	//support for optional features like texture compression, 64 bit floats and multi viewport rendering (VR)
	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceFeatures( device, &deviceFeatures );

	const bool deviceSupportsRequiredFeatures =
	  deviceProperties.limits.maxComputeSharedMemorySize > 0;

	const bool deviceSupportsRequiredExtensions = CheckDeviceExtensionSupport( device );

	bool swapChainSupported = false;
	if( deviceSupportsRequiredExtensions )
	{
		const SwapChainSupportDetails swapchainSupportDetails = QuerySwapChainSupport( device, m_surface );
		swapChainSupported = swapchainSupportDetails.formats.empty() == false && swapchainSupportDetails.presentModes.empty() == false;
	}

	return deviceSupportsRequiredFeatures
		   && deviceSupportsRequiredExtensions
		   && FindQueueFamilies( device, m_surface ).IsComplete()
		   && swapChainSupported;
}

void AstroApp::CreateVkLogicalDevice()
{
	QueueFamilyIndices indices = FindQueueFamilies( m_physicalDevice, m_surface );

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = {
		indices.graphicsFamily.value(),
		indices.presentFamily.value(),
		indices.computeFamily.value()
	};

	float queuePriority = 1.0f;
	for( uint32_t queueFamily : uniqueQueueFamilies )
	{
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back( queueCreateInfo );
	}
	//Declare which features we will be using (these should've been checked in "IsGPUSuitable")


	//TODO: Add required features
	VkPhysicalDeviceFeatures deviceFeatures{};

	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.queueCreateInfoCount = static_cast<uint32_t>( queueCreateInfos.size() );
	createInfo.pEnabledFeatures = &deviceFeatures;

	createInfo.enabledExtensionCount = static_cast<uint32_t>( Required_Device_Extensions.size() );
	createInfo.ppEnabledExtensionNames = Required_Device_Extensions.data();

	if( EnableValidationLayers )
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>( Validation_Layers.size() );
		createInfo.ppEnabledLayerNames = Validation_Layers.data();
	}
	else
	{
		createInfo.enabledLayerCount = 0;
	}

	if( vkCreateDevice( m_physicalDevice, &createInfo, nullptr, &m_logicalDevice ) != VK_SUCCESS )
	{
		throw std::runtime_error( "failed to create logical device!" );
	}

	vkGetDeviceQueue( m_logicalDevice, indices.graphicsFamily.value(), 0, &m_graphicsQueue );
	vkGetDeviceQueue( m_logicalDevice, indices.presentFamily.value(), 0, &m_presentQueue );
	vkGetDeviceQueue( m_logicalDevice, indices.computeFamily.value(), 0, &m_computeQueue );
}

void AstroApp::CreateSurface()
{
	if( glfwCreateWindowSurface( m_instance, m_window, nullptr, &m_surface ) != VK_SUCCESS )
	{
		throw std::runtime_error( "failed to create window surface!" );
	}
}

void AstroApp::CreateSwapchain()
{
	SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport( m_physicalDevice, m_surface );

	VkSurfaceFormatKHR surfaceFormat = SwapchainHelpers::ChooseSwapSurfaceFormat( swapChainSupport.formats );
	VkPresentModeKHR presentMode = SwapchainHelpers::ChooseSwapPresentMode( swapChainSupport.presentModes );
	VkExtent2D extent = SwapchainHelpers::ChooseSwapExtent( m_window, swapChainSupport.capabilities );

	// Keep for later reference
	m_swapChainImageFormat = surfaceFormat.format;
	m_swapChainExtent = extent;

	// minimum required images for the swapchain to function
	// +1 : to avoid stalling whilst waiting for the driver to complete internal operations
	// before we can acquire another image to render to.
	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

	// Clamp image count to maximum
	//    0 is special case meaning no maximum is defined
	if( swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount )
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


	QueueFamilyIndices indices = FindQueueFamilies( m_physicalDevice, m_surface );
	uint32_t queueFamilyIndices[] = {
		indices.graphicsFamily.value(),
		indices.presentFamily.value(),
		indices.computeFamily.value()
	};

	if( indices.graphicsFamily != indices.presentFamily )
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

	if( vkCreateSwapchainKHR( m_logicalDevice, &createInfo, nullptr, &m_swapChain ) != VK_SUCCESS )
	{
		throw std::runtime_error( "failed to create swap chain!" );
	}

	// Get handles to the swapchain images
	vkGetSwapchainImagesKHR( m_logicalDevice, m_swapChain, &imageCount, nullptr );
	m_swapChainImages.resize( imageCount );
	vkGetSwapchainImagesKHR( m_logicalDevice, m_swapChain, &imageCount, m_swapChainImages.data() );
}

void AstroApp::CreateImageViews()
{
	m_swapChainImageViews.resize( m_swapChainImages.size() );
	for( size_t i = 0; i < m_swapChainImages.size(); i++ )
	{
		VkImageViewCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = m_swapChainImages[i];
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = m_swapChainImageFormat;

		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; //Color render target
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		if( vkCreateImageView( m_logicalDevice, &createInfo, nullptr, &m_swapChainImageViews[i] ) != VK_SUCCESS )
		{
			throw std::runtime_error( "failed to create image views!" );
		}
	}
}


void AstroApp::CreateRenderPass()
{
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = m_swapChainImageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	// Stencil - not used
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	// Note : images need to be transitioned to specific layouts that are suitable for the operation that they're going to be involved in next.
	// eg:
	// VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: Images used as color attachment
	// VK_IMAGE_LAYOUT_PRESENT_SRC_KHR: Images to be presented in the swap chain
	// VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL: Images to be used as destination for a memory copy operation


	// Sub-Passes
	// Subpasses are subsequent rendering operations that depend on the contents of framebuffers in previous passes,
	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0; // Attachement index 0 (ie: layout(location = 0) out vec4 outColor)
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	// Create!
	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if( vkCreateRenderPass( m_logicalDevice, &renderPassInfo, nullptr, &m_renderPass ) != VK_SUCCESS )
	{
		throw std::runtime_error( "failed to create render pass!" );
	}
}


void AstroApp::CreateGraphicsPipeline()
{

	// Load simple shader
	auto simpleShaderVertCode = FileHelpers::ReadFile( "src/Resources/Shaders/SimpleShader.vert.spirv" );
	auto simpleShaderFragCode = FileHelpers::ReadFile( "src/Resources/Shaders/SimpleShader.frag.spirv" );

	if( simpleShaderVertCode.size() == 0 )
	{
		throw std::runtime_error( "Simple Shader (vert) file size is 0!" );
	}

	if( simpleShaderFragCode.size() == 0 )
	{
		throw std::runtime_error( "Simple Shader (frag) file size is 0!" );
	}


	VkShaderModule simpleShaderVertModule = CreateShaderModule( simpleShaderVertCode, m_logicalDevice );
	VkShaderModule simpleShaderFragModule = CreateShaderModule( simpleShaderFragCode, m_logicalDevice );

	// Vertex shader stage
	VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = simpleShaderVertModule;
	vertShaderStageInfo.pName = "main";

	// Fragment shader stage
	VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = simpleShaderFragModule;
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	// Vertex shader input
	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 0;
	vertexInputInfo.pVertexBindingDescriptions = nullptr; // Optional
	vertexInputInfo.vertexAttributeDescriptionCount = 0;
	vertexInputInfo.pVertexAttributeDescriptions = nullptr; // Optional

	// Input Assembly info
	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	// Viewports define the transformation from the image to the framebuffer,
	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)m_swapChainExtent.width;
	viewport.height = (float)m_swapChainExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	// Scissor rectangles define in which regions pixels will actually be stored
	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = m_swapChainExtent;

	// Viewport creation info
	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;


	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE; //VK_TRUE: fragments that are beyond the near and far planes are clamped to them as opposed to discarding them
	rasterizer.rasterizerDiscardEnable = VK_FALSE; // needs to be set to false or the rasterizer is disabled
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL; //note VK_POLYGON_MODE_LINE would be wireframe - requires a GPU feature though
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT; // Cull backfaces
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE; // order of vertices to define front facing face
	// depth bias can be used for shadowmapping, but not needed here
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f; // Optional
	rasterizer.depthBiasClamp = 0.0f; // Optional
	rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

	// Multi sampling - disabled for now
	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f; // Optional
	multisampling.pSampleMask = nullptr; // Optional
	multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
	multisampling.alphaToOneEnable = VK_FALSE; // Optional

	// Depth & stencil info - no need for this yet
	//VkPipelineDepthStencilStateCreateInfo

	// Blend mode
	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE; // Disabled atm
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f; // Optional
	colorBlending.blendConstants[1] = 0.0f; // Optional
	colorBlending.blendConstants[2] = 0.0f; // Optional
	colorBlending.blendConstants[3] = 0.0f; // Optional


	// Pipeline Layout (uniforms)
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 0; // Optional
	pipelineLayoutInfo.pSetLayouts = nullptr; // Optional
	pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
	pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

	if( vkCreatePipelineLayout( m_logicalDevice, &pipelineLayoutInfo, nullptr, &m_graphicsPipelineLayout ) != VK_SUCCESS )
	{
		throw std::runtime_error( "failed to create pipeline layout!" );
	}

	// Create pipeline
	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = nullptr; // Optional
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = nullptr; // Optional
	pipelineInfo.layout = m_graphicsPipelineLayout;
	pipelineInfo.renderPass = m_renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
	pipelineInfo.basePipelineIndex = -1; // Optional

	if( vkCreateGraphicsPipelines( m_logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipeline ) != VK_SUCCESS )
	{
		throw std::runtime_error( "failed to create graphics pipeline!" );
	}


	// Shader modules are loaded into the graphics pipeline, so we can destroy the local variables since they're not referenced directly
	vkDestroyShaderModule( m_logicalDevice, simpleShaderFragModule, nullptr );
	vkDestroyShaderModule( m_logicalDevice, simpleShaderVertModule, nullptr );
}


void AstroApp::CreateComputePipeline()
{
	// Load simple compute shader
	auto simpleShaderComputeCode = FileHelpers::ReadFile( "src/Resources/Shaders/SimpleShader.comp.spirv" );

	if( simpleShaderComputeCode.size() == 0 )
	{
		throw std::runtime_error( "Simple Shader (compute) file size is 0!" );
	}

	VkShaderModule simpleShaderComputeModule = CreateShaderModule( simpleShaderComputeCode, m_logicalDevice );

	VkPipelineShaderStageCreateInfo shaderStageInfo{};
	shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStageInfo.pNext = nullptr;
	shaderStageInfo.flags = 0;
	shaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	shaderStageInfo.module = simpleShaderComputeModule;
	shaderStageInfo.pName = "main";
	shaderStageInfo.pSpecializationInfo = nullptr;

	// Pipeline Layout (uniforms)
	//-----TODO : update the layout to add the example simple compute shader's input & output textures
	//????????? is this all correct?
	VkDescriptorSetLayoutBinding layoutBindingZero{};
	layoutBindingZero.binding = 0;
	layoutBindingZero.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	layoutBindingZero.descriptorCount = 1;
	layoutBindingZero.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	layoutBindingZero.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding layoutBindingOne{};
	layoutBindingOne.binding = 1;
	layoutBindingOne.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	layoutBindingOne.descriptorCount = 1;
	layoutBindingOne.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	layoutBindingOne.pImmutableSamplers = nullptr;

	std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings{};
	descriptorSetLayoutBindings.push_back( layoutBindingZero );
	descriptorSetLayoutBindings.push_back( layoutBindingOne );

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
	descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorSetLayoutCreateInfo.pNext = nullptr;
	descriptorSetLayoutCreateInfo.flags = 0;
	descriptorSetLayoutCreateInfo.bindingCount = descriptorSetLayoutBindings.size();
	descriptorSetLayoutCreateInfo.pBindings = descriptorSetLayoutBindings.data();


	VkDescriptorSetLayout descriptorSetLayout{};
	if( vkCreateDescriptorSetLayout( m_logicalDevice, &descriptorSetLayoutCreateInfo, nullptr, &descriptorSetLayout ) != VK_SUCCESS )
	{
		throw std::runtime_error( "failed to create compute pipeline descriptor layout!" );
	}

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.pNext = nullptr;
	pipelineLayoutInfo.flags = 0;
	pipelineLayoutInfo.setLayoutCount = 1; // Optional
	pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout; // Optional
	pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
	pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional


	if( vkCreatePipelineLayout( m_logicalDevice, &pipelineLayoutInfo, nullptr, &m_computePipelineLayout ) != VK_SUCCESS )
	{
		throw std::runtime_error( "failed to create pipeline layout!" );
	}


	VkComputePipelineCreateInfo computePipelineInfo{};
	computePipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	computePipelineInfo.pNext = nullptr;
	computePipelineInfo.flags = 0;
	computePipelineInfo.stage = shaderStageInfo;
	computePipelineInfo.layout = m_computePipelineLayout;
	computePipelineInfo.basePipelineIndex = 0; // Optional
	computePipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional

	if( vkCreateComputePipelines( m_logicalDevice, VK_NULL_HANDLE, 1, &computePipelineInfo, nullptr, &m_computePipeline ) != VK_SUCCESS )
	{
		throw std::runtime_error( "failed to create compute pipeline!" );
	}

	// Shader module is loaded into the compute pipeline, so we can destroy the local variables since they're not referenced directly
	vkDestroyShaderModule( m_logicalDevice, simpleShaderComputeModule, nullptr );
}

void AstroApp::CreateFramebuffers()
{
	m_swapChainFramebuffers.resize( m_swapChainImageViews.size() );

	for( size_t i = 0; i < m_swapChainImageViews.size(); i++ )
	{
		VkImageView attachments[] = {
			m_swapChainImageViews[i]
		};

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = m_renderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = m_swapChainExtent.width;
		framebufferInfo.height = m_swapChainExtent.height;
		framebufferInfo.layers = 1;

		if( vkCreateFramebuffer( m_logicalDevice, &framebufferInfo, nullptr, &m_swapChainFramebuffers[i] ) != VK_SUCCESS )
		{
			throw std::runtime_error( "failed to create framebuffer!" );
		}
	}
}

void AstroApp::CreateCommandPool()
{
	QueueFamilyIndices queueFamilyIndices = FindQueueFamilies( m_physicalDevice, m_surface );

	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
	poolInfo.flags = 0; // Optional

	if( vkCreateCommandPool( m_logicalDevice, &poolInfo, nullptr, &m_commandPool ) != VK_SUCCESS )
	{
		throw std::runtime_error( "failed to create command pool!" );
	}
}

void AstroApp::CreateCommandBuffers()
{
	m_commandBuffers.resize( m_swapChainFramebuffers.size() );

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = m_commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)m_commandBuffers.size();

	if( vkAllocateCommandBuffers( m_logicalDevice, &allocInfo, m_commandBuffers.data() ) != VK_SUCCESS )
	{
		throw std::runtime_error( "failed to allocate graphics command buffers!" );
	}


	for( size_t i = 0; i < m_commandBuffers.size(); i++ )
	{
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0; // Optional
		beginInfo.pInheritanceInfo = nullptr; // Optional

		if( vkBeginCommandBuffer( m_commandBuffers[i], &beginInfo ) != VK_SUCCESS )
		{
			throw std::runtime_error( "failed to begin recording command buffer!" );
		}

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = m_renderPass;
		renderPassInfo.framebuffer = m_swapChainFramebuffers[i];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = m_swapChainExtent;

		VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		vkCmdBeginRenderPass( m_commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE );

		// Bind Graphics pipeline
		vkCmdBindPipeline( m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline );

		// Draw triangle
		vkCmdDraw( m_commandBuffers[i],
		  3, // Vertex count
		  1, // instance count
		  0, // offset vertex -> gl_VertexIndex
		  0 // offset instance index ->
		);

		vkCmdEndRenderPass( m_commandBuffers[i] );

		if( vkEndCommandBuffer( m_commandBuffers[i] ) != VK_SUCCESS )
		{
			throw std::runtime_error( "failed to record command buffer!" );
		}
	}
}


void AstroApp::CreateComputeCommandBuffers()
{
	// Allocate the command buffers
	m_computeCommandBuffers.resize( m_swapChainFramebuffers.size() );

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = m_commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)m_computeCommandBuffers.size();

	if( vkAllocateCommandBuffers( m_logicalDevice, &allocInfo, m_computeCommandBuffers.data() ) != VK_SUCCESS )
	{
		throw std::runtime_error( "failed to allocate compute command buffers!" );
	}


	// DATA SIZE
	const VkDeviceSize memorySize = sizeof( float ); // whatever size of memory we require
	QueueFamilyIndices indices = FindQueueFamilies( m_physicalDevice, m_surface );
	uint32_t queueFamilyIndices[] = {
		indices.computeFamily.value()
	};

	// Allocate Data Buffers
	m_computeDataBuffers.resize( 2 ); // input buffer & output buffer
	for( auto& computeDataBuffer : m_computeDataBuffers )
	{
		VkBufferCreateInfo bufferCreateInfo{};
		bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCreateInfo.pNext = nullptr;
		bufferCreateInfo.flags = 0;
		bufferCreateInfo.size = memorySize;
		bufferCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		bufferCreateInfo.queueFamilyIndexCount = 1;
		bufferCreateInfo.pQueueFamilyIndices = queueFamilyIndices;

		if( vkCreateBuffer( m_logicalDevice, &bufferCreateInfo, nullptr, &computeDataBuffer ) != VK_SUCCESS )
		{
			throw std::runtime_error( "failed to create compute data buffer!" );
		}
	}

	// Allocate the Memory backing the buffers
	VkPhysicalDeviceMemoryProperties memoryProperties{};
	vkGetPhysicalDeviceMemoryProperties( m_physicalDevice, &memoryProperties );

	m_deviceMemories.resize( m_swapChainFramebuffers.size() );

	std::vector<float> initialBufferDataValues{ 7.4f, 1.3f };
	for( uint32_t bufferIndex = 0; bufferIndex < m_computeCommandBuffers.size(); ++bufferIndex )
	{
		auto bufferMemory = m_deviceMemories[bufferIndex];
		uint32_t memoryTypeIndex = FindCompatibleMemoryTypeIndex( memoryProperties, memorySize );

		// found our memory type!
		VkMemoryAllocateInfo memoryAllocInfo{};
		memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memoryAllocInfo.pNext = nullptr;
		memoryAllocInfo.allocationSize = memorySize;
		memoryAllocInfo.memoryTypeIndex = memoryTypeIndex;

		if( vkAllocateMemory( m_logicalDevice, &memoryAllocInfo, nullptr, &bufferMemory ) != VK_SUCCESS )
		{
			throw std::runtime_error( "failed to allocate memory for compute buffers!" );
		}

		if( vkBindBufferMemory( m_logicalDevice, m_computeDataBuffers[bufferIndex], bufferMemory, 0 ) != VK_SUCCESS )
		{
			throw std::runtime_error( "failed to bind memory to compute buffer!" );
		}

		float* payload;
		void* payloadPtr = (void*)payload;
		if( vkMapMemory( m_logicalDevice, bufferMemory, 0, memorySize, 0, &payloadPtr ) != VK_SUCCESS )
		{
			throw std::runtime_error( "failed to map memory to compute buffer!" );
		}

		//for( uint32_t k = 0; k < memorySize / sizeof( int32_t ); k++ )
		for( uint32_t k = 0; k < 1; k++ )
		{
			payload[k] = initialBufferDataValues[bufferIndex];
		}
		vkUnmapMemory( m_logicalDevice, bufferMemory );
	}
}

void AstroApp::CreateSemaphores()
{
	m_computeReadySemaphores.resize( MAX_FRAMES_IN_FLIGHT );
	m_imageAvailableSemaphores.resize( MAX_FRAMES_IN_FLIGHT );
	m_renderFinishedSemaphores.resize( MAX_FRAMES_IN_FLIGHT );
	m_inFlightFences.resize( MAX_FRAMES_IN_FLIGHT );
	m_imagesInFlight.resize( m_swapChainImages.size(), VK_NULL_HANDLE );

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for( size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++ )
	{
		if( vkCreateSemaphore( m_logicalDevice, &semaphoreInfo, nullptr, &m_computeReadySemaphores[i] ) != VK_SUCCESS
			|| vkCreateSemaphore( m_logicalDevice, &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i] ) != VK_SUCCESS
			|| vkCreateSemaphore( m_logicalDevice, &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i] ) != VK_SUCCESS
			|| vkCreateFence( m_logicalDevice, &fenceInfo, nullptr, &m_inFlightFences[i] ) != VK_SUCCESS )
		{
			throw std::runtime_error( "failed to create synchronization objects for a frame!" );
		}
	}
}