module;
#include <stdio.h>

export module boav:offscreen;
import hai;
import stubby;
import vee;
import voo;

constexpr const auto sizes_len = 12;
constexpr const vee::extent sizes[sizes_len]{
    {1290, 2796}, {1284, 2778}, {1179, 2556}, {1170, 2532},
    {1242, 2208}, {750, 1334},  {640, 1136},  {640, 960},
    {2048, 2732}, {1668, 2388}, {1668, 2224}, {1536, 2048},
};

class ofs_ext {
  vee::physical_device pd;
  vee::extent ext;

  voo::offscreen::colour_buffer m_colour { pd, ext };
  voo::offscreen::depth_buffer m_depth { pd, ext };
  voo::offscreen::host_buffer m_host { pd, ext };

  // Render pass + Framebuffer + Pipeline
  vee::render_pass rp = vee::create_render_pass(pd, nullptr);
  vee::framebuffer fb = vee::create_framebuffer({
      .physical_device = pd,
      .render_pass = *rp,
      .attachments = {{ m_colour.image_view(), m_depth.image_view() }},
      .extent = ext,
  });
  vee::gr_pipeline gp{};

public:
  ofs_ext(vee::physical_device pd, vee::extent ext, auto &&create_gp)
      : pd{pd}, ext{ext} {
    gp = create_gp(*rp);
  }

  void cmd_render_pass(vee::command_buffer cb, auto &&blk) {
    vee::cmd_begin_render_pass({
        .command_buffer = cb,
        .render_pass = *rp,
        .framebuffer = *fb,
        .extent = ext,
        .clear_color = {{0.01, 0.02, 0.05, 1.0}},
        .use_secondary_cmd_buf = false,
    });
    vee::cmd_bind_gr_pipeline(cb, *gp);
    blk(cb, ext);
    vee::cmd_end_render_pass(cb);

    vee::cmd_pipeline_barrier(cb, m_colour.image(), vee::from_pipeline_to_host);
    vee::cmd_copy_image_to_buffer(cb, ext, m_colour.image(), m_host.buffer());
  }

  void write() {
    char filename[1024];
    snprintf(filename, 1024, "out/shot-%dx%d.jpg", ext.width, ext.height);

    auto mem = m_host.map();
    auto *data = static_cast<stbi::pixel *>(*mem);
    stbi::write_rgba_unsafe(filename, ext.width, ext.height, data);
  }
};

class offscreen : voo::update_thread {
  hai::uptr<ofs_ext> m_oe[sizes_len];
  hai::fn<void, vee::command_buffer, vee::extent> m_fn;

  void build_cmd_buf(vee::command_buffer cb) override {
    voo::cmd_buf_one_time_submit pcb{cb};
    for (auto &oe : m_oe) oe->cmd_render_pass(cb, m_fn);
  }

public:
  explicit offscreen(vee::physical_device pd, voo::queue * q, auto &&create_gp) : update_thread { q } {
    for (auto i = 0; i < sizes_len; i++) {
      m_oe[i] = hai::uptr<ofs_ext>::make(pd, sizes[i], create_gp);
    }
  }

  void do_it(hai::fn<void, vee::command_buffer, vee::extent> fn) {
    m_fn = fn;
    run_once();

    // Sync CPU+GPU
    vee::device_wait_idle();
    for (auto &oe : m_oe) oe->write();
  }
};
