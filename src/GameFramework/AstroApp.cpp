#include <GameFramework/AstroApp.h>

#include <stdexcept>
#include <vector>
#include <iostream>


constexpr uint16_t WIDTH = 800;
constexpr uint16_t HEIGHT = 600;
// const std::vector<const char*> Validation_Layers = {
//     "VK_LAYER_KHRONOS_validation"
// };
#ifdef NDEBUG
constexpr bool EnableValidationLayers = false;
#else
constexpr bool EnableValidationLayers = true;
#endif


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
#ifndef NDEBUG
	CheckValidationLayers();
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
	std::vector<int16_t> bleh;
	bleh.push_back(5);
	bleh.push_back(2);
	bleh.push_back(1);
	
	int test[] ={5,2,3};

    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    return false;
}
#endif