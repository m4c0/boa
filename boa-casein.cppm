export module boa:casein;
import :ecs_objects;
import :pipeline;
import :vulkan;
import casein;
import hai;
import sires;
import traits;
import vee;

using device_stuff = boa::vulkan::per_device;
using extent_stuff = boa::vulkan::per_extent;
using inflight_stuff = boa::vulkan::per_inflight;
using inflights = boa::vulkan::inflight_pair;
using frame_stuff = boa::vulkan::per_frame;

enum states {
  waiting_nptr,
  setup_stuff,
  ready_to_paint,
  done,
  failed_to_start,
};
namespace boa::casein {
class fsm {
  hai::uptr<vulkan::per_device> m_dev{};
  hai::uptr<vulkan::per_extent> m_ext{};
  hai::uptr<vulkan::pipeline> m_ppl{};
  hai::uptr<vulkan::inflight_pair> m_infs{};
  hai::holder<hai::uptr<vulkan::per_frame>[]> m_frms{};
  states m_state{};

  void setup() {
    m_ext = hai::uptr<extent_stuff>::make(&*m_dev);
    m_ppl = hai::uptr<boa::vulkan::pipeline>::make(&*m_dev, &*m_ext);
    m_infs = hai::uptr<inflights>::make(&*m_dev);

    auto imgs = vee::get_swapchain_images(m_ext->swapchain());
    m_frms = decltype(m_frms)::make(imgs.size());
    for (auto i = 0; i < imgs.size(); i++) {
      auto img = (imgs.data())[i];
      (*m_frms)[i] = hai::uptr<frame_stuff>::make(&*m_dev, &*m_ext, img);
    }

    m_ppl->map_vertices([](auto *vs) {
      vs[0] = {-1, -1};
      vs[1] = {1, 1};
      vs[2] = {1, -1};

      vs[3] = {1, 1};
      vs[4] = {-1, -1};
      vs[5] = {-1, 1};
      return 6;
    });

    m_state = ready_to_paint;
  }

  void paint() {
    try {
      auto &inf = m_infs->flip();

      auto idx = inf.wait_and_takeoff(&*m_ext);

      m_ppl->build_commands(inf.command_buffer());

      inf.submit(&*m_dev, (*m_frms)[idx]->one_time_submit([&inf](auto cb) {
        vee::cmd_execute_command(cb, inf.command_buffer());
      }));
      vee::queue_present({
          .queue = m_dev->queue(),
          .swapchain = m_ext->swapchain(),
          .wait_semaphore = inf.render_finished_sema(),
          .image_index = idx,
      });
    } catch (vee::out_of_date_error) {
      m_state = setup_stuff;
      vee::device_wait_idle();
    }
  }

public:
  void create_window(const ::casein::events::create_window &e) {
    if (m_state == waiting_nptr) {
      auto nptr = e.native_window_handle();
      vee::initialise();
      m_dev = hai::uptr<device_stuff>::make(nptr);
    }
    m_state = sires::open("main.vert.spv")
                  .map([](auto &&) { return setup_stuff; })
                  .unwrap(failed_to_start);
  }

  void repaint() {
    if (m_state == setup_stuff) {
      setup();
    } else if (m_state == ready_to_paint) {
      paint();
    }
  }

  void quit() {
    vee::device_wait_idle();
    m_state = done;
  }

  [[nodiscard]] auto &state() noexcept { return m_state; }
  [[nodiscard]] const auto *dev() const noexcept { return &*m_dev; }
};
} // namespace boa::casein

extern "C" void casein_handle(const casein::event &e) {
  static auto fsm = hai::uptr<boa::casein::fsm>::make();

  switch (e.type()) {
  case casein::CREATE_WINDOW:
    fsm->create_window(e.as<casein::events::create_window>());
    break;
  case casein::REPAINT:
    fsm->repaint();
    break;
  case casein::QUIT:
    fsm->quit();
    fsm = {};
    break;
  default:
    break;
  }
}
