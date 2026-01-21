#pragma once

#include "find_text_dialog.hpp"
#include "replace_text_dialog.hpp"

#include <gtkmm.h>
#include <atomic>
#include <functional>
#include <memory>
#include <thread>

class AppWindow : public Gtk::ApplicationWindow
{
public:
  AppWindow();
  ~AppWindow() override;

private:
  // Root + header
  Gtk::Box m_root{Gtk::Orientation::VERTICAL};
  Gtk::HeaderBar m_header;

  Gtk::Button m_btn_open{"Open"};
  Gtk::Button m_btn_save{"Save"};
  Gtk::Button m_btn_run{"Run Task"};
  Gtk::MenuButton m_file_menu_btn;
  Gtk::MenuButton m_help_menu_btn;

  Glib::RefPtr<Gio::Menu> m_file_menu;
  Glib::RefPtr<Gio::Menu> m_help_menu;

  // Actions
  Glib::RefPtr<Gio::SimpleActionGroup> m_actions;

  Glib::RefPtr<Gtk::ShortcutController> m_shortcuts;

  // Editor area
  Gtk::ScrolledWindow m_editor_scroller;
  Gtk::TextView m_textview;
  Glib::RefPtr<Gtk::TextBuffer> m_buffer;
  bool m_modified = false;

  Gtk::CenterBox m_center;
  Gtk::Box m_editor_container{Gtk::Orientation::VERTICAL};

  Gtk::Label m_status;

  // Footer
  Gtk::Separator m_sep{Gtk::Orientation::HORIZONTAL};
  Gtk::Box m_footer{Gtk::Orientation::HORIZONTAL};
  Gtk::Label m_footer_left;
  Gtk::Label m_footer_right;

  // State
  std::string m_current_path;
  Glib::RefPtr<Gtk::CssProvider> m_css;
  bool m_dark = false;

  std::unique_ptr<FindTextDialog> m_find_text;
  std::unique_ptr<ReplaceTextDialog> m_replace_text;

private:
  void build_header();
  void build_layout();
  void build_editor();
  void build_menu();
  void install_actions();
  void install_shortcuts();

  // Actions
  void on_find_text();
  void on_open();
  void on_replace_text();
  void on_save();
  void on_run_task();
  void on_quit();
  void on_about();
  void on_preferences();
  void on_search_changed();
  void on_toggle_theme();

  // Editor
  void highlight_matches(const Glib::ustring &term);

  // Task updates
  bool on_tick();
  void stop_task_thread();

  // Helpers
  void set_status(const Glib::ustring &s);
  void apply_theme();

  // File helpers
  void load_file(const std::string &path);
  void save_file_to(const std::string &path);

  //
  void query_unsaved_changes(std::function<void(bool should_close)> done);
};
