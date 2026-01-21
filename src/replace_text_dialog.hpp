#pragma once
#include <gtkmm.h>
#include <string>

class ReplaceTextDialog {
public:
  explicit ReplaceTextDialog(Gtk::Window& parent, Gtk::TextView& textview);
  ~ReplaceTextDialog();

  void present();

private:
  Gtk::Window& m_parent;
  Gtk::TextView& m_textview;
  Glib::RefPtr<Gtk::TextBuffer> m_buffer;

  // UI
  Gtk::Window m_win;
  Gtk::Box m_root{Gtk::Orientation::VERTICAL};

  Gtk::Grid m_grid;

  Gtk::Label m_find_lbl{"Find:"};
  Gtk::Entry m_find;

  Gtk::Label m_replace_lbl{"Replace with:"};
  Gtk::Entry m_replace;

  Gtk::Box m_opts{Gtk::Orientation::HORIZONTAL};
  Gtk::CheckButton m_case{"Case sensitive"};
  Gtk::CheckButton m_wrap{"Wrap around"};
  Gtk::CheckButton m_highlight_all{"Highlight all"};

  Gtk::Box m_buttons{Gtk::Orientation::HORIZONTAL};
  Gtk::Button m_find_next{"Find Next"};
  Gtk::Button m_replace_next{"Replace Next"};
  Gtk::Button m_replace_all{"Replace All"};
  Gtk::Button m_close{"Close"};

  Gtk::Label m_status{"Type a term to find."};

  // Search state
  Gtk::TextBuffer::iterator m_last_start;
  Gtk::TextBuffer::iterator m_last_end;
  bool m_has_last = false;

private:
  void build_ui();
  void connect_signals();

  Gtk::TextSearchFlags search_flags() const;

  void on_term_changed();
  void on_options_changed();

  void on_find_next();
  void on_replace_next();
  void on_replace_all();

  void clear_highlights();
  void highlight_all(const Glib::ustring& term, Gtk::TextSearchFlags flags);

  bool find_from(Gtk::TextBuffer::iterator from,
                 const Glib::ustring& term,
                 Gtk::TextSearchFlags flags,
                 Gtk::TextBuffer::iterator& out_start,
                 Gtk::TextBuffer::iterator& out_end);

  void select_and_scroll(Gtk::TextBuffer::iterator s,
                         Gtk::TextBuffer::iterator e);

  void set_status(const Glib::ustring& s);
};
