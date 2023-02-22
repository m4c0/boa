export module boa:pipeline;
import :ecs_objects;
import :vulkan;
import vee;

namespace boa::vulkan {
class pipeline {
  const per_device *dev;
  const per_extent *ext;

  vee::pipeline_layout pl = vee::create_pipeline_layout();

  vee::shader_module vert =
      vee::create_shader_module_from_resource("main.vert.spv");
  vee::shader_module frag =
      vee::create_shader_module_from_resource("main.frag.spv");
  vee::gr_pipeline gp = vee::create_graphics_pipeline(
      *pl, ext->render_pass(),
      {
          vee::pipeline_vert_stage(*vert, "main"),
          vee::pipeline_frag_stage(*frag, "main"),
      },
      {
          vee::vertex_input_bind(sizeof(ecs::point)),
      },
      {
          vee::vertex_attribute_vec2(0, 0),
      });

  static constexpr const auto v_max = 1024;
  vee::buffer v_buf = vee::create_vertex_buffer(sizeof(ecs::point) * v_max);
  vee::device_memory v_mem =
      vee::create_host_buffer_memory(dev->physical_device(), *v_buf);
  decltype(nullptr) v_bind = vee::bind_buffer_memory(*v_buf, *v_mem);

  unsigned v_count{};

public:
  explicit pipeline(const per_device *d, const per_extent *e)
      : dev{d}, ext{e} {}

  void build_commands(vee::command_buffer cb) const {
    const auto extent = ext->extent_2d();

    vee::begin_cmd_buf_render_pass_continue(cb, ext->render_pass());
    vee::cmd_set_scissor(cb, extent);
    vee::cmd_set_viewport(cb, extent);
    vee::cmd_bind_gr_pipeline(cb, *gp);
    vee::cmd_bind_vertex_buffers(cb, 0, *v_buf);
    vee::cmd_draw(cb, v_count);
    vee::end_cmd_buf(cb);
  }

  void map_vertices(auto fn) {
    vee::map_memory<boa::ecs::point>(*v_mem, [&](auto p) { v_count = fn(p); });
  }
};
} // namespace boa::vulkan
