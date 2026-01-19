#include <gtkmm.h>
#include "app_window.hpp"

int main(int argc, char* argv[]) {
  auto app = Gtk::Application::create("com.example.sophisticatedgtk4");
  return app->make_window_and_run<AppWindow>(argc, argv);
}
