export module boav;
#ifndef LECO_TARGET_IPHONEOS
import :offscreen;
#endif
import beeps;
import boa;
import casein;
import dotz;
import fff;
import hai;
import siaudio;
import silog;
import sires;
import sith;
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

struct upc {
  float aspect;
  float time;
  float dead_at;
  float pad{};
  float grid_width = 24;
  float grid_height = 24;
  dotz::ivec2 food = { 10000 };
  dotz::ivec2 party = { 10000 };
  float party_start = -1;
};

struct v_buffer {
  vee::buffer buf {};
  vee::device_memory mem {};

  constexpr v_buffer() = default;
  v_buffer(const voo::device_and_queue & dq, unsigned sz) {
    buf = vee::create_storage_buffer(sz);
    mem = vee::create_host_buffer_memory(dq.physical_device(), sz);
    vee::bind_buffer_memory(*buf, *mem);
  }
};

struct storage {
  float first_seen;
  float seen;
};
hai::uptr<boa::game> g_g {};
upc g_pc{};
boa::outcome volatile g_outcome{};
beeps beep{};
v_buffer * g_buffer {};

auto frag_mod() {
  return sires::stat("boav.frag.spv").take([](auto err) {
    silog::log(silog::error, "Failed to stat shader: %s", err.cstr().begin());
    return 0;
  });
}

class thread : public vapp {
  volatile bool m_shots;

public:
  void take_shots() { m_shots = true; }

  void run() override {
    sitime::stopwatch watch{};

    voo::device_and_queue dq { "boas" };
    voo::one_quad quad { dq };

    // Descriptor set layout + pool + set
    vee::descriptor_set_layout dsl = vee::create_descriptor_set_layout({
      vee::dsl_fragment_storage(),
    });
    vee::descriptor_pool dp = vee::create_descriptor_pool(1, {vee::storage_buffer()});
    vee::descriptor_set dset = vee::allocate_descriptor_set(*dp, *dsl);

    // Pipeline
    vee::pipeline_layout pl = vee::create_pipeline_layout(
      { *dsl },
      { vee::vert_frag_push_constant_range<upc>() }
    );
    long frag_ts{};
    const auto create_gp = [&](vee::render_pass::type rp) {
      return vee::create_graphics_pipeline({
        .pipeline_layout = *pl,
        .render_pass = rp,
        .shaders {
          voo::shader("boav.vert.spv").pipeline_vert_stage(),
          voo::shader("boav.frag.spv").pipeline_frag_stage(),
        },
        .bindings { quad.vertex_input_bind() },
        .attributes { quad.vertex_attribute(0) },
      });
    };
    vee::gr_pipeline gp = create_gp(dq.render_pass());

#ifndef LECO_TARGET_IPHONEOS
    offscreen ofs { dq.physical_device(), dq.queue(), create_gp };
#endif

    // Game grid buffer
    constexpr const unsigned gg_buf_size = max_cells * sizeof(storage);
    v_buffer gg_buf { dq, gg_buf_size };
    vee::update_descriptor_set_with_storage(dset, 0, *gg_buf.buf);
    g_buffer = &gg_buf;
    release_init_lock();

    while (!interrupted()) {
      voo::swapchain_and_stuff sw { dq };

      extent_loop(dq.queue(), sw, [&] {
        g_pc.aspect = sw.aspect();

        if (frag_mod() != frag_ts) {
          gp = create_gp(dq.render_pass());
          frag_ts = frag_mod();
        }

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
          quad.run(cb, 0, 1);
        };

        sw.queue_one_time_submit(dq.queue(), [&](auto pcb) {
          auto scb = sw.cmd_render_pass({ *pcb });
          auto ext = sw.extent();
          render(*scb, ext);
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

  using vapp::wait_init;
} t;

static void update_grid() {
  t.wait_init();

  voo::mapmem m { *g_buffer->mem };
  auto buf = static_cast<storage *>(*m);

  for (auto i = 0; i < max_cells; i++) buf[i].seen = 0;

  auto s = g_g->size();
  for (auto [x, y, p] : *g_g) {
    if (buf[p].first_seen == 0) buf[p].first_seen = g_pc.time;
    buf[p].seen = s-- / static_cast<float>(g_g->size());
  }

  for (auto i = 0; i < max_cells; i++) {
    if (buf[i].seen == 0) buf[i].first_seen = 0;
  }

  auto [x, y, p] = g_g->food();
  if (g_pc.food != dotz::ivec2 { x, y }) {
    if (g_outcome == boa::outcome::eat_food) {
      beep.eat();
      g_pc.party_start = g_pc.time;
      g_pc.party = g_pc.food;
    }
    g_pc.food = { x, y };
  }
  if (g_g->is_new_game()) {
    beep.reset();
    g_pc.party = {1000, 1000};
    g_pc.party_start = 0;
    g_pc.dead_at = 0.0;
  } else if (g_pc.dead_at == 0.0)
    g_pc.dead_at = g_g->is_game_over() ? g_pc.time : 0;

  if (g_outcome == boa::outcome::move) {
    beep.walk();
  }
  if (g_outcome == boa::outcome::death) {
    beep.death();
  }

  g_pc.grid_width = g_g->grid_width();
  g_pc.grid_height = g_g->grid_height();
}

static void reset() {
  if (g_g->is_game_over()) {
    g_g = hai::uptr<boa::game>::make(g_g->grid_width(), g_g->grid_height());
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
  if (!g_g) return;
  g_outcome = g_g->tick();
  if (g_outcome != boa::outcome::none) update_grid();
}

struct init {
  fff::timer ticker { 25, tick };

  init() {
    using namespace casein;

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
      if (g_buffer) update_grid();
    });

    handle(TOUCH_UP, reset);

    siaudio::filler([](float * f, unsigned n) { beep.fill_buffer(f, n); });
    siaudio::rate(44100);
  }
} i;

#pragma leco add_shader "boav.vert"
#pragma leco add_shader "boav.frag"
