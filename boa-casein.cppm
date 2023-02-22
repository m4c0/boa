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
  states m_state{};

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

  [[nodiscard]] auto &state() noexcept { return m_state; }
  [[nodiscard]] const auto *dev() const noexcept { return &*m_dev; }
};
} // namespace boa::casein

extern "C" void casein_handle(const casein::event &e) {
  static boa::casein::fsm fsm{};
  static hai::uptr<extent_stuff> ext{};
  static hai::uptr<boa::vulkan::pipeline> ppl{};
  static hai::uptr<inflights> infs{};
  static hai::holder<hai::uptr<frame_stuff>[]> frms{};

  switch (e.type()) {
  case casein::CREATE_WINDOW:
    fsm.create_window(e.as<casein::events::create_window>());
    break;
  case casein::REPAINT:
    switch (fsm.state()) {
    case setup_stuff: {
      ext = hai::uptr<extent_stuff>::make(fsm.dev());
      ppl = hai::uptr<boa::vulkan::pipeline>::make(fsm.dev(), &*ext);
      infs = hai::uptr<inflights>::make(fsm.dev());

      auto imgs = vee::get_swapchain_images(ext->swapchain());
      frms = decltype(frms)::make(imgs.size());
      for (auto i = 0; i < imgs.size(); i++) {
        auto img = (imgs.data())[i];
        (*frms)[i] = hai::uptr<frame_stuff>::make(fsm.dev(), &*ext, img);
      }

      ppl->map_vertices([](auto *vs) {
        vs[0] = {-1, -1};
        vs[1] = {1, 1};
        vs[2] = {1, -1};

        vs[3] = {1, 1};
        vs[4] = {-1, -1};
        vs[5] = {-1, 1};
        return 6;
      });

      fsm.state() = ready_to_paint;
      break;
    }
    case ready_to_paint: {
      try {
        auto &inf = infs->flip();

        auto idx = inf.wait_and_takeoff(&*ext);

        ppl->build_commands(inf.command_buffer());

        inf.submit(fsm.dev(), (*frms)[idx]->one_time_submit([&inf](auto cb) {
          vee::cmd_execute_command(cb, inf.command_buffer());
        }));
        vee::queue_present({
            .queue = fsm.dev()->queue(),
            .swapchain = ext->swapchain(),
            .wait_semaphore = inf.render_finished_sema(),
            .image_index = idx,
        });
      } catch (vee::out_of_date_error) {
        fsm.state() = setup_stuff;
        vee::device_wait_idle();
      }
      break;
    }
    default:
      break;
    }
    break;
  case casein::QUIT:
    vee::device_wait_idle();
    frms = {};
    infs = {};
    ppl = {};
    ext = {};
    fsm = {};
    fsm.state() = done;
    break;
  default:
    break;
  }
}
