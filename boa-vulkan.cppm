export module boa:vulkan;
import :ecs_objects;
import casein;
import vee;

namespace boa::vulkan {
class per_device {
  casein::native_handle_t nptr;

  vee::instance i = vee::create_instance("my-app");
  vee::debug_utils_messenger dbg = vee::create_debug_utils_messenger();
  vee::surface s = vee::create_surface(nptr);
  vee::physical_device_pair pdqf =
      vee::find_physical_device_with_universal_queue(*s);
  vee::device d =
      vee::create_single_queue_device(pdqf.physical_device, pdqf.queue_family);

  vee::queue q = vee::get_queue_for_family(pdqf.queue_family);

public:
  explicit per_device(casein::native_handle_t nptr) : nptr{nptr} {}

  [[nodiscard]] constexpr auto physical_device() const noexcept {
    return pdqf.physical_device;
  }
  [[nodiscard]] constexpr auto queue_family() const noexcept {
    return pdqf.queue_family;
  }
  [[nodiscard]] constexpr auto queue() const noexcept { return q; }
  [[nodiscard]] constexpr auto surface() const noexcept { return *s; }
};

class per_extent {
  const per_device *dev;

  vee::extent extent =
      vee::get_surface_capabilities(dev->physical_device(), dev->surface())
          .currentExtent;

  vee::command_pool cp = vee::create_command_pool(dev->queue_family());
  vee::render_pass rp =
      vee::create_render_pass(dev->physical_device(), dev->surface());
  vee::swapchain swc =
      vee::create_swapchain(dev->physical_device(), dev->surface());

  vee::pipeline_layout pl = vee::create_pipeline_layout();

  vee::shader_module vert =
      vee::create_shader_module_from_resource("main.vert.spv");
  vee::shader_module frag =
      vee::create_shader_module_from_resource("main.frag.spv");
  vee::gr_pipeline gp = vee::create_graphics_pipeline(
      *pl, *rp,
      {
          vee::pipeline_vert_stage(*vert, "main"),
          vee::pipeline_frag_stage(*frag, "main"),
      },
      {
          vee::vertex_input_bind(sizeof(ecs::point)),
      },
      {
          vee::vertex_attribute_vec2(0, 0),
      });

  vee::buffer v_buf = vee::create_vertex_buffer(sizeof(ecs::point) * 3);
  vee::device_memory v_mem =
      vee::create_host_buffer_memory(dev->physical_device(), *v_buf);
  decltype(nullptr) v_bind = vee::bind_buffer_memory(*v_buf, *v_mem);

  vee::image d_img =
      vee::create_depth_image(dev->physical_device(), dev->surface());
  vee::device_memory d_mem =
      vee::create_local_image_memory(dev->physical_device(), *d_img);
  decltype(nullptr) d_bind = vee::bind_image_memory(*d_img, *d_mem);
  vee::image_view d_iv = vee::create_depth_image_view(*d_img);

public:
  per_extent(const per_device *dev) : dev{dev} {}

  [[nodiscard]] constexpr auto command_pool() const noexcept { return *cp; }
  [[nodiscard]] constexpr auto extent_2d() const noexcept { return extent; }
  [[nodiscard]] constexpr auto render_pass() const noexcept { return *rp; }
  [[nodiscard]] constexpr auto swapchain() const noexcept { return *swc; }

  void begin_secondary_cmdbuf(vee::command_buffer cb) const {
    vee::begin_cmd_buf_render_pass_continue(cb, *rp);
    vee::cmd_set_scissor(cb, extent);
    vee::cmd_set_viewport(cb, extent);
    vee::cmd_bind_gr_pipeline(cb, *gp);
    vee::cmd_bind_vertex_buffers(cb, 0, *v_buf);
  }

  [[nodiscard]] auto create_framebuffer(const vee::image_view &iv) const {
    vee::fb_params fbp{
        .physical_device = dev->physical_device(),
        .surface = dev->surface(),
        .render_pass = *this->rp,
        .image_buffer = *iv,
        .depth_buffer = *this->d_iv,
    };
    return vee::create_framebuffer(fbp);
  }

  void map_vertices(auto fn) { vee::map_memory<boa::ecs::point>(*v_mem, fn); }
};
} // namespace boa::vulkan
