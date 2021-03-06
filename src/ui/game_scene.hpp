#ifndef RMP_GAME_SCENE_HPP
#define RMP_GAME_SCENE_HPP

#include <chrono>
#include <memory>
#include <string>

#include <rmkit.h>

#include "puzzles.hpp"
#include "ui/button_mixin.hpp"
#include "ui/canvas.hpp"
#include "ui/fs_pixmap.hpp"
#include "ui/game_menu.hpp"
#include "ui/game_over.hpp"
#include "ui/help.hpp"
#include "ui/puzzle_drawer.hpp"
#include "ui/toast.hpp"

class GameScene : public frontend {
protected:
    typedef ButtonMixin<FSPixmap> IconButton;
    typedef ToggleButtonMixin<FSPixmap> ToggleIconButton;

    // Main UI
    ui::Scene scene;

    ui::Button * new_game_btn = nullptr;
    IconButton * back_btn = nullptr;
    ui::Text * game_title = nullptr;
    ToggleIconButton * controls_btn = nullptr;
    IconButton * undo_btn = nullptr;
    IconButton * redo_btn = nullptr;
    GameMenu::Button * menu_btn = nullptr;
    void build_toolbar();

    Toast * controls_toast = nullptr;
    void show_controls(int timeout = 2000);

    Canvas * canvas;
    ui::Text * status_text;

    // Menu scene
    std::unique_ptr<GameMenu> game_menu;
    GameMenu* build_menu(int x, int y, int w, int h);
    void show_menu();

    // Game over dialog
    std::unique_ptr<GameOverDialog> game_over_dlg;
    int last_status = 0;

    // Help dialog
    std::unique_ptr<HelpDialog> help_dlg;
    HelpDialog* build_help();

    // Puzzle frontend
    std::unique_ptr<PuzzleDrawer> drawer;
    std::chrono::high_resolution_clock::time_point timer_prev;
    ui::TimerPtr game_timer;

public:
    GameScene();

    ui::MOUSE_EVENT back_click;

    void show()
    {
        ui::MainLoop::set_scene(scene);
        ui::MainLoop::full_refresh();
    }
    bool is_shown() { return scene == ui::MainLoop::scene; }

    void set_game(const game * a_game);
    void set_params(game_params * params);
    void init_game();
    void new_game();
    void restart_game();
    bool load_state(const std::string & filename);
    bool save_state(const std::string & filename);
    bool load_state();
    bool save_state();
    game_state * get_game_state()
    {
        return me->statepos > 0 ? me->states[me->statepos-1].state : NULL;
    }

    void check_solved();
    bool wants_full_refresh();

    // Puzzle frontend implementation
    void frontend_default_colour(float *output);
    void activate_timer();
    void deactivate_timer();
    void status_bar(const char *text);

protected:
    // Puzzle event handlers
    void init_input_handlers();
    void handle_puzzle_key(int key_id);
    void handle_puzzle_key(int x, int y, int key_id);
    void handle_canvas_event(input::SynMotionEvent & evt, int key_id);
};


#endif // RMP_GAME_SCENE_HPP
