#include "replace_text_dialog.hpp"

ReplaceTextDialog::ReplaceTextDialog(Gtk::Window &parent, Gtk::TextView &textview)
    : m_parent(parent), m_textview(textview)
{
    m_buffer = m_textview.get_buffer();

    // Ensure highlight tag exists
    auto table = m_buffer->get_tag_table();
    if (!table->lookup("find_hl"))
    {
        auto tag = Gtk::TextBuffer::Tag::create("find_hl");
        tag->property_background() = "gold";
        tag->property_foreground() = "black";
        table->add(tag);
    }

    build_ui();
    connect_signals();
}

ReplaceTextDialog::~ReplaceTextDialog()
{
    // optional: keep highlights, but usually nicer to clear
    clear_highlights();
}

void ReplaceTextDialog::present()
{
    m_win.present();
    m_find.grab_focus();
}

void ReplaceTextDialog::build_ui()
{
    m_win.set_title("Replace");
    m_win.set_transient_for(m_parent);
    m_win.set_destroy_with_parent(true);
    m_win.set_modal(true);
    m_win.set_default_size(560, 190);

    m_root.set_margin(12);
    m_root.set_spacing(10);
    m_win.set_child(m_root);

    // Grid: labels + entries
    m_grid.set_row_spacing(8);
    m_grid.set_column_spacing(10);

    m_find_lbl.set_halign(Gtk::Align::START);
    m_replace_lbl.set_halign(Gtk::Align::START);

    m_find.set_hexpand(true);
    m_replace.set_hexpand(true);

    m_grid.attach(m_find_lbl, 0, 0, 1, 1);
    m_grid.attach(m_find, 1, 0, 1, 1);
    m_grid.attach(m_replace_lbl, 0, 1, 1, 1);
    m_grid.attach(m_replace, 1, 1, 1, 1);

    // Options
    m_opts.set_spacing(10);
    m_wrap.set_active(true);
    m_highlight_all.set_active(true);

    m_opts.append(m_case);
    m_opts.append(m_wrap);
    m_opts.append(m_highlight_all);

    // Buttons
    m_buttons.set_spacing(8);

    m_buttons.append(m_find_next);
    m_buttons.append(m_replace_next);
    m_buttons.append(m_replace_all);

    // spacer
    auto spacer = Gtk::make_managed<Gtk::Label>("");
    spacer->set_hexpand(true);
    m_buttons.append(*spacer);

    m_buttons.append(m_close);

    m_status.set_halign(Gtk::Align::START);

    m_root.append(m_grid);
    m_root.append(m_opts);
    m_root.append(m_buttons);
    m_root.append(m_status);

    m_win.signal_close_request().connect([this]() -> bool
                                         {
    m_win.hide();
    return false; }, false);
}

void ReplaceTextDialog::connect_signals()
{
    m_find.signal_changed().connect(sigc::mem_fun(*this, &ReplaceTextDialog::on_term_changed));
    m_find.signal_activate().connect(sigc::mem_fun(*this, &ReplaceTextDialog::on_find_next));
    m_replace.signal_activate().connect(sigc::mem_fun(*this, &ReplaceTextDialog::on_replace_next));

    m_case.signal_toggled().connect(sigc::mem_fun(*this, &ReplaceTextDialog::on_options_changed));
    m_wrap.signal_toggled().connect(sigc::mem_fun(*this, &ReplaceTextDialog::on_options_changed));
    m_highlight_all.signal_toggled().connect(sigc::mem_fun(*this, &ReplaceTextDialog::on_options_changed));

    m_find_next.signal_clicked().connect(sigc::mem_fun(*this, &ReplaceTextDialog::on_find_next));
    m_replace_next.signal_clicked().connect(sigc::mem_fun(*this, &ReplaceTextDialog::on_replace_next));
    m_replace_all.signal_clicked().connect(sigc::mem_fun(*this, &ReplaceTextDialog::on_replace_all));

    m_close.signal_clicked().connect([this]()
                                     { m_win.hide(); });
}

Gtk::TextSearchFlags ReplaceTextDialog::search_flags() const
{
    Gtk::TextSearchFlags flags{};
    if (!m_case.get_active())
        flags |= Gtk::TextSearchFlags::CASE_INSENSITIVE;
    return flags;
}

void ReplaceTextDialog::set_status(const Glib::ustring &s)
{
    m_status.set_text(s);
}

void ReplaceTextDialog::clear_highlights()
{
    auto b = m_buffer->begin();
    auto e = m_buffer->end();
    m_buffer->remove_tag_by_name("find_hl", b, e);
}

bool ReplaceTextDialog::find_from(Gtk::TextBuffer::iterator from,
                                  const Glib::ustring &term,
                                  Gtk::TextSearchFlags flags,
                                  Gtk::TextBuffer::iterator &out_start,
                                  Gtk::TextBuffer::iterator &out_end)
{
    out_start = from;
    out_end = from;
    return out_start.forward_search(term, flags, out_start, out_end);
}

void ReplaceTextDialog::highlight_all(const Glib::ustring &term, Gtk::TextSearchFlags flags)
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

void ReplaceTextDialog::select_and_scroll(Gtk::TextBuffer::iterator s,
                                          Gtk::TextBuffer::iterator e)
{
    m_buffer->select_range(s, e);
    m_textview.scroll_to(s, 0.15);
    m_textview.grab_focus();

    m_last_start = s;
    m_last_end = e;
    m_has_last = true;
}

void ReplaceTextDialog::on_term_changed()
{
    m_has_last = false;

    auto term = m_find.get_text();
    if (m_highlight_all.get_active())
        highlight_all(term, search_flags());
    else
        clear_highlights();

    set_status(term.empty() ? "Type a term to find." : "Ready.");
}

void ReplaceTextDialog::on_options_changed()
{
    on_term_changed();
}

void ReplaceTextDialog::on_find_next()
{
    const auto term = m_find.get_text();
    if (term.empty())
    {
        set_status("Enter a search term.");
        return;
    }

    auto flags = search_flags();

    Gtk::TextBuffer::iterator start_from;
    if (m_has_last)
        start_from = m_last_end;
    else
        start_from = m_buffer->get_insert()->get_iter();

    Gtk::TextBuffer::iterator s, e;
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

void ReplaceTextDialog::on_replace_next()
{
    const auto term = m_find.get_text();
    if (term.empty())
    {
        set_status("Enter a search term.");
        return;
    }

    const auto repl = m_replace.get_text();

    auto flags = search_flags();

    // If we currently have a selected match equal to last, replace it; otherwise find next first.
    Gtk::TextBuffer::iterator s, e;

    bool have_match = false;
    if (m_has_last)
    {
        // Ensure last match is still valid: simplest is to search from last_start again
        auto check_from = m_last_start;
        if (find_from(check_from, term, flags, s, e) && s == m_last_start)
        {
            have_match = true;
        }
    }

    if (!have_match)
    {
        on_find_next();
        if (!m_has_last)
            return;
        s = m_last_start;
        e = m_last_end;
    }
    else
    {
        // use s/e from check
        m_last_end = e;
    }

    // Replace selection [s, e] safely
    m_buffer->begin_user_action();

    // Create a mark at the start of the match (left gravity keeps it at the start)
    auto mark = m_buffer->create_mark(s, /*left_gravity=*/true);

    // Erase the match (INVALIDATES iterators!)
    m_buffer->erase(s, e);

    // Re-acquire a valid iterator from the mark and insert replacement
    auto pos = m_buffer->get_iter_at_mark(mark);
    m_buffer->delete_mark(mark);

    m_buffer->insert(pos, repl);

    m_buffer->end_user_action();

    m_has_last = false; // reset, then find next occurrence after inserted text
    if (m_highlight_all.get_active())
        highlight_all(term, flags);

    set_status("Replaced. Finding next…");
    on_find_next();
}

void ReplaceTextDialog::on_replace_all()
{
    const auto term = m_find.get_text();
    if (term.empty())
    {
        set_status("Enter a search term.");
        return;
    }

    const auto repl = m_replace.get_text();
    // Optional: prevent accidental delete-all
    // if (repl.empty()) { set_status("Replacement is empty (would delete matches)."); return; }

    auto flags = search_flags();

    // Clear highlights first (optional)
    if (m_highlight_all.get_active())
    {
        // we'll re-highlight at the end
    }
    else
    {
        clear_highlights();
    }

    int count = 0;

    // Use a MARK as the moving cursor so we never keep iterators across edits
    auto cursor = m_buffer->create_mark(m_buffer->begin(), true);

    m_buffer->begin_user_action();

    while (true)
    {
        // Recreate iterator from mark each loop (always valid)
        auto it = m_buffer->get_iter_at_mark(cursor);

        Gtk::TextBuffer::iterator s, e;
        s = it;
        e = it;

        if (!s.forward_search(term, flags, s, e))
            break;

        // Mark where to continue AFTER replacement (at end of match, before we mutate)
        auto after = m_buffer->create_mark(e, true);

        // Replace using a mark at match start (safe across erase)
        auto start_mark = m_buffer->create_mark(s, true);

        // Mutations invalidate iterators -> do not use s/e after this
        m_buffer->erase(s, e);

        auto insert_pos = m_buffer->get_iter_at_mark(start_mark);
        m_buffer->delete_mark(start_mark);
        m_buffer->insert(insert_pos, repl);

        ++count;

        // Continue from the "after" mark (which tracks position through edits)
        m_buffer->delete_mark(cursor);
        cursor = after;
    }

    m_buffer->end_user_action();

    // cleanup
    m_buffer->delete_mark(cursor);

    m_has_last = false;

    // re-highlight if enabled
    if (m_highlight_all.get_active())
    {
        highlight_all(term, flags);
    }

    set_status(Glib::ustring("Replaced ") + std::to_string(count) + " occurrence(s).");
}
