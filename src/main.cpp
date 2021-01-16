#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_TRUETYPE_IMPLEMENTATION
#include <rmkit.h>

#include <cstdio>
#include <tuple>

#include "debug.hpp"
#include "puzzles.hpp"

#include "ui/canvas.hpp"
#include "ui/puzzle_drawer.hpp"

#include <sys/time.h>

const int TIMER_INTERVAL_MS = 100;
const double TIMER_INTERVAL_S = (double)TIMER_INTERVAL_MS / 1000;

double timer_now() {
    struct timeval now;
    gettimeofday(&now, NULL);
    return now.tv_sec + (now.tv_usec * 0.000001);
}



class Frontend : public frontend
{
public:
    ui::Text * status;
    double timer_prev = 0.0;
    bool timer_active = false;
    Frontend(ui::Text * status)
        : frontend(), status(status)
    {
    }
    ~Frontend() {}

    void trigger_timer()
    {
        double now = timer_now();
        if ((now - timer_prev) > TIMER_INTERVAL_S) {
            midend_timer(me, (float)(now - timer_prev));
            timer_prev = now;
        }
    }

    void frontend_default_colour(float *output)
    {
        output[0] = output[1] = output[2] = 1.f;
    }

    void activate_timer()
    {
        timer_prev = timer_now();
        timer_active = true;
    }

    void deactivate_timer()
    {
        timer_active = false;
    }

    void status_bar(const char *text)
    {
        // TODO: this doesn't actually clear all the existing text
        status->undraw();
        status->text = text;
        status->dirty = 1;
    }
};

// TODO: use the full game list
extern const game lightup;

class OutlineButton : public ui::Button {
public:
    using ui::Button::Button;
    void before_render()
    {
        // center vertically
        y_padding = (h - textWidget->font_size) / 2;
        ui::Button::before_render();
    }

    void render()
    {
        ui::Button::render();
        fb->draw_rect(x, y, w, h, BLACK, false);
    }
};

class App {
public:
    Canvas * canvas;
    PuzzleDrawer * drawer;
    ui::Text * status;
    Frontend * fe;
    int timer_start = 0;

    App()
    {
        auto fb = framebuffer::get();
        fb->clear_screen();
        fb->waveform_mode = WAVEFORM_MODE_INIT;
        fb->redraw_screen(true);
        int w, h;
        std::tie(w, h) = fb->get_display_size();

        auto scene = ui::make_scene();
        ui::MainLoop::set_scene(scene);

        // ----- Layout -----
        int tb_h = 100;
        ui::Text::DEFAULT_FS = 30;

        auto v0 = ui::VerticalLayout(0, 0, w, h, scene);
        auto toolbar = ui::HorizontalLayout(0, 0, w, tb_h, scene);
        v0.pack_start(toolbar);

        // Status bar
        status = new ui::Text(10, 0, fb->width - 20, 50, "");
        status->justify = ui::Text::LEFT;
        v0.pack_end(status, 10);

        // Canvas
        canvas = new Canvas(100, 0, w - 200, h - 400);
        drawer = new PuzzleDrawer(canvas);
        v0.pack_center(canvas);

        // Bottom toolbar
        auto new_game = new OutlineButton(0, 0, 300, tb_h, "New Game");
        toolbar.pack_start(new_game);

        auto restart = new OutlineButton(0, 0, 300, tb_h, "Restart");
        toolbar.pack_start(restart);

        auto redo = new OutlineButton(0, 0, 100, tb_h, "=>");
        toolbar.pack_end(redo);

        auto undo = new OutlineButton(0, 0, 100, tb_h, "<=");
        toolbar.pack_end(undo);

        // ----- Events -----
        new_game->mouse.click += [=](auto &ev) {
            start_game();
        };
        restart->mouse.click += [=](auto &ev) {
            restart_game();
        };
        undo->mouse.click += [=](auto &ev) {
            midend_process_key(fe->me, 0, 0, UI_UNDO);
        };
        redo->mouse.click += [=](auto &ev) {
            midend_process_key(fe->me, 0, 0, UI_REDO);
        };

        // Canvas handlers
        canvas->mouse.down += [=](auto &ev) {
            printf("DOWN(%d,%d) (%d,%d)\n", ev.x - canvas->x, ev.y - canvas->y, ev.x, ev.y);
            midend_process_key(fe->me, ev.x - canvas->x, ev.y - canvas->y, LEFT_BUTTON);
        };
        canvas->mouse.up += [=](auto &ev) {
            printf("UP(%d,%d) (%d,%d)\n", ev.x - canvas->x, ev.y - canvas->y, ev.x, ev.y);
            midend_process_key(fe->me, ev.x - canvas->x, ev.y - canvas->y, LEFT_RELEASE);
        };
        canvas->mouse.leave += [=](auto &ev) {
            printf("LEAVE(%d,%d) (%d,%d)\n", ev.x - canvas->x, ev.y - canvas->y, ev.x, ev.y);
            midend_process_key(fe->me, ev.x - canvas->x, ev.y - canvas->y, LEFT_RELEASE);
        };

        // Frontend
        fe = new Frontend(status);
    }

    void start_game()
    {
        midend_new_game(fe->me);
        fe->status_bar("");
        int x = canvas->w;
        int y = canvas->h;
        midend_size(fe->me, &x, &y, /* user_size = */ true);
        debugf("midend_size(%p, %d, %d, false) => (%d, %d)\n", (void*)fe->me, canvas->w, canvas->h, x, y);
        midend_redraw(fe->me);
        ui::MainLoop::refresh();
        ui::MainLoop::redraw();
    }

    void restart_game()
    {
        midend_restart_game(fe->me);
        fe->status_bar("");
        ui::MainLoop::refresh();
        ui::MainLoop::redraw();
    }

    void run()
    {
        fe->init_midend(drawer, &lightup);
        start_game();

        while (1) {
            // Check timer
            if (fe->timer_active) {
                fe->trigger_timer();
            }
            // Check win/lose
            int status = midend_status(fe->me);
            if (status > 0) {
                fe->status_bar("You win!");
            } else if (status < 0) {
                fe->status_bar("You lose");
            }
            // Process events and redraw
            ui::MainLoop::main();
            ui::MainLoop::redraw();
            ui::MainLoop::read_input(fe->timer_active ? TIMER_INTERVAL_MS : 0);
        }
    }
};

int main()
{
    App app;
    app.run();
    return 0;
}
