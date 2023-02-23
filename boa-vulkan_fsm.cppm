export module boa:vulkan_fsm;
import :ecs_objects;
import :pipeline;
import :vulkan;
import casein;
import hai;
import sires;
import vee;

namespace boa::vulkan {
enum states {
  waiting_nptr,
  setup_stuff,
  ready_to_paint,
  done,
  failed_to_start,
};

class fsm {
  hai::uptr<per_device> m_dev{};
  hai::uptr<per_extent> m_ext{};
  hai::uptr<pipeline> m_ppl{};
  hai::uptr<inflight_pair> m_infs{};
  hai::holder<hai::uptr<per_frame>[]> m_frms{};

  void setup() {
    m_ext = hai::uptr<per_extent>::make(&*m_dev);
    m_ppl = hai::uptr<pipeline>::make(&*m_dev, &*m_ext);
    m_infs = hai::uptr<inflight_pair>::make(&*m_dev);

    auto imgs = vee::get_swapchain_images(m_ext->swapchain());
    m_frms = decltype(m_frms)::make(imgs.size());
    for (auto i = 0; i < imgs.size(); i++) {
      auto img = (imgs.data())[i];
      (*m_frms)[i] = hai::uptr<per_frame>::make(&*m_dev, &*m_ext, img);
    }
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
      vee::device_wait_idle();
      setup();
    }
  }

public:
  void create_window(const ::casein::events::create_window &e) {
    auto nptr = e.native_window_handle();
    vee::initialise();
    m_dev = hai::uptr<per_device>::make(nptr);
    setup();
  }

  void repaint() { paint(); }

  void update(const ecs::grid &g) {
    m_ppl->map_instances_colour([&](auto is) {
      constexpr const ecs::rgba on{1, 1, 1, 1};
      constexpr const ecs::rgba off{0, 0.1, 0, 1};

      for (auto b : g) {
        *is = b ? on : off;
        is++;
      }
    });
  }

  void quit() { vee::device_wait_idle(); }
};
} // namespace boa::vulkan
