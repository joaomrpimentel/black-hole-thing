#include "Application.h"

int main() {
  Application app;

  if (!app.init(1280, 720, "Black Hole Visualizer")) {
    return -1;
  }

  app.run();

  return 0;
}
