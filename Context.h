#pragma once

#define NOMINMAX
#define VK_USE_PLATFORM_WIN32_KHR

#include <windows.h>
#include <vulkan/vulkan.h>
#include <vector>
#include <optional>

// ── Constants ─────────────────────────────────────────────────────────────────
const uint32_t WIDTH = 1400;
const uint32_t HEIGHT = 800;
const int MAX_FRAMES_IN_FLIGHT = 2;

typedef struct VmaAllocator_T* VmaAllocator;

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

// ── Structs ───────────────────────────────────────────────────────────────────

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR        capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR>   presentModes;
};

struct Context {
    HWND                     hwnd;
    VkInstance               instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkSurfaceKHR             surface;
    VkPhysicalDevice         physicalDevice = VK_NULL_HANDLE;
    VkDevice                 device;
    VkQueue                  graphicsQueue;
    VkQueue                  presentQueue;
    VkSwapchainKHR           swapChain;
    std::vector<VkImage>     swapChainImages;
    VkFormat                 swapChainImageFormat;
    VkExtent2D               swapChainExtent;
    std::vector<VkImageView> swapChainImageViews;

    VkImage                  depthImage;
    VkFormat                 depthFormat{ VK_FORMAT_UNDEFINED };
    VkImageView              depthImageView;

    VmaAllocator             allocator;

    void init(HWND hwnd, HINSTANCE hinstance);
    ~Context();

private:
    // Instance & device setup
    bool                        checkValidationLayerSupport();
    std::vector<const char*>    getRequiredExtensions();
    void                        populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
    void                        setupDebugMessenger();
    void                        createInstance();
    void                        createSurface(HINSTANCE hinstance);

    // Physical & logical device
    bool                        isDeviceSuitable(VkPhysicalDevice device);
    bool                        checkDeviceExtensionSupport(VkPhysicalDevice device);
    SwapChainSupportDetails     querySwapChainSupport(VkPhysicalDevice physicalDevice);
    void                        pickPhysicalDevice();
    void                        createLogicalDevice();

    // Swapchain
    VkSurfaceFormatKHR          chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR            chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D                  chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    void                        createSwapChain();
    void                        createImageViews();
    void                        cleanupSwapChain();
    void                        createDepthImageView();

public:
    void                        recreateSwapChain();
    QueueFamilyIndices          findQueueFamilies(VkPhysicalDevice device);
};

// ── Forward declarations ───────────────────────────────────────────────────────

// Debug utils
VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
void     DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);