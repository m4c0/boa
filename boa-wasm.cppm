export module boa:wasm;
import :render;
import casein;
import hai;

namespace boa {
class r_pimpl {};
renderer::renderer() : m_data{hai::uptr<r_pimpl>::make()} {}
renderer::~renderer() {}
void renderer::setup(casein::native_handle_t) {}
void renderer::update(const ecs::grid &) {}
void renderer::repaint() {}
void renderer::quit() {}
} // namespace boa
