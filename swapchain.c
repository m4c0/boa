#define _CRT_SECURE_NO_WARNINGS
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __APPLE__
#  include <TargetConditionals.h>
#  define VK_USE_PLATFORM_METAL_EXT
#endif

#if !TARGET_OS_IPHONE
#  define VOLK_IMPLEMENTATION
#  include "volk.h"
#endif

// not really needed. I'm using because it enables Vim's ctrl-n
#include "Vulkan-Headers/include/vulkan/vulkan_core.h"

#ifdef __APPLE__
#  include "Vulkan-Headers/include/vulkan/vulkan_metal.h"
#endif

// Covers a 4:1 screen, as if such thing will ever exist
#define MAX_CELLS (24 * 24 * 4)

#define VBUF_SIZE MAX_CELLS * sizeof(storage_t)

typedef struct storage {
  float first_seen;
  float seen;
} storage_t;

typedef struct ivec2 {
  int x, y;
} ivec2_t;
struct upc {
  float aspect;
  float time;
  float dead_at;
  float pad;
  float grid_width;
  float grid_height;
  ivec2_t food;
  ivec2_t party;
  float party_start;
} g_upc = {
  .grid_width = 24,
  .grid_height = 24,
  .food = { 10000, 10000 },
  .party = { 10000, 10000 },
  .party_start = -1,
};

#define MAX_SWAPCHAIN_IMAGES 8
typedef struct vlk_swc {
  VkFramebuffer   fb  [MAX_SWAPCHAIN_IMAGES];
  VkImageView     iv  [MAX_SWAPCHAIN_IMAGES];
  VkSwapchainKHR  swc;
} vlk_swc_t;

static vlk_swc_t vlk_swc     = {0};
static vlk_swc_t vlk_swc_old = {0};

static VkCommandBuffer vlk_cb      [MAX_SWAPCHAIN_IMAGES];
static VkImage         vlk_swc_img [MAX_SWAPCHAIN_IMAGES];

#define MAX_INFLIGHTS 3
static VkFence     vlk_fence        [MAX_INFLIGHTS];
static VkSemaphore vlk_sema_img     [MAX_INFLIGHTS];
static VkSemaphore vlk_sema_present [MAX_INFLIGHTS];
static unsigned    vlk_cur_inflight;

static VkCommandPool      vlk_cpool;
static VkDevice           vlk_dev;
static VkExtent2D         vlk_ext = { 300, 300 };
static VkInstance         vlk_ins;
static VkPhysicalDevice   vlk_pd;
static VkQueue            vlk_q;
static VkRenderPass       vlk_rp;
static VkSurfaceFormatKHR vlk_surf_fmt;
static VkSurfaceKHR       vlk_surf;
static unsigned           vlk_qf;
static unsigned           vlk_swc_count;

static VkBuffer              vlk_vbuf;
static VkDescriptorPool      vlk_dpool;
static VkDescriptorSetLayout vlk_dsl;
static VkDescriptorSet       vlk_dset;
static VkDeviceMemory        vlk_vmem;
static VkPipelineLayout      vlk_pl;
static VkPipeline            vlk_ppl;

#ifdef __APPLE__
CAMetalLayer * vlk_metal_layer();
#endif

void vlk_log(int r, const char * msg);
static void vlk_check(VkResult r, const char * msg) {
  if (r == VK_SUCCESS) return;
  vlk_log(r, msg);
  exit(1);
}
#define _(X) vlk_check((X), #X)

static void vlk_create_instance() {
  const char * ext[] = {
    0, // Platform-specific
    VK_KHR_SURFACE_EXTENSION_NAME,
    // Next two are only used by OSX
    VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
    VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
  };

  VkApplicationInfo app = {
    .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
    .apiVersion = VK_API_VERSION_1_0,
  };
  VkInstanceCreateInfo info = (VkInstanceCreateInfo) {
    .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    .pApplicationInfo = &app,
    .ppEnabledExtensionNames = ext,
    .enabledExtensionCount = 2,
  };

#ifdef __APPLE__
  ext[0] = VK_EXT_METAL_SURFACE_EXTENSION_NAME;

#if !TARGET_OS_IPHONE
  // MoltenVK kinda requires this extension/flag. It works without it, but the
  // validation layer will complain.
  info.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
  info.enabledExtensionCount += 2;
#endif
#endif

  _(vkCreateInstance(&info, NULL, &vlk_ins));

#if !TARGET_OS_IPHONE
  volkLoadInstance(vlk_ins);
#endif
}

static void vlk_find_physical_device() {
  VkPhysicalDevice pd[16];
  uint32_t pdsz = 16;
  _(vkEnumeratePhysicalDevices(vlk_ins, &pdsz, pd));
  for (int i = 0; i < pdsz; i++) {
    VkQueueFamilyProperties qp[16];
    uint32_t qpsz = 16;
    vkGetPhysicalDeviceQueueFamilyProperties(pd[i], &qpsz, qp);
    for (vlk_qf = 0; vlk_qf < qpsz; vlk_qf++) {
      if ((qp[vlk_qf].queueFlags & VK_QUEUE_COMPUTE_BIT) == 0) continue;
      if ((qp[vlk_qf].queueFlags & VK_QUEUE_TRANSFER_BIT) == 0) continue;
      vlk_pd = pd[i];
      return;
    }
  }
  assert(0);
}

static void vlk_create_device() {
  const char * ext[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    "VK_KHR_portability_subset",
  };

  const float pri = 1.0f;
  VkDeviceQueueCreateInfo q = (VkDeviceQueueCreateInfo) {
    .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
    .queueCount = 1,
    .pQueuePriorities = &pri,
    .queueFamilyIndex = vlk_qf,
  };
  VkDeviceCreateInfo info = (VkDeviceCreateInfo) {
    .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
    .queueCreateInfoCount = 1,
    .pQueueCreateInfos = &q,
    .ppEnabledExtensionNames = ext,
    .enabledExtensionCount = 1,
  };

#ifdef __APPLE__
  // It would be more "Vulkan-idiomatic" to test if the current instance has
  // the portability flag.
  info.ppEnabledExtensionNames = ext;
  info.enabledExtensionCount++;
#endif

  _(vkCreateDevice(vlk_pd, &info, NULL, &vlk_dev));
#if !TARGET_OS_IPHONE
  volkLoadDevice(vlk_dev);
#endif

  vkGetDeviceQueue(vlk_dev, vlk_qf, 0, &vlk_q);
}

static void vlk_create_render_pass() {
  VkAttachmentDescription att = {
    .format      = vlk_surf_fmt.format,
    .samples     = VK_SAMPLE_COUNT_1_BIT,
    .loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR,
    .storeOp     = VK_ATTACHMENT_STORE_OP_STORE,
    .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
  };

  VkAttachmentReference ref = {
    .attachment = 0,
    .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
  };

  // Without this, we might face WRITE_AFTER_READ access hazards
  VkSubpassDependency dep = {
    .srcSubpass    = VK_SUBPASS_EXTERNAL,
    .srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    .dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
  };

  VkSubpassDescription subpass = {
    .colorAttachmentCount = 1,
    .pColorAttachments    = &ref,
  };

  VkRenderPassCreateInfo info = {
    .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
    .attachmentCount = 1,
    .pAttachments    = &att,
    .subpassCount    = 1,
    .pSubpasses      = &subpass,
    .dependencyCount = 1,
    .pDependencies   = &dep,
  };
  _(vkCreateRenderPass(vlk_dev, &info, NULL, &vlk_rp));
}

static void vlk_create_surface() {
#ifdef __APPLE__
  VkMetalSurfaceCreateInfoEXT info = {
    .sType = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT,
    .pLayer = vlk_metal_layer(),
  };
  _(vkCreateMetalSurfaceEXT(vlk_ins, &info, NULL, &vlk_surf));
#endif

  VkSurfaceCapabilitiesKHR cap;
  _(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vlk_pd, vlk_surf, &cap));

  vlk_swc_count = cap.minImageCount + 1;
  if (vlk_swc_count > cap.maxImageCount && cap.maxImageCount > 0) vlk_swc_count = cap.maxImageCount;
  assert(vlk_swc_count < MAX_SWAPCHAIN_IMAGES);

  // No concensus on docs or the Internet about how to deal with surface
  // formats. Picking the first seems to be enough for most cases.
  uint32_t sz = 1;
  VkResult res = vkGetPhysicalDeviceSurfaceFormatsKHR(vlk_pd, vlk_surf, &sz, &vlk_surf_fmt);
  if (res != VK_INCOMPLETE) vlk_check(res, "vkGetPhysicalDeviceSurfaceFormatsKHR invalid return");
}

static void vlk_create_swapchain() {
  VkSwapchainCreateInfoKHR info = {
    .sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
    .surface          = vlk_surf,
    .minImageCount    = vlk_swc_count,
    .imageFormat      = vlk_surf_fmt.format,
    .imageColorSpace  = vlk_surf_fmt.colorSpace,
    .imageExtent      = vlk_ext,
    .imageArrayLayers = 1,
    // In theory we can add more usages as well
    .imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    .compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
    .preTransform     = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
    // We can use others modes, if supported, for max FPS or discard frames if
    // CPU faster than GPU
    .presentMode      = VK_PRESENT_MODE_FIFO_KHR,
    // Should be "true" unless we want to read clipped parts
    .clipped          = VK_TRUE,
    .oldSwapchain     = vlk_swc_old.swc,
  };
  _(vkCreateSwapchainKHR(vlk_dev, &info, NULL, &vlk_swc.swc));

  _(vkGetSwapchainImagesKHR(vlk_dev, vlk_swc.swc, &vlk_swc_count, vlk_swc_img));
}

static void vlk_create_image_views() {
  for (int i = 0; i < vlk_swc_count; i++) {
    VkImageViewCreateInfo info = {
      .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image            = vlk_swc_img[i],
      .viewType         = VK_IMAGE_VIEW_TYPE_2D,
      .format           = vlk_surf_fmt.format,
      .subresourceRange = {
        .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
        .levelCount     = 1,
        .layerCount     = 1,
      },
    };
    _(vkCreateImageView(vlk_dev, &info, NULL, vlk_swc.iv + i));
  }
}

static void vlk_create_framebuffer() {
  for (int i = 0; i < vlk_swc_count; i++) {
    VkFramebufferCreateInfo info = {
      .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
      .renderPass      = vlk_rp,
      .attachmentCount = 1,
      .pAttachments    = vlk_swc.iv + i,
      .width           = vlk_ext.width,
      .height          = vlk_ext.height,
      .layers          = 1,
    };
    _(vkCreateFramebuffer(vlk_dev, &info, NULL, vlk_swc.fb + i));
  }
}

static void vlk_create_semaphores() {
  VkSemaphoreCreateInfo info = {
    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
  };
  for (int i = 0; i < MAX_INFLIGHTS; i++) {
    _(vkCreateSemaphore(vlk_dev, &info, NULL, vlk_sema_img     + i));
    _(vkCreateSemaphore(vlk_dev, &info, NULL, vlk_sema_present + i));
  }
}

static void vlk_create_fence() {
  VkFenceCreateInfo info = {
    .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
    .flags = VK_FENCE_CREATE_SIGNALED_BIT,
  };
  for (int i = 0; i < MAX_INFLIGHTS; i++) {
    _(vkCreateFence(vlk_dev, &info, NULL, vlk_fence + i));
  }
}

static void vlk_create_command_pool() {
  VkCommandPoolCreateInfo info = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
    .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
  };
  _(vkCreateCommandPool(vlk_dev, &info, NULL, &vlk_cpool));
}

static void vlk_create_command_buffer() {
  VkCommandBufferAllocateInfo info = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .commandPool = vlk_cpool,
    .commandBufferCount = vlk_swc_count,
  };
  _(vkAllocateCommandBuffers(vlk_dev, &info, vlk_cb));
}

static void vlk_begin_command_buffer(int i) {
  VkCommandBufferBeginInfo info = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
  };
  vkBeginCommandBuffer(vlk_cb[i], &info);
}
static void vlk_end_command_buffer(int i) {
  vkEndCommandBuffer(vlk_cb[i]);
}

static void vlk_cmd_begin_render_pass(int i) {
  VkClearValue clear = {
    .color = {{ 0.1, 0.2, 0.3, 1 }},
  };
  VkRenderPassBeginInfo rp = {
    .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
    .renderPass      = vlk_rp,
    .framebuffer     = vlk_swc.fb[i],
    .renderArea      = (VkRect2D) { .extent = vlk_ext },
    .clearValueCount = 1,
    .pClearValues    = &clear
  };
  vkCmdBeginRenderPass(vlk_cb[i], &rp, VK_SUBPASS_CONTENTS_INLINE);
}
static void vlk_cmd_end_render_pass(int i) {
  vkCmdEndRenderPass(vlk_cb[i]);
}

static void vlk_destroy_swc(vlk_swc_t * swc) {
  for (int i = 0; i < vlk_swc_count; i++) {
    vkDestroyFramebuffer(vlk_dev, swc->fb[i], NULL);
    vkDestroyImageView(vlk_dev, swc->iv[i], NULL);
  }
  vkDestroySwapchainKHR(vlk_dev, swc->swc, NULL);

  *swc = (vlk_swc_t) {0};
}
static void vlk_create_swc() {
  if (vlk_swc_old.swc) vlk_destroy_swc(&vlk_swc_old);
  vlk_swc_old = vlk_swc;

  vlk_create_swapchain();
  vlk_create_image_views();
  vlk_create_framebuffer();
}

FILE * vlk_open(const char * name);
static VkShaderModule vlk_create_shader_module(const char * name) {
  FILE * f = vlk_open(name);
  assert(f);
  assert(0 == fseek(f, 0, SEEK_END));
  long sz = ftell(f);
  assert(sz && (sz % 4 == 0));
  assert(0 == fseek(f, 0, SEEK_SET));
  uint32_t * data = malloc(sz);
  assert(1 == fread(data, sz, 1, f));
  fclose(f);

  VkShaderModuleCreateInfo info = {
    .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
    .codeSize = sz,
    .pCode = data,
  };

  VkShaderModule mod;
  _(vkCreateShaderModule(vlk_dev, &info, NULL, &mod));

  free(data);
  return mod;
}

#define F(x, y) (((x) & (y)) == (y))
static int vlk_find_host_memory() {
  VkPhysicalDeviceMemoryProperties props;
  vkGetPhysicalDeviceMemoryProperties(vlk_pd, &props);

  int host = -1;
  for (int i = 0; i < props.memoryTypeCount; i++) {
    VkMemoryPropertyFlags flags = props.memoryTypes[i].propertyFlags;
    if (host == -1 && F(flags, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) host = i;
  }
  assert(host >= 0);
  return host;
}

void vlk_init() {
#if !TARGET_OS_IPHONE
  _(volkInitialize());
#endif

  vlk_create_instance();
  vlk_find_physical_device();
  vlk_create_surface();
  vlk_create_device();
  vlk_create_command_pool();
  vlk_create_command_buffer();
  vlk_create_render_pass();
  vlk_create_swc();
  vlk_create_semaphores();
  vlk_create_fence();

  vlk_create_swc();

  VkBufferCreateInfo vbuf_info = {
    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .size = VBUF_SIZE,
    .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
  };
  _(vkCreateBuffer(vlk_dev, &vbuf_info, NULL, &vlk_vbuf));

  VkMemoryAllocateInfo vmem_info = {
    .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
    .allocationSize = VBUF_SIZE,
    .memoryTypeIndex = vlk_find_host_memory(),
  };
  _(vkAllocateMemory(vlk_dev, &vmem_info, NULL, &vlk_vmem));
  _(vkBindBufferMemory(vlk_dev, vlk_vbuf, vlk_vmem, 0));

  VkDescriptorSetLayoutCreateInfo dsl_info = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
    .bindingCount = 1,
    .pBindings = (VkDescriptorSetLayoutBinding[]) {{
      .binding = 0,
      .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      .descriptorCount = 1,
      .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
    }},
  };
  _(vkCreateDescriptorSetLayout(vlk_dev, &dsl_info, NULL, &vlk_dsl));

  VkDescriptorPoolCreateInfo dpool_info = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
    .maxSets = 1,
    .poolSizeCount = 1,
    .pPoolSizes = (VkDescriptorPoolSize[]) {{
      .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      .descriptorCount = 1,
    }},
  };
  _(vkCreateDescriptorPool(vlk_dev, &dpool_info, NULL, &vlk_dpool));

  VkDescriptorSetAllocateInfo dset_info = {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
    .descriptorPool = vlk_dpool,
    .descriptorSetCount = 1,
    .pSetLayouts = &vlk_dsl,
  };
  _(vkAllocateDescriptorSets(vlk_dev, &dset_info, &vlk_dset));

  VkPipelineLayoutCreateInfo pl_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .setLayoutCount = 1,
    .pSetLayouts = &vlk_dsl,
  };
  _(vkCreatePipelineLayout(vlk_dev, &pl_info, NULL, &vlk_pl));

  VkShaderModule vert = vlk_create_shader_module("boav.vert");
  VkShaderModule frag = vlk_create_shader_module("boav.frag");

  VkGraphicsPipelineCreateInfo ppl_info = {
    .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
  };
  _(vkCreateGraphicsPipelines(vlk_dev, NULL, 1, &ppl_info, NULL, &vlk_ppl));

  vkDestroyShaderModule(vlk_dev, vert, NULL);
  vkDestroyShaderModule(vlk_dev, frag, NULL);
}

void vlk_frame() {
  unsigned inf = vlk_cur_inflight;

  _(vkWaitForFences(vlk_dev, 1, vlk_fence + inf, VK_TRUE, ~0UL));
  _(vkResetFences  (vlk_dev, 1, vlk_fence + inf));

  unsigned idx;
  vkAcquireNextImageKHR(vlk_dev, vlk_swc.swc, ~0UL, vlk_sema_img[inf], VK_NULL_HANDLE, &idx);

  vlk_begin_command_buffer(idx);
  vlk_cmd_begin_render_pass(idx);
  // Render pass is only used to draw something via its "clear value"
  vlk_cmd_end_render_pass(idx);
  vlk_end_command_buffer(idx);

  // TODO: confirm if this is the best and document why
  VkPipelineStageFlags stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  // The idea of the wait semaphore is to wait until the swapchain _actually_
  // made the image available
  VkSubmitInfo submit = {
    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .pCommandBuffers = vlk_cb + idx,
    .commandBufferCount = 1,
    .pWaitSemaphores = vlk_sema_img + inf,
    .pWaitDstStageMask = &stage,
    .waitSemaphoreCount = 1,
    .pSignalSemaphores = vlk_sema_present + inf,
    .signalSemaphoreCount = 1,
  };
  // The fence signals we can reuse the current in-flight
  _(vkQueueSubmit(vlk_q, 1, &submit, vlk_fence[inf]));

  // Present is entirely async. We don't have control when it finishes. We then
  // use a semaphore to force it to wait until we finished processing the
  // image.
  VkPresentInfoKHR pres = {
    .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
    .pWaitSemaphores = vlk_sema_present + inf,
    .waitSemaphoreCount = 1,
    .swapchainCount = 1,
    .pSwapchains = &vlk_swc.swc,
    .pImageIndices = &idx,
  };
  VkResult res = vkQueuePresentKHR(vlk_q, &pres);
  // TODO: deal with suboptimal
  if (res != VK_SUBOPTIMAL_KHR) {
    vlk_check(res, "vkQueuePresentKHR");
  } else {
    VkSurfaceCapabilitiesKHR cap;
    _(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vlk_pd, vlk_surf, &cap));
    vlk_ext = cap.currentExtent;

    vlk_create_swc();
  }
}

void vlk_deinit() {
  vkDeviceWaitIdle(vlk_dev);

  vlk_destroy_swc(&vlk_swc);
  vlk_destroy_swc(&vlk_swc_old);

  for (int i = 0; i < MAX_INFLIGHTS; i++) {
    vkDestroyFence    (vlk_dev, vlk_fence       [i], NULL);
    vkDestroySemaphore(vlk_dev, vlk_sema_img    [i], NULL);
    vkDestroySemaphore(vlk_dev, vlk_sema_present[i], NULL);
  }

  vkDestroyBuffer(vlk_dev, vlk_vbuf, NULL);
  vkFreeMemory(vlk_dev, vlk_vmem, NULL);

  vkDestroyCommandPool(vlk_dev, vlk_cpool, NULL);
  vkDestroyRenderPass(vlk_dev, vlk_rp, NULL);
  vkDestroyDevice(vlk_dev, NULL);
  vkDestroySurfaceKHR(vlk_ins, vlk_surf, NULL);
  vkDestroyInstance(vlk_ins, NULL);
}
