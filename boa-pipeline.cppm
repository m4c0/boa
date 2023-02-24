export module boa:pipeline;
import :ecs_objects;
import :vulkan;
import vee;

namespace boa::vulkan {
template <typename Tp, unsigned Max> class bound_buffer {
  const per_device *m_dev;

  vee::buffer m_buf = vee::create_vertex_buffer(sizeof(Tp) * Max);
  vee::device_memory m_mem =
      vee::create_host_buffer_memory(m_dev->physical_device(), *m_buf);
  decltype(nullptr) m_bind = vee::bind_buffer_memory(*m_buf, *m_mem);

public:
  explicit bound_buffer(const per_device *d) : m_dev{d} {}

  [[nodiscard]] auto operator*() const noexcept { return *m_buf; }

  void map(auto fn) { vee::map_memory<Tp>(*m_mem, fn); }
};
struct pcs {
  ecs::xy grid_pos{ecs::grid_w / 2, ecs::grid_h / 2};
  ecs::xy grid_size{ecs::grid_w / 2, ecs::grid_h / 2};
};
class pipeline {
  const per_device *dev;
  const per_extent *ext;

  vee::pipeline_layout pl = vee::create_pipeline_layout({
      vee::vertex_push_constant_range<pcs>(),
  });

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
          vee::vertex_input_bind(sizeof(ecs::xy)),
          vee::vertex_input_bind_per_instance(sizeof(ecs::xy)),
          vee::vertex_input_bind_per_instance(sizeof(ecs::rgba)),
      },
      {
          vee::vertex_attribute_vec2(0, 0),
          vee::vertex_attribute_vec2(1, 0),
          vee::vertex_attribute_vec2(2, 0),
      });

  static constexpr const auto v_count = 6;
  static constexpr const auto i_count = ecs::grid_cells;

  bound_buffer<ecs::xy, v_count> vertices{dev};
  bound_buffer<ecs::xy, i_count> instance_pos{dev};
  bound_buffer<ecs::rgba, i_count> instance_colour{dev};

  void map_vertices() {
    vertices.map([](auto vs) {
      vs[0] = {0, 0};
      vs[1] = {1, 1};
      vs[2] = {1, 0};

      vs[3] = {1, 1};
      vs[4] = {0, 0};
      vs[5] = {0, 1};
    });
  }
  void map_instances_pos() {
    instance_pos.map([](auto is) {
      unsigned i = 0;
      for (auto y = 0; y < ecs::grid_h; y++) {
        for (auto x = 0; x < ecs::grid_w; x++, i++) {
          is[i].x = x;
          is[i].y = y;
        }
      }
    });
  }

public:
  explicit pipeline(const per_device *d, const per_extent *e) : dev{d}, ext{e} {
    map_vertices();
    map_instances_pos();
  }

  void map_instances_colour(auto fn) { instance_colour.map(fn); }

  void build_commands(vee::command_buffer cb) const {
    const auto extent = ext->extent_2d();
    constexpr const pcs pc{};

    vee::begin_cmd_buf_render_pass_continue(cb, ext->render_pass());
    vee::cmd_set_scissor(cb, extent);
    vee::cmd_set_viewport(cb, extent);
    vee::cmd_bind_gr_pipeline(cb, *gp);
    vee::cmd_bind_vertex_buffers(cb, 0, *vertices);
    vee::cmd_bind_vertex_buffers(cb, 1, *instance_pos);
    vee::cmd_bind_vertex_buffers(cb, 2, *instance_colour);
    vee::cmd_push_vertex_constants(cb, *pl, &pc);
    vee::cmd_draw(cb, v_count, i_count);
    vee::end_cmd_buf(cb);
  }
};
} // namespace boa::vulkan
