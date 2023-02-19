#include "../casein/build.hpp"
#include "../ecow/ecow.hpp"
#include "../vee/build.hpp"

int main(int argc, char **argv) {
  using namespace ecow;

  auto b = unit::create<app>("boas");

  return run_main(b, argc, argv);
}
