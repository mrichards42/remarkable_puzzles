#include "game_scene.hpp"

#include <tuple>

#include <rmkit.h>

#include "debug.hpp"
#include "game_list.hpp"
#include "timer.hpp"
#include "ui/msg.hpp"

void init_presets_menu(ui::TextDropdown * presets_menu, midend * me)
{
    // reset the dropdown
    presets_menu->scene = NULL;
    presets_menu->options.clear();
    presets_menu->sections.clear();
    // fill in preset names
    auto section = presets_menu->add_section("Presets");
    std::vector<std::string> names;
    auto menu = midend_get_presets(me, NULL);
    int preset_id = midend_which_preset(me);
    for (int i = 0; i < menu->n_entries; i++) {
        auto entry = menu->entries[i];
        names.push_back(entry.title);
        if (entry.id == preset_id)
            presets_menu->text = entry.title;
    }
    section->add_options(names);
}

void init_games_menu(ui::TextDropdown * games_menu)
{
    auto section = games_menu->add_section(games_menu->text);
    std::vector<std::string> names;
    for (const game * g : GAME_LIST)
        names.push_back(g->name);
    section->add_options(names);
}

GameScene::GameScene() : frontend()
{
    scene = ui::make_scene();

    // ----- Layout -----
    int w, h;
    std::tie(w, h) = framebuffer::get()->get_display_size();
    int tb_h = 100;

    auto v0 = ui::VerticalLayout(0, 0, w, h, scene);
    auto toolbar = ui::HorizontalLayout(0, 0, w, tb_h, scene);
    v0.pack_start(toolbar);

    // Status bar
    auto status_style = ui::Stylesheet().justify_left();
    status_text = new ui::Text(10, 0, w - 20, 50, "");
    status_text->set_style(status_style);
    v0.pack_end(status_text, 10);

    // Canvas
    canvas = new Canvas(100, 0, w - 200, h - 400);
    drawer = std::make_unique<PuzzleDrawer>(canvas);
    v0.pack_center(canvas);

    // Toolbar
    new_game_btn = new ui::Button(0, 0, 300, tb_h, "New Game");
    toolbar.pack_start(new_game_btn);

    restart_btn = new ui::Button(0, 0, 300, tb_h, "Restart");
    toolbar.pack_start(restart_btn);

    presets_menu = new ui::TextDropdown(0, 0, 300, tb_h, "Presets");
    presets_menu->dir = ui::DropdownButton::DOWN;
    toolbar.pack_start(presets_menu);

    games_menu = new ui::TextDropdown(0, 0, 300, tb_h, "Games");
    games_menu->dir = ui::DropdownButton::DOWN;
    init_games_menu(games_menu);
    toolbar.pack_start(games_menu);

    redo_btn = new ui::Button(0, 0, 100, tb_h, "=>");
    toolbar.pack_end(redo_btn);

    undo_btn = new ui::Button(0, 0, 100, tb_h, "<=");
    toolbar.pack_end(undo_btn);

    // ----- Events -----
    // Toolbar
    new_game_btn->mouse.click += [=](auto &ev) {
        start_game();
    };
    restart_btn->mouse.click += [=](auto &ev) {
        restart_game();
    };
    undo_btn->mouse.click += [=](auto &ev) {
        handle_puzzle_key(UI_UNDO);
    };
    redo_btn->mouse.click += [=](auto &ev) {
        handle_puzzle_key(UI_REDO);
    };
    games_menu->events.selected += [=](int idx) {
        set_game(GAME_LIST[idx]);
    };
    presets_menu->events.selected += [=](int idx) {
        set_params(midend_get_presets(me, NULL)->entries[idx].params);
    };

    // Canvas mouse events
    canvas_mouse = std::make_shared<MouseManager>(canvas);
    canvas_mouse->short_click += [=](auto &ev) {
        handle_canvas_event(ev, LEFT_BUTTON);
        handle_canvas_event(ev, LEFT_RELEASE);
    };
    canvas_mouse->long_click += [=](auto &ev) {
        handle_canvas_event(ev, RIGHT_BUTTON);
        handle_canvas_event(ev, RIGHT_RELEASE);
    };
    canvas_mouse->drag_start += [=](auto &ev) {
        handle_canvas_event(ev, LEFT_BUTTON);
    };
    canvas_mouse->dragging += [=](auto &ev) {
        handle_canvas_event(ev, LEFT_DRAG);
    };
    canvas_mouse->drag_end += [=](auto &ev) {
        handle_canvas_event(ev, LEFT_RELEASE);
    };
}

void GameScene::handle_canvas_event(input::SynMotionEvent & ev, int key_id)
{
    handle_puzzle_key(canvas->logical_x(ev.x), canvas->logical_y(ev.y), key_id);
}

void GameScene::handle_puzzle_key(int key_id)
{
    handle_puzzle_key(0, 0, key_id);
}

void GameScene::handle_puzzle_key(int x, int y, int key_id)
{
    midend_process_key(me, x, y, key_id);
    debugf("process key %4d, %4d, %d\n", x, y, key_id);
}

void GameScene::check_solved()
{
    if (timer_active)
        return;
    int status = midend_status(me);
    if (status == last_status)
        return;
    last_status = status;
    if (status == 0) // 0 = game still in progress
        return;
    // show the game over dialog
    bool win = status > 0;
    if (!game_over_dlg)
        game_over_dlg = std::make_unique<SimpleMessageDialog>(500, 200);
    game_over_dlg->show(win ? "You win!" : "Game over");
}

void GameScene::start_game()
{
    // Clear the screen
    canvas->drawfb()->clear_screen();
    auto fb = framebuffer::get();
    fb->clear_screen();
    fb->waveform_mode = WAVEFORM_MODE_INIT;
    fb->redraw_screen(true);
    fb->waveform_mode = WAVEFORM_MODE_DU;

    midend_new_game(me);
    status_bar("");
    last_status = 0;

    // resize and center the canvas
    int w = canvas->w;
    int h = canvas->h;
    midend_size(me, &w, &h, /* user_size = */ true);
    debugf("midend_size(%p, %d, %d, false) => (%d, %d)\n", (void*)me, canvas->w, canvas->h, w, h);
    canvas->translate((canvas->w - w) / 2, (canvas->h - h) / 2);

    midend_redraw(me);
    ui::MainLoop::refresh();
    ui::MainLoop::redraw();
}

void GameScene::restart_game()
{
    midend_restart_game(me);
    status_bar("");
    last_status = 0;
    ui::MainLoop::refresh();
    ui::MainLoop::redraw();
}

void GameScene::set_game(const game * a_game)
{
    init_midend(drawer.get(), a_game);
    init_presets_menu(presets_menu, me);
    start_game();
}

void GameScene::set_params(game_params * params)
{
    midend_set_params(me, params);
    start_game();
}

// Puzzle frontend
void GameScene::check_timer()
{
    if (timer_active) {
        double now = timer::now();
        if ((now - timer_prev) > timer::INTERVAL_S) {
            midend_timer(me, (float)(now - timer_prev));
            timer_prev = now;
        }
    }
}

void GameScene::frontend_default_colour(float *output)
{
    output[0] = output[1] = output[2] = 1.f;
}

void GameScene::activate_timer()
{
    timer_prev = timer::now();
    timer_active = true;
}

void GameScene::deactivate_timer()
{
    timer_active = false;
}

void GameScene::status_bar(const char *text)
{
    // TODO: this doesn't actually clear all the existing text?
    status_text->undraw();
    status_text->text = text;
    status_text->dirty = 1;
}
