#pragma leco add_impl impls
module;
extern "C" {
#include "gme.h"
#include "sfx.h"
#include "snd.h"
#include "snk.h"
#include "tmr.h"
}

export module boav;
#ifndef LECO_TARGET_IPHONEOS
import :offscreen;
#endif
import casein;
import hai;
import sitime;
import vapp;
import vee;
import voo;

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

VkDeviceMemory g_mem {};

static void update_grid(snk_outcome_t outcome) {
  voo::mapmem m { g_mem };
  gme_update((gme_storage_t *)*m, outcome);
}

class thread : public vapp {
  volatile bool m_shots;

public:
  void take_shots() { m_shots = true; }

  void run() override {
    sitime::stopwatch watch{};

    voo::device_and_queue dq { "boas", casein::native_ptr };
    auto rp = voo::single_att_render_pass(dq);

    // Descriptor set layout + pool + set
    vee::descriptor_set_layout dsl = vee::create_descriptor_set_layout({
      vee::dsl_fragment_storage(),
    });
    vee::descriptor_pool dp = vee::create_descriptor_pool(1, {vee::storage_buffer()});
    vee::descriptor_set dset = vee::allocate_descriptor_set(*dp, *dsl);

    // Pipeline
    vee::pipeline_layout pl = vee::create_pipeline_layout(
      *dsl,
      vee::vert_frag_push_constant_range<gme_upc_t>()
    );
    const auto create_gp = [&](vee::render_pass::type rp) {
      return vee::create_graphics_pipeline({
        .pipeline_layout = *pl,
        .render_pass = rp,
        .back_face_cull = false,
        .shaders {
          *voo::vert_shader("boav.vert.spv"),
          *voo::frag_shader("boav.frag.spv"),
        },
      });
    };
    vee::gr_pipeline gp = create_gp(*rp);

#ifndef LECO_TARGET_IPHONEOS
    offscreen ofs { dq.physical_device(), create_gp };
#endif

    // Game grid buffer
    constexpr const unsigned sz = max_cells * sizeof(gme_storage_t);
    vee::buffer buf = vee::create_buffer(sz, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    vee::device_memory mem = vee::create_host_buffer_memory(dq.physical_device(), *buf);
    vee::bind_buffer_memory(*buf, *mem);

    auto bi = vee::descriptor_buffer_info(*buf);
    vee::update_descriptor_set(vee::write_descriptor_set({
      .dstSet = dset,
      .descriptorCount = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      .pBufferInfo = &bi,
    }));
    g_mem = *mem;
    unsigned scr_w = 0, scr_h = 0;

    while (!interrupted()) {
      voo::swapchain_and_stuff sw { dq, *rp };

      extent_loop(dq.queue(), sw, [&] {
        auto [w, h] = casein::window_size;
        if (scr_w != w || scr_h != h) {
          scr_w = w; scr_h = h;
          update_grid(gme_resize(w, h));
        }

        gme_pc.aspect = sw.aspect();

        // Passing time in seconds
        gme_pc.time = 0.001 * watch.millis();

        const auto render = [&](auto cb, auto ext) {
          gme_pc.aspect = (float)ext.width / (float)ext.height;

          vee::cmd_set_scissor(cb, ext);
          vee::cmd_set_viewport(cb, ext);

          vee::cmd_push_vert_frag_constants(cb, *pl, &gme_pc);
          vee::cmd_bind_gr_pipeline(cb, *gp);
          vee::cmd_bind_descriptor_set(cb, *pl, 0, dset);
          vkCmdDraw(cb, 3, 1, 0, 0);
        };

        sw.queue_one_time_submit([&] {
          auto scb = sw.cmd_render_pass({});
          auto ext = sw.extent();
          render((VkCommandBuffer) scb, ext);
        });

#ifndef LECO_TARGET_IPHONEOS
        if (m_shots) {
          ofs.do_it(render);
          m_shots = false;
        }
#endif
      });
    }
  }
} t;

static void new_game() {
  update_grid(gme_new_game());
}
static void up() {
  update_grid(snk_update_dir(snk_d_u));
}
static void down() {
  update_grid(snk_update_dir(snk_d_d));
}
static void left() {
  update_grid(snk_update_dir(snk_d_l));
}
static void right() {
  update_grid(snk_update_dir(snk_d_r));
}

static void tick() {
  if (!g_mem) return;
  update_grid(snk_run_tick());
}

struct init {
  init() {
    using namespace casein;

    handle(GESTURE, G_SWIPE_UP, up);
    handle(GESTURE, G_SWIPE_DOWN, down);
    handle(GESTURE, G_SWIPE_LEFT, left);
    handle(GESTURE, G_SWIPE_RIGHT, right);
    handle(GESTURE, G_TAP_1, new_game);
    handle(GESTURE, G_SHAKE, new_game);

    handle(KEY_DOWN, K_UP, up);
    handle(KEY_DOWN, K_DOWN, down);
    handle(KEY_DOWN, K_LEFT, left);
    handle(KEY_DOWN, K_RIGHT, right);
    handle(KEY_DOWN, K_SPACE, new_game);
    
#ifndef LECO_TARGET_IPHONEOS
    handle(KEY_DOWN, K_R, [] {
      t.take_shots();
      // t.render(&*g, {}); // just to bring the game back
    });
#endif

    handle(TOUCH_UP, new_game);

    tmr_fn = &tick;

    sfx_init();
    snd_init(&sfx_fill);
  }
  ~init() {
    snd_deinit();
    tmr_deinit();
  }
} i;

#pragma leco add_shader "boav.vert"
#pragma leco add_shader "boav.frag"
