#include "nt3window.h"

double framerate; //going to assign this variable in this file

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
            this->screens[s] = new MainMenu(this);
            break;
        case P_GAMEOPTIONS:
            this->screens[s] = new NT3Screen(this);
            break;
        case PP_GAMEOPTIONS:
            this->screens[s] = new NT3Screen(this);
            break;
        case GLOBAL_OPTIONS:
            this->screens[s] = new NT3Screen(this);
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
        connect(this->screens[s], &NT3Game::resize, this, QOverload<const QSize&>::of(&QWindow::resize));
        connect(this->screens[s], &NT3Game::stateEnd, this, &NT3Window::stateEnd);
    }
    
    QScreen* screen = this->screen();
    
    framerate = 1.0/screen->refreshRate();
    
    QRect screenRect = screen->availableGeometry();
    int screen_width = screenRect.width();
    int screen_height = screenRect.height();
    
    if (screen_width*1.0/screen_height > this->screens[this->NT3state]->aspect_ratio){ //screen is relatively wider than the app
        
        int window_width = static_cast<int>(screen_height*this->screens[this->NT3state]->aspect_ratio);
        this->setGeometry((screen_width - window_width)/2, 0, window_width, screen_height);
        
    } else { //screen is relatively taller than app, or it's the same ratio
        
        int window_height = static_cast<int>(screen_width*1.0/this->screens[this->NT3state]->aspect_ratio);
        this->setGeometry(0, (screen_height - window_height)/2, screen_width, window_height);
    }
    
    this->screens[this->NT3state]->init();
}

NT3Window::~NT3Window(){
    for (uint s = 0; s < num_nt3_states; ++s){
        delete this->screens[s];
    }
}

void NT3Window::stateEnd(NT3_state_enum next){
    Q_ASSERT(next < num_nt3_states);
    this->NT3state = next;
    this->screens[next]->init();
    
    //forces the new screen object to consider the current window size
    QResizeEvent ev(this->size(), this->size()/2);
    this->resizeEvent(&ev);
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
    if (this->screens[this->NT3state]->lockAR(event->size())){
        this->screens[this->NT3state]->calcScaleFactors();
    }
}

void NT3Window::keyPressEvent(QKeyEvent* ev){
    Q_ASSERT(this->NT3state < num_nt3_states);
    this->screens[this->NT3state]->keyPressEvent(ev);
}

void NT3Window::keyReleaseEvent(QKeyEvent* ev){
    Q_ASSERT(this->NT3state < num_nt3_states);
    this->screens[this->NT3state]->keyReleaseEvent(ev);
}