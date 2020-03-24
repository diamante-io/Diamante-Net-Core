#include "medida/medida.h"

int main(int argc, char* argv[]) {
  medida::MetricsRegistry registry;
  auto& counter = registry.NewCounter({"foo", "bar", "baz"});
  counter.inc();

  return 0;
}