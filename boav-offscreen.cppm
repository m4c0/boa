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
  voo::offscreen::buffers m_bufs;
  vee::gr_pipeline m_gp;

public:
  ofs_ext(vee::physical_device pd, vee::extent ext, auto &&create_gp)
      : m_bufs { ext, VK_FORMAT_R8G8B8A8_SRGB }
      , m_gp { create_gp(m_bufs.render_pass()) } {}

  void cmd_render_pass(vee::command_buffer cb, auto &&blk) {
    vee::cmd_begin_render_pass(m_bufs.render_pass_begin({
        .command_buffer = cb,
        .clear_colours = { vee::clear_colour(0.01, 0.02, 0.05, 1.0) },
    }));
    vee::cmd_bind_gr_pipeline(cb, *m_gp);
    blk(cb, m_bufs.extent());
    vee::cmd_end_render_pass(cb);

    m_bufs.cmd_copy_to_host(cb);
  }

  void write() {
    auto [w, h] = m_bufs.extent();

    char filename[1024];
    snprintf(filename, 1024, "out/shot-%dx%d.jpg", w, h);

    auto mem = m_bufs.map_host();
    auto *data = static_cast<stbi::pixel *>(*mem);
    stbi::write_rgba_unsafe(filename, w, h, data);
  }
};

class offscreen {
  hai::uptr<ofs_ext> m_oe[sizes_len];
  voo::single_cb m_cb {};

public:
  explicit offscreen(vee::physical_device pd, auto && create_gp) {
    for (auto i = 0; i < sizes_len; i++) {
      m_oe[i] = hai::uptr<ofs_ext>::make(pd, sizes[i], create_gp);
    }
  }

  void do_it(hai::fn<void, vee::command_buffer, vee::extent> fn) {
    auto cb = m_cb.cb();
    {
      voo::cmd_buf_one_time_submit pcb { cb };
      for (auto &oe : m_oe) oe->cmd_render_pass(cb, fn);
    }
    voo::queue::universal()->queue_submit({ .command_buffer = cb });

    // Sync CPU+GPU
    vee::device_wait_idle();
    for (auto &oe : m_oe) oe->write();
  }
};
