#include "../casein/build.hpp"
#include "../ecow/ecow.hpp"
#include "../hai/build.hpp"
#include "../quack/build.hpp"
#include "../traits/build.hpp"
#include "../vee/build.hpp"

int main(int argc, char **argv) {
  using namespace ecow;

  auto a = unit::create<app>("boas");

  auto m = a->add_unit<mod>("boa");
  m->add_wsdep("casein", casein());
  m->add_wsdep("quack", quack());
  m->add_part("ecs_objects");
  m->add_part("xorll");
  m->add_part("game");
  m->add_part("casein");

  return run_main(a, argc, argv);
}
