#include "nt3window.h"

NT3Window::NT3Window()
{
    this->setTitle("Not Tetris 3");
    
    for (uint s = 0; s < num_nt3_states; ++s){
        switch(s){
        case LOGO:
            this->screens[s] = new Logo(this);
            break;
        case CREDITS:
            this->screens[s] = new Credits(this);
            break;
        case MAINMENU:
            this->screens[s] = new NT3Screen(this); //TODO: make main menu screen
            break;
        case GAMEA:
            this->screens[s] = new NT3Game(this);
            break;
        default:
            fprintf(stderr, "Invalid NT3 state: %u\n", s);
            this->close();
            break;
        }
        connect(this->screens[s], &NT3Game::close, this, &QWindow::close);
        connect(this->screens[s], &NT3Game::setGeometry, this, QOverload<int, int, int, int>::of(&QWindow::setGeometry));
        connect(this->screens[s], &NT3Game::resize, this, QOverload<const QSize&>::of(&QWindow::resize));
        connect(this->screens[s], &NT3Game::stateEnd, this, &NT3Window::stateEnd);
    }
    
    this->screens[this->NT3state]->init(this->screen());
}

NT3Window::~NT3Window(){
    for (uint s = 0; s < num_nt3_states; ++s){
        delete this->screens[s];
    }
}

void NT3Window::stateEnd(NT3_state_enum next){
    Q_ASSERT(next < num_nt3_states);
    this->NT3state = next;
    this->screens[next]->init(this->screen());
}

void NT3Window::render(QPainter &painter){
    Q_ASSERT(this->NT3state < num_nt3_states);
    this->screens[this->NT3state]->render(painter);
#ifdef TIME_FRAME_COMPS
    if (this->NT3state == GAMEA){
        NT3Game* game = static_cast<NT3Game*>(this->screens[GAMEA]);
        if (game->debug_framerate){
            painter.setPen(game->debug_line_color);
            painter.drawText(QPointF(3*game->ui_scale, 12*game->ui_scale), QString::number(this->frame_times.render_time));
            painter.drawText(QPointF(3*game->ui_scale, 14*game->ui_scale), QString::number(this->frame_times.game_frame_time));
        }
    }
#endif
}

void NT3Window::doGameStep(){
    Q_ASSERT(this->NT3state < num_nt3_states);
    this->screens[this->NT3state]->doGameStep();
}

void NT3Window::resizeEvent(QResizeEvent* event){
    Q_ASSERT(this->NT3state < num_nt3_states);
    this->screens[this->NT3state]->resizeEvent(event);
}

void NT3Window::keyPressEvent(QKeyEvent* ev){
    Q_ASSERT(this->NT3state < num_nt3_states);
    this->screens[this->NT3state]->keyPressEvent(ev);
}

void NT3Window::keyReleaseEvent(QKeyEvent* ev){
    Q_ASSERT(this->NT3state < num_nt3_states);
    this->screens[this->NT3state]->keyReleaseEvent(ev);
}