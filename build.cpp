#include "../casein/build.hpp"
#include "../ecow/ecow.hpp"
#include "../hai/build.hpp"
#include "../sires/build.hpp"
#include "../traits/build.hpp"
#include "../vee/build.hpp"

int main(int argc, char **argv) {
  using namespace ecow;

  constexpr const auto add_mod = [](app &b) {
    auto m = b.add_unit<mod>("boa");
    m->add_wsdep("casein", casein());
    m->add_wsdep("hai", hai());
    m->add_wsdep("sires", sires());
    m->add_wsdep("traits", traits());
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

    auto m = add_mod(b);
    m->add_part("vulkan");
    m->add_part("pipeline");
    m->add_part("vulkan_fsm");
  };
  const auto setup_wasm = [&](app &b) {
    auto m = add_mod(b);
    m->add_part("wasm");
    m->add_feat<inline_js>("boa_fill_colour", R"((r, g, b) => {
  var rr = Math.pow(r, 1.0 / 2.2) * 256.0;
  var gg = Math.pow(g, 1.0 / 2.2) * 256.0;
  var bb = Math.pow(b, 1.0 / 2.2) * 256.0;
  ctx.fillStyle = `rgb(${rr}, ${gg}, ${bb})`;
})");
    m->add_feat<inline_js>("boa_fill_rect", "ctx.fillRect.bind(ctx)");
  };

  auto a = unit::create<per_feat<app>>("boas");
  setup_vulkan(a->for_feature(posix));
  setup_vulkan(a->for_feature(android_ndk));
  setup_wasm(a->for_feature(webassembly));
  return run_main(a, argc, argv);
}
