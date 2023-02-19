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
  b->add_unit<>("main");

  return run_main(b, argc, argv);
}
