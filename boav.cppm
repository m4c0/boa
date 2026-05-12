#pragma leco add_impl impls
module;
extern "C" {
#include "sfx.h"
#include "snd.h"
#include "snk.h"
#include "tmr.h"
}

export module boav;
#ifndef LECO_TARGET_IPHONEOS
import :offscreen;
#endif
import boa;
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

struct ivec2 {
  unsigned x, y;
};
struct upc {
  float aspect;
  float time;
  float dead_at;
  float pad{};
  float grid_width = 24;
  float grid_height = 24;
  ivec2 food = { 10000 };
  ivec2 party = { 10000 };
  float party_start = -1;
};

struct storage {
  float first_seen;
  float seen;
};

hai::uptr<boa::game> g_g {};
upc g_pc{};
snk_outcome_t volatile g_outcome{};
VkDeviceMemory g_mem {};

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
      vee::vert_frag_push_constant_range<upc>()
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
    constexpr const unsigned sz = max_cells * sizeof(storage);
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

    while (!interrupted()) {
      voo::swapchain_and_stuff sw { dq, *rp };

      extent_loop(dq.queue(), sw, [&] {
        g_pc.aspect = sw.aspect();

        // Passing time in seconds
        g_pc.time = 0.001 * watch.millis();

        const auto render = [&](auto cb, auto ext) {
          g_pc.aspect = static_cast<float>(ext.width) /
                        static_cast<float>(ext.height);

          vee::cmd_set_scissor(cb, ext);
          vee::cmd_set_viewport(cb, ext);

          vee::cmd_push_vert_frag_constants(cb, *pl, &g_pc);
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

static void update_first_seen(storage * buf) {
  auto s = snk_size;
  auto p = snk_head;
  while (p != -1) {
    if (buf[p].first_seen == 0) buf[p].first_seen = g_pc.time;
    buf[p].seen = s-- / static_cast<float>(snk_size);
    p = snk_next(p);
  }
}
static void update_grid() {
  voo::mapmem m { g_mem };
  auto buf = static_cast<storage *>(*m);

  for (auto i = 0; i < max_cells; i++) buf[i].seen = 0;
  update_first_seen(buf);

  for (auto i = 0; i < max_cells; i++) {
    if (buf[i].seen == 0) buf[i].first_seen = 0;
  }

  unsigned x = snk_food % (unsigned)g_pc.grid_width;
  unsigned y = snk_food / (unsigned)g_pc.grid_width;
  if (g_pc.food.x != x || g_pc.food.y != y) {
    if (g_outcome == snk_o_eat_food) {
      sfx_eat();
      g_pc.party_start = g_pc.time;
      g_pc.party = g_pc.food;
    }
    g_pc.food = { x, y };
  }
  if (snk_is_new()) {
    sfx_reset();
    g_pc.party = {1000, 1000};
    g_pc.party_start = 0;
    g_pc.dead_at = 0.0;
  } else if (g_pc.dead_at == 0.0)
    g_pc.dead_at = snk_is_over() ? g_pc.time : 0;

  if (g_outcome == snk_o_move ) sfx_walk();
  if (g_outcome == snk_o_death) sfx_death();

  g_pc.grid_width  = snk_grid_w;
  g_pc.grid_height = snk_grid_h;
}

static void reset() {
  if (snk_is_over()) {
    g_g = hai::uptr<boa::game>::make(snk_grid_w, snk_grid_h);
    update_grid();
  }
}
static void up() {
  g_outcome = g_g->up();
  update_grid();
}
static void down() {
  g_outcome = g_g->down();
  update_grid();
}
static void left() {
  g_outcome = g_g->left();
  update_grid();
}
static void right() {
  g_outcome = g_g->right();
  update_grid();
}

static void tick() {
  if (!g_g || !g_mem) return;
  g_outcome = g_g->tick();
  if (g_outcome != snk_o_none) update_grid();
}

struct init {
  init() {
    using namespace casein;

    tmr_fn = &tick;

    sfx_init();

    handle(GESTURE, G_SWIPE_UP, up);
    handle(GESTURE, G_SWIPE_DOWN, down);
    handle(GESTURE, G_SWIPE_LEFT, left);
    handle(GESTURE, G_SWIPE_RIGHT, right);
    handle(GESTURE, G_TAP_1, reset);
    handle(GESTURE, G_SHAKE, reset);

    handle(KEY_DOWN, K_UP, up);
    handle(KEY_DOWN, K_DOWN, down);
    handle(KEY_DOWN, K_LEFT, left);
    handle(KEY_DOWN, K_RIGHT, right);
    handle(KEY_DOWN, K_SPACE, reset);
    
#ifndef LECO_TARGET_IPHONEOS
    handle(KEY_DOWN, K_R, [] {
      t.take_shots();
      // t.render(&*g, {}); // just to bring the game back
    });
#endif

    handle(RESIZE_WINDOW, [] {
      auto [w, h] = casein::window_size;
      auto grid_h = 24.0f;
      auto grid_w = grid_h;
      if (w > h) {
        grid_w = grid_w * w / h;
      } else {
        grid_h = grid_h * h / w;
      }

      g_g = hai::uptr<boa::game>::make(static_cast<unsigned>(grid_w), static_cast<unsigned>(grid_h));
      if (g_mem) update_grid();
    });

    handle(TOUCH_UP, reset);

    snd_init(&sfx_fill);
  }
  ~init() {
    snd_deinit();
    tmr_deinit();
  }
} i;

#pragma leco add_shader "boav.vert"
#pragma leco add_shader "boav.frag"
