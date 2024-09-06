export module boav;
#ifndef LECO_TARGET_IPHONEOS
import :offscreen;
#endif
import beeps;
import boa;
import casein;
import dotz;
import hai;
import silog;
import sires;
import sith;
import sitime;
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
  float grid_width;
  float grid_height;
  dotz::ivec2 food;
  dotz::ivec2 party;
  float party_start;
};

struct storage {
  float first_seen;
  float seen;
};
hai::uptr<boa::game> g_g {};
upc g_pc{};
boa::outcome volatile g_outcome{};
beeps beep{};
voo::updater<voo::h2l_buffer> * g_buffer {};

auto frag_mod() {
  return sires::stat("boav.frag.spv").take([](auto err) {
    silog::log(silog::error, "Failed to stat shader: %s", err.cstr().begin());
    return 0;
  });
}

static void update_grid(voo::h2l_buffer * gg) {
  voo::mapmem m { gg->host_memory() };
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

class thread : public voo::casein_thread {
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
      vee::shader_module vert = vee::create_shader_module_from_resource("boav.vert.spv");
      vee::shader_module frag = vee::create_shader_module_from_resource("boav.frag.spv");
      return vee::create_graphics_pipeline({
        .pipeline_layout = *pl,
        .render_pass = rp,
        .shaders {
          vee::pipeline_vert_stage(*vert, "main"),
          vee::pipeline_frag_stage(*frag, "main"),
        },
        .bindings {
          vee::vertex_input_bind(2 * sizeof(float)),
        },
        .attributes{
          vee::vertex_attribute_vec2(0, 0),
        },
      });
    };
    vee::gr_pipeline gp = create_gp(dq.render_pass());

    // offscreen ofs { dq.physical_device(), create_gp };

    while (!interrupted()) {
      voo::swapchain_and_stuff sw { dq };

      // Game grid buffer
      constexpr const unsigned gg_buf_size = max_cells * sizeof(storage);
      voo::updater gg_buf { dq.queue(), &update_grid, dq, gg_buf_size };
      vee::update_descriptor_set_with_storage(dset, 0, gg_buf.data().local_buffer());
      g_buffer = &gg_buf;

      extent_loop(dq.queue(), sw, [&] {
        g_pc.aspect = sw.aspect();;

        if (frag_mod() != frag_ts) {
          gp = create_gp(dq.render_pass());
          frag_ts = frag_mod();
        }

        // Passing time in seconds
        g_pc.time = 0.001 * watch.millis();

        sw.queue_one_time_submit(dq.queue(), [&](auto pcb) {
          auto scb = sw.cmd_render_pass(pcb);
          auto ext = sw.extent();

          vee::cmd_set_scissor(*scb, ext);
          vee::cmd_set_viewport(*scb, ext);

          vee::cmd_push_vert_frag_constants(*scb, *pl, &g_pc);
          vee::cmd_bind_gr_pipeline(*scb, *gp);
          vee::cmd_bind_descriptor_set(*scb, *pl, 0, dset);
          quad.run(*scb, 0, 1);
        });

        /*
        bool write = false;
        if (m_shots) {
          ofs.cmd_render_pass(cb, [&](auto ext) {
            g_pc.aspect = static_cast<float>(ext.width) /
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
        if (write) ofs.write();
        */
      });
    }
  }
} t;

static void reset() {
  if (g_g->is_game_over()) {
    g_g = hai::uptr<boa::game>::make(g_g->grid_width(), g_g->grid_height());
    g_buffer->run_once();
  }
}
static constexpr auto render(boa::outcome (boa::game::*m)(void)) {
  return [=] {
    g_outcome = ((*g_g).*m)();
    g_buffer->run_once();
  };
}

struct init {
  init() {
    using namespace casein;

    handle(GESTURE, G_SWIPE_UP, render(&boa::game::up));
    handle(GESTURE, G_SWIPE_DOWN, render(&boa::game::down));
    handle(GESTURE, G_SWIPE_LEFT, render(&boa::game::left));
    handle(GESTURE, G_SWIPE_RIGHT, render(&boa::game::right));
    handle(GESTURE, G_TAP_1, reset);
    handle(GESTURE, G_SHAKE, reset);

    handle(KEY_DOWN, K_UP, render(&boa::game::up));
    handle(KEY_DOWN, K_DOWN, render(&boa::game::down));
    handle(KEY_DOWN, K_LEFT, render(&boa::game::left));
    handle(KEY_DOWN, K_RIGHT, render(&boa::game::right));
    handle(KEY_DOWN, K_SPACE, reset);
    // handle(KEY_DOWN, K_R, [] {
    //   t.take_shots();
    //   t.render(&*g, {}); // just to bring the game back
    // });

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
      if (g_buffer) g_buffer->run_once();
    });

    handle(TIMER, [] {
      g_outcome = g_g->tick();
      if (g_outcome != boa::outcome::none) g_buffer->run_once();
    });

    handle(TOUCH_UP, reset);
  }
} i;

#pragma leco add_shader "boav.vert"
#pragma leco add_shader "boav.frag"
