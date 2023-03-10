export module boa:vulkan;
import :ecs_objects;
import casein;
import traits;
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

  vee::swapchain swc =
      vee::create_swapchain(dev->physical_device(), dev->surface());

  vee::command_pool pcp = vee::create_command_pool(dev->queue_family());
  vee::render_pass rp =
      vee::create_render_pass(dev->physical_device(), dev->surface());

  vee::image d_img =
      vee::create_depth_image(dev->physical_device(), dev->surface());
  vee::device_memory d_mem =
      vee::create_local_image_memory(dev->physical_device(), *d_img);
  decltype(nullptr) d_bind = vee::bind_image_memory(*d_img, *d_mem);
  vee::image_view d_iv = vee::create_depth_image_view(*d_img);

  unsigned v_count{};

public:
  per_extent(const per_device *dev) : dev{dev} {}

  [[nodiscard]] constexpr auto primary_command_pool() const noexcept {
    return *pcp;
  }
  [[nodiscard]] constexpr auto extent_2d() const noexcept { return extent; }
  [[nodiscard]] constexpr auto render_pass() const noexcept { return *rp; }
  [[nodiscard]] constexpr auto swapchain() const noexcept { return *swc; }

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
};

class per_inflight {
  vee::semaphore img_available_sema = vee::create_semaphore();
  vee::semaphore rnd_finished_sema = vee::create_semaphore();
  vee::fence f = vee::create_fence_signaled();
  vee::command_buffer cb;

public:
  explicit per_inflight(const vee::command_pool *pool)
      : cb{vee::allocate_secondary_command_buffer(**pool)} {}

  [[nodiscard]] constexpr auto command_buffer() const noexcept { return cb; }
  [[nodiscard]] constexpr auto render_finished_sema() const noexcept {
    return *rnd_finished_sema;
  }

  [[nodiscard]] auto wait_and_takeoff(const per_extent *ext) const {
    vee::wait_and_reset_fence(*f);
    return vee::acquire_next_image(ext->swapchain(), *img_available_sema);
  }

  void submit(const per_device *dev,
              const vee::command_buffer primary_cb) const {
    vee::queue_submit({
        .queue = dev->queue(),
        .fence = *f,
        .command_buffer = primary_cb, // TODO: "per_frame"
        .wait_semaphore = *img_available_sema,
        .signal_semaphore = *rnd_finished_sema,
    });
  }
};

class inflight_pair {
  vee::command_pool cp;

  per_inflight front{&cp};
  per_inflight back{&cp};

public:
  explicit inflight_pair(const per_device *dev)
      : cp{vee::create_command_pool(dev->queue_family())} {}

  [[nodiscard]] auto &flip() {
    auto tmp = traits::move(front);
    front = traits::move(back);
    back = traits::move(tmp);

    return back;
  }
};

class per_frame {
  const per_extent *ext;
  vee::image_view iv;
  vee::command_buffer cb =
      vee::allocate_primary_command_buffer(ext->primary_command_pool());
  vee::framebuffer fb = ext->create_framebuffer(iv);

public:
  per_frame(const per_device *dev, const per_extent *ext, auto img)
      : ext{ext}, iv{vee::create_rgba_image_view(img, dev->physical_device(),
                                                 dev->surface())} {}

  [[nodiscard]] auto one_time_submit(auto fn) {
    vee::begin_cmd_buf_one_time_submit(cb);
    vee::cmd_begin_render_pass({
        .command_buffer = cb,
        .render_pass = ext->render_pass(),
        .framebuffer = *fb,
        .extent = ext->extent_2d(),
    });

    fn(cb);

    vee::cmd_end_render_pass(cb);
    vee::end_cmd_buf(cb);
    return cb;
  }
};
} // namespace boa::vulkan
