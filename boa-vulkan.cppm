export module boa:vulkan;
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

  [[nodiscard]] constexpr auto physical_device_pair() const noexcept {
    return pdqf;
  }
  [[nodiscard]] constexpr auto queue() const noexcept { return q; }
  [[nodiscard]] constexpr auto &surface() const noexcept { return s; }
};
} // namespace boa::vulkan
