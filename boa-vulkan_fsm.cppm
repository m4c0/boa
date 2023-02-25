export module boa:vulkan_fsm;
import :ecs_objects;
import :pipeline;
import :render;
import :vulkan;
import casein;
import hai;
import sires;
import vee;

namespace boa {
enum states {
  waiting_nptr,
  setup_stuff,
  ready_to_paint,
  done,
  failed_to_start,
};

class r_pimpl {
  hai::uptr<vulkan::per_device> m_dev{};
  hai::uptr<vulkan::per_extent> m_ext{};
  hai::uptr<vulkan::pipeline> m_ppl{};
  hai::uptr<vulkan::inflight_pair> m_infs{};
  hai::holder<hai::uptr<vulkan::per_frame>[]> m_frms{};

  void setup() {
    m_ext = hai::uptr<vulkan::per_extent>::make(&*m_dev);
    m_ppl = hai::uptr<vulkan::pipeline>::make(&*m_dev, &*m_ext);
    m_infs = hai::uptr<vulkan::inflight_pair>::make(&*m_dev);

    auto imgs = vee::get_swapchain_images(m_ext->swapchain());
    m_frms = decltype(m_frms)::make(imgs.size());
    for (auto i = 0; i < imgs.size(); i++) {
      auto img = (imgs.data())[i];
      (*m_frms)[i] = hai::uptr<vulkan::per_frame>::make(&*m_dev, &*m_ext, img);
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
  void create_window(const ::casein::native_handle_t nptr) {
    vee::initialise();
    m_dev = hai::uptr<vulkan::per_device>::make(nptr);
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

renderer::renderer() : m_data{hai::uptr<r_pimpl>::make()} {}
renderer::~renderer() {}
void renderer::setup(casein::native_handle_t nptr) {
  m_data->create_window(nptr);
}
void renderer::update(const ecs::grid &g) { m_data->update(g); }
void renderer::repaint() { m_data->repaint(); }
void renderer::quit() {
  m_data->quit();
  m_data = {};
}
} // namespace boa
