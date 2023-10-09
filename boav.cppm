export module boav;
#ifndef LECO_TARGET_IPHONEOS
import :offscreen;
#endif
import boa;
import casein;
import hai;
import silog;
import sires;
import sith;
import sitime;
import vee;

#ifdef LECO_TARGET_IPHONEOS
class offscreen {
public:
  constexpr offscreen(auto, auto) {}
  constexpr void cmd_render_pass(auto, auto) {}
  constexpr void write() {}
};
#endif

// Covers a 4:1 screen, as if such thing will ever exist
static constexpr const auto max_cells = 24 * (24 * 4);

class vec2 {
  float m_x = 0.0;
  float m_y = 0.0;

public:
  constexpr vec2() = default;
  constexpr vec2(auto x, auto y)
      : m_x{static_cast<float>(x)}, m_y{static_cast<float>(y)} {}

  constexpr bool operator==(const vec2 &o) const noexcept {
    // Only works because they are always rounded (i.e. came from ints)
    return m_x == o.m_x && m_y == o.m_y;
  }
};
struct upc {
  float aspect;
  float time;
  float dead_at;
  float pad{};
  float grid_width;
  float grid_height;
  vec2 food;
  vec2 party;
  float party_start;
};

struct storage {
  float first_seen;
  float seen;
};

struct quad {
  float points[12]{0.0, 0.0, 1.0, 1.0, 1.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0, 1.0};
};

auto frag_mod() {
  return sires::stat("boav.frag.spv").take([](auto err) {
    silog::log(silog::error, "Failed to stat shader: %s", err);
    return 0;
  });
}

class thread : public sith::thread {
  casein::native_handle_t m_nptr;
  volatile float m_aspect;
  volatile bool m_resized;
  volatile bool m_shots;
  boa::game *volatile m_g{};

public:
  void render(boa::game *g) { m_g = g; }

  void start(casein::native_handle_t n) {
    m_nptr = n;
    sith::thread::start();
  }
  void resize(float aspect) {
    m_aspect = aspect;
    m_resized = true;
  }
  void take_shots() { m_shots = true; }

  void run() override {
    sitime::stopwatch watch{};

    // Instance
    vee::instance i = vee::create_instance("boas");
    vee::debug_utils_messenger dbg = vee::create_debug_utils_messenger();
    vee::surface s = vee::create_surface(m_nptr);
    auto [pd, qf] = vee::find_physical_device_with_universal_queue(*s);

    // Device
    vee::device d = vee::create_single_queue_device(pd, qf);
    vee::queue q = vee::get_queue_for_family(qf);

    // Command pool + buffers
    vee::command_pool cp = vee::create_command_pool(qf);
    vee::command_buffer cb = vee::allocate_primary_command_buffer(*cp);

    vee::semaphore img_available_sema = vee::create_semaphore();
    vee::semaphore rnd_finished_sema = vee::create_semaphore();
    vee::fence f = vee::create_fence_signaled();

    while (!interrupted()) {
      try {
        // Swapchain
        vee::swapchain swc = vee::create_swapchain(pd, *s);
        auto swc_imgs = vee::get_swapchain_images(*swc);

        // Depth buffer
        vee::image d_img = vee::create_depth_image(pd, *s);
        vee::device_memory d_mem = vee::create_local_image_memory(pd, *d_img);
        vee::bind_image_memory(*d_img, *d_mem);
        vee::image_view d_iv = vee::create_depth_image_view(*d_img);

        // Quad vertex buffer
        vee::buffer qv_buf = vee::create_vertex_buffer(sizeof(quad));
        vee::device_memory qv_mem = vee::create_host_buffer_memory(pd, *qv_buf);
        vee::bind_buffer_memory(*qv_buf, *qv_mem);
        { *static_cast<quad *>(*vee::mapmem{*qv_mem}) = {}; }

        // Game grid buffer
        constexpr const auto gg_buf_size = max_cells * sizeof(storage);
        vee::buffer gg_buf = vee::create_storage_buffer(gg_buf_size);
        vee::device_memory gg_mem = vee::create_host_buffer_memory(pd, *gg_buf);
        vee::bind_buffer_memory(*gg_buf, *gg_mem);

        vee::extent ext = vee::get_surface_capabilities(pd, *s).currentExtent;
        vee::render_pass rp = vee::create_render_pass(pd, *s);

        // Descriptor set layout + pool + set
        vee::descriptor_set_layout dsl = vee::create_descriptor_set_layout({
            vee::dsl_fragment_storage(),
        });
        vee::descriptor_pool dp =
            vee::create_descriptor_pool(1, {vee::storage_buffer()});
        vee::descriptor_set dset = vee::allocate_descriptor_set(*dp, *dsl);
        vee::update_descriptor_set_with_storage(dset, 0, *gg_buf);

        // Pipeline
        vee::pipeline_layout pl = vee::create_pipeline_layout(
            {*dsl}, {vee::vert_frag_push_constant_range<upc>()});
        long frag_ts{};
        const auto create_gp = [&](const vee::render_pass &rp) {
          vee::shader_module vert =
              vee::create_shader_module_from_resource("boav.vert.spv");
          vee::shader_module frag =
              vee::create_shader_module_from_resource("boav.frag.spv");
          return vee::create_graphics_pipeline(
              *pl, *rp,
              {
                  vee::pipeline_vert_stage(*vert, "main"),
                  vee::pipeline_frag_stage(*frag, "main"),
              },
              {
                  vee::vertex_input_bind(2 * sizeof(float)),
              },
              {
                  vee::vertex_attribute_vec2(0, 0),
              });
        };
        vee::gr_pipeline gp = create_gp(rp);

        // Frame buffers
        hai::array<vee::image_view> c_ivs{swc_imgs.size()};
        hai::array<vee::framebuffer> fbs{swc_imgs.size()};
        for (auto i = 0; i < fbs.size(); i++) {
          c_ivs[i] = vee::create_rgba_image_view(swc_imgs[i], pd, *s);
          fbs[i] = vee::create_framebuffer({
              .physical_device = pd,
              .surface = *s,
              .render_pass = *rp,
              .image_buffer = *c_ivs[i],
              .depth_buffer = *d_iv,
          });
        }

        offscreen ofs{pd, create_gp};

        upc m_pc{};

        m_resized = false;
        while (!interrupted() && !m_resized) {
          m_pc.aspect = m_aspect;

          if (frag_mod() != frag_ts) {
            gp = create_gp(rp);
            frag_ts = frag_mod();
          }

          // Passing time in seconds
          float t = m_pc.time = 0.001 * watch.millis();

          // Fill grid
          if (m_g != nullptr) {
            vee::mapmem mm{*gg_mem};
            auto *buf = static_cast<storage *>(*mm);
            for (auto i = 0; i < max_cells; i++) {
              buf[i].seen = 0;
            }
            auto s = m_g->size();
            for (auto [x, y, p] : *m_g) {
              if (buf[p].first_seen == 0)
                buf[p].first_seen = t;
              buf[p].seen = s-- / static_cast<float>(m_g->size());
            }
            for (auto i = 0; i < max_cells; i++) {
              if (buf[i].seen == 0)
                buf[i].first_seen = 0;
            }
            auto [x, y, p] = m_g->food();
            if (m_pc.food != vec2{x, y}) {
              m_pc.party_start = t;
              m_pc.party = m_pc.food;
              m_pc.food = vec2{x, y};
            }
            if (m_g->is_new_game()) {
              m_pc.party = {1000, 1000};
              m_pc.party_start = 0;
              m_pc.dead_at = 0.0;
            } else if (m_pc.dead_at == 0.0)
              m_pc.dead_at = m_g->is_game_over() ? t : 0;

            m_pc.grid_width = m_g->grid_width();
            m_pc.grid_height = m_g->grid_height();
            m_g = nullptr;
          }

          // Flip
          vee::wait_and_reset_fence(*f);
          auto idx = vee::acquire_next_image(*swc, *img_available_sema);

          // Build command buffer
          vee::begin_cmd_buf_one_time_submit(cb);
          vee::cmd_begin_render_pass({
              .command_buffer = cb,
              .render_pass = *rp,
              .framebuffer = *fbs[idx],
              .extent = ext,
              .clear_color = {{0.01, 0.02, 0.05, 1.0}},
              .use_secondary_cmd_buf = false,
          });

          vee::cmd_set_scissor(cb, ext);
          vee::cmd_set_viewport(cb, ext);

          vee::cmd_push_vert_frag_constants(cb, *pl, &m_pc);
          vee::cmd_bind_gr_pipeline(cb, *gp);
          vee::cmd_bind_vertex_buffers(cb, 0, *qv_buf);
          vee::cmd_bind_descriptor_set(cb, *pl, 0, dset);
          vee::cmd_draw(cb, 6);

          vee::cmd_end_render_pass(cb);

          bool write = false;
          if (m_shots) {
            ofs.cmd_render_pass(cb, [&](auto ext) {
              m_pc.aspect = static_cast<float>(ext.width) /
                            static_cast<float>(ext.height);

              vee::cmd_set_scissor(cb, ext);
              vee::cmd_set_viewport(cb, ext);

              vee::cmd_push_vert_frag_constants(cb, *pl, &m_pc);
              vee::cmd_bind_vertex_buffers(cb, 0, *qv_buf);
              vee::cmd_bind_descriptor_set(cb, *pl, 0, dset);
              vee::cmd_draw(cb, 6);
            });
            write = true;
            m_shots = false;
          }
          vee::end_cmd_buf(cb);

          // Submit and present
          vee::queue_submit({
              .queue = q,
              .fence = *f,
              .command_buffer = cb,
              .wait_semaphore = *img_available_sema,
              .signal_semaphore = *rnd_finished_sema,
          });
          vee::queue_present({
              .queue = q,
              .swapchain = *swc,
              .wait_semaphore = *rnd_finished_sema,
              .image_index = idx,
          });

          if (write)
            ofs.write();
        }

        vee::device_wait_idle();
      } catch (vee::out_of_date_error) {
      }
    }
  }
};

extern "C" void casein_handle(const casein::event &e) {
  static thread t{};
  static hai::uptr<boa::game> g{};

  static constexpr auto reset = [](auto) {
    if (g->is_game_over()) {
      g = hai::uptr<boa::game>::make(g->grid_width(), g->grid_height());
      t.render(&*g);
    }
  };

  static constexpr auto g_map = [] {
    casein::subevent_map<casein::events::gesture, casein::G_MAX> res{};
    res[casein::G_SWIPE_UP] = [](auto) {
      g->up();
      t.render(&*g);
    };
    res[casein::G_SWIPE_DOWN] = [](auto) {
      g->down();
      t.render(&*g);
    };
    res[casein::G_SWIPE_LEFT] = [](auto) {
      g->left();
      t.render(&*g);
    };
    res[casein::G_SWIPE_RIGHT] = [](auto) {
      g->right();
      t.render(&*g);
    };
    res[casein::G_TAP_1] = reset;
    res[casein::G_SHAKE] = reset;
    return res;
  }();
  static constexpr auto k_map = [] {
    casein::subevent_map<casein::events::key_down, casein::K_MAX> res{};
    res[casein::K_UP] = [](auto) {
      g->up();
      t.render(&*g);
    };
    res[casein::K_DOWN] = [](auto) {
      g->down();
      t.render(&*g);
    };
    res[casein::K_LEFT] = [](auto) {
      g->left();
      t.render(&*g);
    };
    res[casein::K_RIGHT] = [](auto) {
      g->right();
      t.render(&*g);
    };
    res[casein::K_SPACE] = reset;
    res[casein::K_R] = [](auto) {
      t.take_shots();
      t.render(&*g); // just to bring the game back
    };
    return res;
  }();
  static constexpr auto map = [] {
    casein::event_map res{};
    res[casein::CREATE_WINDOW] = [](const casein::event &e) {
      t.start(*e.as<casein::events::create_window>());
    };
    res[casein::RESIZE_WINDOW] = [](const casein::event &e) {
      auto [w, h, _, __] = *e.as<casein::events::resize_window>();

      auto grid_h = 24.0f;
      auto grid_w = grid_h;
      if (w > h) {
        grid_w = grid_w * w / h;
      } else {
        grid_h = grid_h * h / w;
      }

      g = hai::uptr<boa::game>::make(static_cast<unsigned>(grid_w),
                                     static_cast<unsigned>(grid_h));
      t.resize(grid_w / grid_h);
      t.render(&*g);
    };
    res[casein::GESTURE] = [](auto e) { g_map.handle(e); };
    res[casein::KEY_DOWN] = [](auto e) { k_map.handle(e); };
    res[casein::TOUCH_UP] = reset;
    res[casein::REPAINT] = [](auto) {
      if (g && g->tick())
        t.render(&*g);
    };
    res[casein::QUIT] = [](auto) { t.stop(); };
    return res;
  }();

  map.handle(e);
}

#pragma leco add_shader "boav.vert"
#pragma leco add_shader "boav.frag"
