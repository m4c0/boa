#include "../casein/build.hpp"
#include "../ecow/ecow.hpp"
#include "../hai/build.hpp"
#include "../sires/build.hpp"
#include "../traits/build.hpp"
#include "../vee/build.hpp"

int main(int argc, char **argv) {
  using namespace ecow;

  constexpr const auto add_deps = [](app &b) {
    b.add_wsdep("casein", casein());
    b.add_wsdep("hai", hai());
    b.add_wsdep("sires", sires());
    b.add_wsdep("traits", traits());
  };
  constexpr const auto add_mod = [](app &b) {
    auto m = b.add_unit<mod>("boa");
    m->add_part("ecs_objects");
    m->add_part("xorll");
    m->add_part("game");
    m->add_part("render");
    m->add_part("casein");
    return m;
  };

  const auto setup_vulkan = [&](app &b) {
    b.add_unit<spirv>("main.vert");
    b.add_unit<spirv>("main.frag");
    b.add_resource("main.vert.spv");
    b.add_resource("main.frag.spv");

    b.add_wsdep("vee", vee());
    add_deps(b);

    auto m = add_mod(b);
    m->add_part("vulkan");
    m->add_part("pipeline");
    m->add_part("vulkan_fsm");
  };
  const auto setup_wasm = [&](app &b) {
    add_deps(b);

    auto m = add_mod(b);
    m->add_part("wasm");
    m->add_feat<js>()->set("boa_fill_colour", R"((r, g, b) => {
  ctx.fillStyle = `rgb(${r}, ${g}, ${b})`;
})");
    m->add_feat<js>()->set("boa_fill_rect", "ctx.fillRect.bind(ctx)");
  };

  auto a = unit::create<per_feat<app>>("boas");
  setup_vulkan(a->for_feature(posix));
  setup_vulkan(a->for_feature(android_ndk));
  setup_wasm(a->for_feature(webassembly));
  return run_main(a, argc, argv);
}
