#include "../casein/build.hpp"
#include "../ecow/ecow.hpp"
#include "../hai/build.hpp"
#include "../sires/build.hpp"
#include "../traits/build.hpp"
#include "../vee/build.hpp"

int main(int argc, char **argv) {
  using namespace ecow;

  auto b = unit::create<app>("boas");
  b->add_wsdep("casein", casein());
  b->add_wsdep("hai", hai());
  b->add_wsdep("sires", sires());
  b->add_wsdep("traits", traits());
  b->add_wsdep("vee", vee());

  b->add_unit<spirv>("main.vert");
  b->add_unit<spirv>("main.frag");
  b->add_resource("main.vert.spv");
  b->add_resource("main.frag.spv");

  auto m = b->add_unit<mod>("boa");
  m->add_part("ecs_objects");
  m->add_part("vulkan");

  b->add_unit<>("main");

  auto pf = unit::create<per_feat<seq>>("pf");
  pf->for_feature(posix).add_ref(b);
  pf->for_feature(android_ndk).add_ref(b);

  return run_main(pf, argc, argv);
}
