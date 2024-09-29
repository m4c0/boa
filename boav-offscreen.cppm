module;
#include <stdio.h>

export module boav:offscreen;
import hai;
import stubby;
import vee;

constexpr const auto sizes_len = 12;
constexpr const vee::extent sizes[sizes_len]{
    {1290, 2796}, {1284, 2778}, {1179, 2556}, {1170, 2532},
    {1242, 2208}, {750, 1334},  {640, 1136},  {640, 960},
    {2048, 2732}, {1668, 2388}, {1668, 2224}, {1536, 2048},
};

class ofs_ext {
  vee::physical_device pd;
  vee::extent ext;

  // Colour buffer
  vee::image t_img = vee::create_renderable_image(ext);
  vee::device_memory t_mem = vee::create_local_image_memory(pd, *t_img);
  [[maybe_unused]] decltype(nullptr) t_bind =
      vee::bind_image_memory(*t_img, *t_mem);
  vee::image_view t_iv = vee::create_srgba_image_view(*t_img);

  // Depth buffer
  vee::image d_img = vee::create_depth_image(ext);
  vee::device_memory d_mem = vee::create_local_image_memory(pd, *d_img);
  [[maybe_unused]] decltype(nullptr) d_bind =
      vee::bind_image_memory(*d_img, *d_mem);
  vee::image_view d_iv = vee::create_depth_image_view(*d_img);

  // Host-readable output buffer
  vee::buffer o_buf =
      vee::create_transfer_dst_buffer(ext.width * ext.height * 4);
  vee::device_memory o_mem = vee::create_host_buffer_memory(pd, *o_buf);
  [[maybe_unused]] decltype(nullptr) o_bind =
      vee::bind_buffer_memory(*o_buf, *o_mem);

  // Render pass + Framebuffer + Pipeline
  vee::render_pass rp = vee::create_render_pass(pd, nullptr);
  vee::framebuffer fb = vee::create_framebuffer({
      .physical_device = pd,
      .render_pass = *rp,
      .image_buffer = *t_iv,
      .depth_buffer = *d_iv,
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
    blk(ext);
    vee::cmd_end_render_pass(cb);

    vee::cmd_pipeline_barrier(cb, *t_img, vee::from_pipeline_to_host);
    vee::cmd_copy_image_to_buffer(cb, ext, *t_img, *o_buf);
  }

  void write() {
    char filename[1024];
    snprintf(filename, 1024, "out/shot-%dx%d.jpg", ext.width, ext.height);

    auto mem = vee::map_memory(*o_mem);
    auto *data = static_cast<stbi::pixel *>(mem);
    stbi::write_rgba_unsafe(filename, ext.width, ext.height, data);
    vee::unmap_memory(*o_mem);
  }
};

class offscreen {
  hai::uptr<ofs_ext> m_oe[sizes_len];

public:
  explicit offscreen(vee::physical_device pd, auto &&create_gp) {
    for (auto i = 0; i < sizes_len; i++) {
      m_oe[i] = hai::uptr<ofs_ext>::make(pd, sizes[i], create_gp);
    }
  }

  void cmd_render_pass(vee::command_buffer cb, auto &&blk) {
    for (auto &oe : m_oe) {
      oe->cmd_render_pass(cb, blk);
    }
  }

  void write() {
    // Sync CPU+GPU
    vee::device_wait_idle();
    for (auto &oe : m_oe) {
      oe->write();
    }
  }
};
