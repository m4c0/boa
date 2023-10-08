export module boav:offscreen;
import vee;

constexpr const vee::extent sizes[]{
    {1290, 2796}, {1284, 2778}, {1179, 2556}, {1170, 2532},
    {1242, 2208}, {750, 1334},  {640, 1136},  {640, 960},
    {2048, 2732}, {1668, 2388}, {1668, 2224}, {1536, 2048},
};

class ofs_ext {
  vee::physical_device pd;
  vee::render_pass::type rp;
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

  // Framebuffer
  vee::framebuffer fb = vee::create_framebuffer({
      .physical_device = pd,
      .render_pass = rp,
      .image_buffer = *t_iv,
      .depth_buffer = *d_iv,
      .extent = ext,
  });

public:
  ofs_ext(vee::physical_device pd, const vee::render_pass &rp, vee::extent ext)
      : pd{pd}, rp{*rp}, ext{ext} {}
};

class offscreen {
  vee::physical_device pd;

  vee::render_pass rp = vee::create_render_pass(pd, nullptr);
  vee::gr_pipeline gp{};

public:
  explicit offscreen(vee::physical_device pd) : pd{pd} {}
  void cmd_begin_render_pass(vee::command_buffer cb) {}
};
