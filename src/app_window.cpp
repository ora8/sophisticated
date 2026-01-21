#include "app_window.hpp"

#include <fstream>
#include <iostream>
#include <sstream>

AppWindow::AppWindow()
{
    auto theme = Gtk::IconTheme::get_for_display(Gdk::Display::get_default());
    if (theme)
        theme->add_search_path("assets");

    set_icon_name("com.example.sophisticatedgtk4");

    set_title("Editor App (GTK4)");
    set_default_size(1100, 700);

    set_child(m_root);

    build_header();
    build_layout();
    build_editor(); // ✅ IMPORTANT: actually create/pack editor widgets
    build_menu();
    install_actions();
    install_shortcuts();

    apply_theme();
}

AppWindow::~AppWindow()
{
    // ✅ avoid lifetime crashes if dialog touches the buffer on shutdown
    m_find_text.reset();
}

void AppWindow::build_header()
{
    m_header.set_show_title_buttons(true);

    auto file_lbl = Gtk::make_managed<Gtk::Label>("File");
    m_file_menu_btn.set_child(*file_lbl);

    auto help_lbl = Gtk::make_managed<Gtk::Label>("Help");
    m_help_menu_btn.set_child(*help_lbl);

    m_header.pack_start(m_file_menu_btn);
    m_header.pack_start(m_help_menu_btn);

    set_titlebar(m_header);
}

void AppWindow::build_layout()
{
    m_root.set_spacing(0);

    // Center editor column
    m_center.set_hexpand(true);
    m_center.set_vexpand(true);
    m_center.set_center_widget(m_editor_container);

    m_editor_container.set_spacing(8);
    m_editor_container.set_margin(12);
    m_editor_container.set_halign(Gtk::Align::FILL);
    m_editor_container.set_valign(Gtk::Align::FILL);
    m_editor_container.set_hexpand(true);
    m_editor_container.set_vexpand(true);
    m_editor_container.set_size_request(-1, -1);

    m_root.append(m_center);

    // Footer/status bar
    m_root.append(m_sep);

    m_footer.set_spacing(8);
    m_footer.set_margin_start(10);
    m_footer.set_margin_end(10);
    m_footer.set_margin_top(6);
    m_footer.set_margin_bottom(6);

    m_footer_left.set_halign(Gtk::Align::START);
    m_footer_right.set_halign(Gtk::Align::END);
    m_footer_left.set_hexpand(true);

    m_footer_left.set_text("Ready.");
    m_footer_right.set_text("gtkmm-4.0");

    m_footer.append(m_footer_left);
    m_footer.append(m_footer_right);
    m_root.append(m_footer);
}

void AppWindow::build_editor()
{
    // Scroller + TextView
    m_editor_scroller.set_policy(Gtk::PolicyType::AUTOMATIC, Gtk::PolicyType::AUTOMATIC);
    m_editor_scroller.set_hexpand(true);
    m_editor_scroller.set_vexpand(true);

    m_buffer = Gtk::TextBuffer::create();
    m_textview.set_buffer(m_buffer);
    m_textview.set_monospace(true);
    m_buffer = Gtk::TextBuffer::create();
    m_textview.set_buffer(m_buffer);

    // Track modifications
    m_buffer->signal_changed().connect([this]()
                                       {
    if (!m_modified) {
        m_modified = true;
        m_footer_left.set_text("Modified");
    } });

    // Highlight tag for search
    auto tagtable = m_buffer->get_tag_table();
    if (!tagtable->lookup("hl"))
    {
        auto tag = Gtk::TextBuffer::Tag::create("hl");
        tag->property_background() = "yellow";
        tag->property_foreground() = "black";
        tagtable->add(tag);
    }

    m_editor_scroller.set_child(m_textview);

    // ✅ Pack into center container
    m_editor_container.append(m_editor_scroller);
}

void AppWindow::build_menu()
{
    // ----- File menu -----
    m_file_menu = Gio::Menu::create();

    auto file_section = Gio::Menu::create();
    file_section->append("Open", "win.open");
    file_section->append("Save", "win.save");
    file_section->append("Find…", "win.find_text");
    file_section->append("Replace…", "win.replace_text");

    auto quit_section = Gio::Menu::create();
    quit_section->append("Quit", "win.quit");

    m_file_menu->append_section(file_section);
    m_file_menu->append_section(quit_section);
    m_file_menu_btn.set_menu_model(m_file_menu);

    // ----- Help menu -----
    m_help_menu = Gio::Menu::create();
    m_help_menu->append("Preferences", "win.preferences");
    m_help_menu->append("Toggle Theme", "win.toggle_theme");
    m_help_menu->append("About", "win.about");

    m_help_menu_btn.set_menu_model(m_help_menu);
}

void AppWindow::install_actions()
{
    m_actions = Gio::SimpleActionGroup::create();

    auto open = Gio::SimpleAction::create("open");
    open->signal_activate().connect([this](auto &)
                                    { on_open(); });
    m_actions->add_action(open);

    auto save = Gio::SimpleAction::create("save");
    save->signal_activate().connect([this](auto &)
                                    { on_save(); });
    m_actions->add_action(save);

    auto find_text = Gio::SimpleAction::create("find_text");
    find_text->signal_activate().connect([this](auto &)
                                         { on_find_text(); });
    m_actions->add_action(find_text);

    auto replace_text = Gio::SimpleAction::create("replace_text");
    replace_text->signal_activate().connect([this](auto &)
                                            { on_replace_text(); });
    m_actions->add_action(replace_text);

    auto prefs = Gio::SimpleAction::create("preferences");
    prefs->signal_activate().connect([this](auto &)
                                     { on_preferences(); });
    m_actions->add_action(prefs);

    auto theme = Gio::SimpleAction::create("toggle_theme");
    theme->signal_activate().connect([this](auto &)
                                     { on_toggle_theme(); });
    m_actions->add_action(theme);

    auto about = Gio::SimpleAction::create("about");
    about->signal_activate().connect([this](auto &)
                                     { on_about(); });
    m_actions->add_action(about);

    auto quit = Gio::SimpleAction::create("quit");
    quit->signal_activate().connect([this](auto &)
                                    { on_quit(); });
    m_actions->add_action(quit);

    // ✅ "win.*" namespace for menu models
    insert_action_group("win", m_actions);
}

void AppWindow::install_shortcuts()
{
    m_shortcuts = Gtk::ShortcutController::create();
    m_shortcuts->set_scope(Gtk::ShortcutScope::GLOBAL); // works anywhere in the window

    auto add = [&](guint keyval, Gdk::ModifierType mods, const char* action_name) {
        auto trigger = Gtk::KeyvalTrigger::create(keyval, mods);
        auto action  = Gtk::NamedAction::create(action_name);
        auto sc      = Gtk::Shortcut::create(trigger, action);
        m_shortcuts->add_shortcut(sc);
    };

    add(GDK_KEY_o, Gdk::ModifierType::CONTROL_MASK, "win.open");         // Ctrl+O
    add(GDK_KEY_s, Gdk::ModifierType::CONTROL_MASK, "win.save");         // Ctrl+S
    add(GDK_KEY_f, Gdk::ModifierType::CONTROL_MASK, "win.find_text");    // Ctrl+F
    add(GDK_KEY_h, Gdk::ModifierType::CONTROL_MASK, "win.replace_text"); // Ctrl+H (common “Replace”)
    add(GDK_KEY_q, Gdk::ModifierType::CONTROL_MASK, "win.quit");         // Ctrl+Q

    add_controller(m_shortcuts);
}

// -------- File helpers --------
void AppWindow::load_file(const std::string &path)
{
    if (!m_buffer)
    {
        std::cerr << "ERROR: buffer not initialized\n";
        return;
    }

    std::ifstream in(path);
    if (!in)
    {
        set_status("Failed to open: " + path);
        return;
    }

    std::stringstream ss;
    ss << in.rdbuf();

    m_buffer->set_text(ss.str()); // ✅ only once
    m_current_path = path;

    set_status("Opened: " + path);

    m_modified = false;
}

void AppWindow::save_file_to(const std::string &path)
{
    if (!m_buffer)
        return;

    std::ofstream out(path);
    if (!out)
    {
        set_status("Failed to save: " + path);
        return;
    }

    out << m_buffer->get_text();
    m_current_path = path;

    set_status("Saved: " + path);
}

// -------- Actions --------
void AppWindow::on_find_text()
{
    if (!m_find_text)
    {
        m_find_text = std::make_unique<FindTextDialog>(*this, m_textview);
    }
    m_find_text->present();
}

void AppWindow::on_open()
{
    auto dlg = Gtk::FileDialog::create();
    dlg->set_title("Open File");

    dlg->open(*this, [this, dlg](const Glib::RefPtr<Gio::AsyncResult> &res)
              {
        try {
            auto file = dlg->open_finish(res);
            if (!file) return;
            load_file(file->get_path());
        } catch (const Glib::Error &e) {
            set_status(Glib::ustring("Open canceled/failed: ") + e.what());
        } });
}

void AppWindow::on_replace_text()
{
    if (!m_replace_text)
    {
        m_replace_text = std::make_unique<ReplaceTextDialog>(*this, m_textview);
    }
    m_replace_text->present();
}

void AppWindow::on_save()
{
    if (!m_modified)
        return; // No changes to save

    if (!m_current_path.empty())
    {
        save_file_to(m_current_path);
        return;
    }

    auto dlg = Gtk::FileDialog::create();
    dlg->set_title("Save File");

    dlg->save(*this, [this, dlg](const Glib::RefPtr<Gio::AsyncResult> &res)
              {
        try {
            auto file = dlg->save_finish(res);
            if (!file) return;
            save_file_to(file->get_path());
        } catch (const Glib::Error &e) {
            set_status(Glib::ustring("Save canceled/failed: ") + e.what());
        } });
}

void AppWindow::on_search_changed()
{
}

void AppWindow::highlight_matches(const Glib::ustring &term)
{
    if (!m_buffer)
        return;

    auto start = m_buffer->begin();
    auto end = m_buffer->end();
    m_buffer->remove_tag_by_name("hl", start, end);

    if (term.empty())
        return;

    auto it = m_buffer->begin();
    while (true)
    {
        auto match_start = it;
        auto match_end = it;

        if (!match_start.forward_search(term,
                                        Gtk::TextSearchFlags::CASE_INSENSITIVE,
                                        match_start, match_end))
            break;

        m_buffer->apply_tag_by_name("hl", match_start, match_end);
        it = match_end;
    }
}

void AppWindow::on_preferences()
{
    auto prefs = Gtk::make_managed<Gtk::Window>();
    prefs->set_title("Preferences");
    prefs->set_transient_for(*this);
    prefs->set_modal(true);
    prefs->set_default_size(420, 160);

    auto box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL);
    box->set_margin(16);
    box->set_spacing(12);

    auto title = Gtk::make_managed<Gtk::Label>();
    title->set_markup("<b>Theme</b>");
    title->set_halign(Gtk::Align::START);

    auto dark = Gtk::make_managed<Gtk::CheckButton>("Enable darker look (simple CSS)");
    dark->set_active(m_dark);
    dark->signal_toggled().connect([this, dark]()
                                   {
        m_dark = dark->get_active();
        apply_theme(); });

    box->append(*title);
    box->append(*dark);

    prefs->set_child(*box);
    prefs->present();
}

void AppWindow::on_toggle_theme()
{
    m_dark = !m_dark;
    apply_theme();
}

void AppWindow::apply_theme()
{
    if (!m_css)
        m_css = Gtk::CssProvider::create();

    const char *css_light = "textview { font-size: 12pt; }";
    const char *css_dark =
        "* { background-color: #1e1e1e; color: #e6e6e6; }"
        "entry, textview { background-color: #2a2a2a; color: #e6e6e6; }"
        "textview text { background-color: #2a2a2a; color: #e6e6e6; }";

    try
    {
        m_css->load_from_data(m_dark ? css_dark : css_light);
    }
    catch (...)
    {
    }

    if (auto display = Gdk::Display::get_default())
    {
        Gtk::StyleContext::add_provider_for_display(
            display, m_css, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    }

    set_status(m_dark ? "Theme: dark" : "Theme: light");
}

void AppWindow::on_about()
{
    auto about = Gtk::make_managed<Gtk::AboutDialog>();
    about->set_transient_for(*this);
    about->set_modal(true);
    about->set_program_name("Text Editor");
    about->set_version("1.0");
    about->set_comments("gtkmm-4.0 editor demo with File/Help menus + Find dialog.");
    about->present();
}

void AppWindow::on_quit()
{
    if (!m_modified) {
        close();
        return;
    }

    query_unsaved_changes([this](bool should_close) {
        if (should_close)
            close();
    });
}

void AppWindow::query_unsaved_changes(std::function<void(bool should_close)> done)
{
    auto* dialog = new Gtk::MessageDialog(
        *this,
        "The document has unsaved changes.",
        false,
        Gtk::MessageType::QUESTION,
        Gtk::ButtonsType::NONE,
        true
    );

    dialog->set_secondary_text("Do you want to save your changes before exiting?");

    dialog->add_button("_Cancel", Gtk::ResponseType::CANCEL);
    dialog->add_button("_Discard", Gtk::ResponseType::REJECT);
    dialog->add_button("_Save", Gtk::ResponseType::ACCEPT);
    dialog->set_default_response(Gtk::ResponseType::ACCEPT);

    dialog->signal_response().connect(
        [this, dialog, done](int response) mutable {
            dialog->hide();
            delete dialog;

            switch (static_cast<Gtk::ResponseType>(response)) {
            case Gtk::ResponseType::ACCEPT: { // Save
                if (!m_current_path.empty()) {
                    save_file_to(m_current_path);
                    done(true);  // close after save
                } else {
                    auto dlg = Gtk::FileDialog::create();
                    dlg->set_title("Save File");

                    dlg->save(*this, [this, dlg, done](const Glib::RefPtr<Gio::AsyncResult>& res) mutable {
                        try {
                            auto file = dlg->save_finish(res);
                            if (!file) { done(false); return; } // user canceled Save As
                            save_file_to(file->get_path());
                            done(true);
                        } catch (const Glib::Error& e) {
                            set_status(Glib::ustring("Save failed: ") + e.what());
                            done(false);
                        }
                    });
                }
                break;
            }

            case Gtk::ResponseType::REJECT: // Discard
                done(true);
                break;

            default: // Cancel / closed
                done(false);
                break;
            }
        }
    );

    dialog->present();
}


void AppWindow::set_status(const Glib::ustring &s)
{
    // ✅ show status in footer (you removed dashboard status label)
    m_footer_left.set_text(s);
}
