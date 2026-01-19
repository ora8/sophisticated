#pragma once
#include <gtkmm.h>
#include <string>

class FindTextDialog {
public:
  explicit FindTextDialog(Gtk::Window& parent, Gtk::TextView& textview);
  ~FindTextDialog();

  void present();

private:
  Gtk::Window& m_parent;
  Gtk::TextView& m_textview;
  Glib::RefPtr<Gtk::TextBuffer> m_buffer;

  // UI
  Gtk::Window m_win;
  Gtk::Box m_root{Gtk::Orientation::VERTICAL};
  Gtk::Box m_row1{Gtk::Orientation::HORIZONTAL};
  Gtk::Box m_row2{Gtk::Orientation::HORIZONTAL};

  Gtk::Entry m_query;
  Gtk::CheckButton m_case{"Case sensitive"};
  Gtk::CheckButton m_wrap{"Wrap around"};
  Gtk::CheckButton m_highlight_all{"Highlight all"};

  Gtk::Button m_prev{"Previous"};
  Gtk::Button m_next{"Next"};
  Gtk::Button m_close{"Close"};

  Gtk::Label m_status{"Type a term and press Next."};

  // Search state
  Gtk::TextBuffer::iterator m_last_start;
  Gtk::TextBuffer::iterator m_last_end;
  bool m_has_last = false;

private:
  void build_ui();
  void connect_signals();

  void on_next();
  void on_prev();
  void on_term_changed();
  void on_options_changed();

  void clear_highlights();
  void highlight_all(const Glib::ustring& term, Gtk::TextSearchFlags flags);

  Gtk::TextSearchFlags search_flags() const;

  bool find_from(Gtk::TextBuffer::iterator from,
                 const Glib::ustring& term,
                 Gtk::TextSearchFlags flags,
                 Gtk::TextBuffer::iterator& out_start,
                 Gtk::TextBuffer::iterator& out_end);

  bool find_backward_from(Gtk::TextBuffer::iterator from,
                          const Glib::ustring& term,
                          Gtk::TextSearchFlags flags,
                          Gtk::TextBuffer::iterator& out_start,
                          Gtk::TextBuffer::iterator& out_end);

void select_and_scroll(Gtk::TextBuffer::iterator s,
                       Gtk::TextBuffer::iterator e);


  void set_status(const Glib::ustring& s);
};
