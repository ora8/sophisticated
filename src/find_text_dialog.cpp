#include "find_text_dialog.hpp"

FindTextDialog::FindTextDialog(Gtk::Window &parent, Gtk::TextView &textview)
    : m_parent(parent), m_textview(textview)
{
    m_buffer = m_textview.get_buffer();

    // Ensure highlight tag exists
    auto tagtable = m_buffer->get_tag_table();
    if (!tagtable->lookup("find_hl"))
    {
        auto tag = Gtk::TextBuffer::Tag::create("find_hl");
        tag->property_background() = "gold";
        tag->property_foreground() = "black";
        tagtable->add(tag);
    }

    build_ui();
    connect_signals();
}

FindTextDialog::~FindTextDialog()
{
    clear_highlights();
}

void FindTextDialog::present()
{
    m_win.present();
    m_query.grab_focus();
}

void FindTextDialog::build_ui()
{
    m_win.set_title("Find");
    m_win.set_transient_for(m_parent);
      m_win.set_destroy_with_parent(true);
  m_win.set_modal(true);        
    m_win.set_default_size(520, 140);

    m_root.set_margin(12);
    m_root.set_spacing(10);
    m_win.set_child(m_root);

    // Row 1: entry + buttons
    m_row1.set_spacing(8);
    m_query.set_placeholder_text("Find in document…");
    m_query.set_hexpand(true);

    m_row1.append(m_query);
    m_row1.append(m_prev);
    m_row1.append(m_next);

    // Row 2: options + close
    m_row2.set_spacing(10);

    m_wrap.set_active(true);
    m_highlight_all.set_active(true);

    m_row2.append(m_case);
    m_row2.append(m_wrap);
    m_row2.append(m_highlight_all);

    // spacer
    auto spacer = Gtk::make_managed<Gtk::Label>("");
    spacer->set_hexpand(true);
    m_row2.append(*spacer);

    m_row2.append(m_close);

    m_status.set_halign(Gtk::Align::START);

    m_root.append(m_row1);
    m_root.append(m_row2);
    m_root.append(m_status);

    m_win.signal_close_request().connect([this]() -> bool
                                         {
    m_win.hide();
    return false; }, false);
}

void FindTextDialog::connect_signals()
{
    m_next.signal_clicked().connect(sigc::mem_fun(*this, &FindTextDialog::on_next));
    m_prev.signal_clicked().connect(sigc::mem_fun(*this, &FindTextDialog::on_prev));
    m_close.signal_clicked().connect([this]()
                                     { m_win.hide(); });

    m_query.signal_changed().connect(sigc::mem_fun(*this, &FindTextDialog::on_term_changed));
    m_query.signal_activate().connect(sigc::mem_fun(*this, &FindTextDialog::on_next));

    m_case.signal_toggled().connect(sigc::mem_fun(*this, &FindTextDialog::on_options_changed));
    m_wrap.signal_toggled().connect(sigc::mem_fun(*this, &FindTextDialog::on_options_changed));
    m_highlight_all.signal_toggled().connect(sigc::mem_fun(*this, &FindTextDialog::on_options_changed));
}

Gtk::TextSearchFlags FindTextDialog::search_flags() const
{
    Gtk::TextSearchFlags flags{};
    if (!m_case.get_active())
        flags |= Gtk::TextSearchFlags::CASE_INSENSITIVE;
    return flags;
}

void FindTextDialog::set_status(const Glib::ustring &s)
{
    m_status.set_text(s);
}

void FindTextDialog::clear_highlights()
{
    auto start = m_buffer->begin();
    auto end = m_buffer->end();
    m_buffer->remove_tag_by_name("find_hl", start, end);
}

void FindTextDialog::highlight_all(const Glib::ustring &term, Gtk::TextSearchFlags flags)
{
    clear_highlights();
    if (term.empty())
        return;

    auto it = m_buffer->begin();
    Gtk::TextBuffer::iterator s, e;

    while (find_from(it, term, flags, s, e))
    {
        m_buffer->apply_tag_by_name("find_hl", s, e);
        it = e;
    }
}

bool FindTextDialog::find_from(Gtk::TextBuffer::iterator from,
                               const Glib::ustring &term,
                               Gtk::TextSearchFlags flags,
                               Gtk::TextBuffer::iterator &out_start,
                               Gtk::TextBuffer::iterator &out_end)
{
    out_start = from;
    out_end = from;
    return out_start.forward_search(term, flags, out_start, out_end);
}

// GTKmm doesn’t provide a neat “backward_search” helper on iterator in all versions,
// so we implement prev by scanning forward and remembering the last match before "from".
bool FindTextDialog::find_backward_from(Gtk::TextBuffer::iterator from,
                                        const Glib::ustring &term,
                                        Gtk::TextSearchFlags flags,
                                        Gtk::TextBuffer::iterator &out_start,
                                        Gtk::TextBuffer::iterator &out_end)
{
    auto it = m_buffer->begin();
    Gtk::TextBuffer::iterator s, e;

    bool found = false;
    Gtk::TextBuffer::iterator best_s = m_buffer->begin();
    Gtk::TextBuffer::iterator best_e = m_buffer->begin();

    while (find_from(it, term, flags, s, e))
    {
        // stop once match starts at/after "from"
        if (s >= from)
            break;
        found = true;
        best_s = s;
        best_e = e;
        it = e;
    }

    if (!found)
        return false;
    out_start = best_s;
    out_end = best_e;
    return true;
}

void FindTextDialog::select_and_scroll(Gtk::TextBuffer::iterator s,
                                       Gtk::TextBuffer::iterator e)
{
    m_buffer->select_range(s, e);
    m_textview.scroll_to(s, 0.15); // s is mutable local copy
    m_textview.grab_focus();

    m_last_start = s;
    m_last_end = e;
    m_has_last = true;
}

void FindTextDialog::on_term_changed()
{
    m_has_last = false;

    auto term = m_query.get_text();
    if (m_highlight_all.get_active())
    {
        highlight_all(term, search_flags());
    }
    else
    {
        clear_highlights();
    }

    set_status(term.empty() ? "Type a term and press Next." : "Ready.");
}

void FindTextDialog::on_options_changed()
{
    on_term_changed();
}

void FindTextDialog::on_next()
{
    const auto term = m_query.get_text();
    if (term.empty())
    {
        set_status("Enter a search term.");
        return;
    }

    auto flags = search_flags();
    Gtk::TextBuffer::iterator s, e;

    // Start search from end of last match, or current cursor position
    Gtk::TextBuffer::iterator start_from;
    if (m_has_last)
    {
        start_from = m_last_end;
    }
    else
    {
        start_from = m_buffer->get_insert()->get_iter();
    }

    if (find_from(start_from, term, flags, s, e))
    {
        select_and_scroll(s, e);
        set_status("Match found.");
        return;
    }

    if (m_wrap.get_active())
    {
        auto begin = m_buffer->begin();
        if (find_from(begin, term, flags, s, e))
        {
            select_and_scroll(s, e);
            set_status("Wrapped to start.");
            return;
        }
    }

    set_status("No more matches.");
}

void FindTextDialog::on_prev()
{
    const auto term = m_query.get_text();
    if (term.empty())
    {
        set_status("Enter a search term.");
        return;
    }

    auto flags = search_flags();
    Gtk::TextBuffer::iterator s, e;

    // Search backward from start of last match, or cursor
    Gtk::TextBuffer::iterator from;
    if (m_has_last)
    {
        from = m_last_start;
    }
    else
    {
        from = m_buffer->get_insert()->get_iter();
    }

    if (find_backward_from(from, term, flags, s, e))
    {
        select_and_scroll(s, e);
        set_status("Match found.");
        return;
    }

    if (m_wrap.get_active())
    {
        // wrap to end: find last match in buffer
        auto endpos = m_buffer->end();
        if (find_backward_from(endpos, term, flags, s, e))
        {
            select_and_scroll(s, e);
            set_status("Wrapped to end.");
            return;
        }
    }

    set_status("No earlier matches.");
}
