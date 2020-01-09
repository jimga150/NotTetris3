#include "nt3window.h"

double framerate;

int volume;
double hue;
bool fullscreen;

QString* music_urls;

NT3Window::NT3Window() //TODO: sounds
{   
    this->setTitle("Not Tetris 3");
    
    volume = DEFAULT_VOLUME;
    this->oldVolume = DEFAULT_VOLUME;
    
    hue = DEFAULT_HUE;
    this->oldHue = hue;
    
    fullscreen = DEFAULT_FULLSCREEN;
    
    music_urls = new QString[num_music_types]{
        "qrc:/resources/sounds/music/themeA.mp3",
        "qrc:/resources/sounds/music/themeB.mp3",
        "qrc:/resources/sounds/music/themeC.mp3",
        ""
    };
    
    QScreen* screen = this->screen();
    framerate = 1.0/screen->refreshRate();
    
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
            this->screens[s] = new Menu1P(this);
            break;
        case PP_GAMEOPTIONS:
            this->screens[s] = new NT3Screen(this); //TODO: make 2p game options screen
            break;
        case GLOBAL_OPTIONS:
            this->screens[s] = new GlobalOptions(this);
            break;
        case GAMEA:
            this->screens[s] = new GameA(this);
            break;
        case GAMEB:
            this->screens[s] = new NT3Screen(this); //TODO: make Gamemode B (oh boy)
            break;
        case GAME_LOST:
            this->screens[s] = new NT3Screen(this); //TODO: make Game over screen
            break;
        default:
            fprintf(stderr, "Invalid NT3 state: %u\n", s);
            this->close();
            break;
        }
        connect(this->screens[s], &NT3Screen::close, this, &QWindow::close);
        connect(this->screens[s], &NT3Screen::resize, this, QOverload<const QSize&>::of(&QWindow::resize));
        connect(this->screens[s], &NT3Screen::stateEnd, this, &NT3Window::stateEnd);
        connect(this->screens[s], &NT3Screen::changeMusic, this, &NT3Window::musicChange);
    }
    
    this->music_player.setVolume(volume);
    
    connect(&this->music_player, &QMediaPlayer::stateChanged, this, &NT3Window::restartMusic);
    
    this->setupWindow();
    
    this->stateEnd(this->NT3state);
}

NT3Window::~NT3Window(){
    for (uint s = 0; s < num_nt3_states; ++s){
        delete this->screens[s];
    }
    if (music_urls){
        delete [] music_urls;
    }
}

void NT3Window::setupWindow(){
    QScreen* screen = this->screen();
    
    QRect screenRect = screen->availableGeometry();
    int screen_width = screenRect.width();
    int screen_height = screenRect.height();
    
    if (screen_width*1.0/screen_height > this->screens[this->NT3state]->aspect_ratio){ //screen is relatively wider than the app
        
        int window_width = static_cast<int>(screen_height*this->screens[this->NT3state]->aspect_ratio);
        this->fullscreen_offset = QPoint((screen_width - window_width)/2, 0);
        this->setGeometry(QRect(this->fullscreen_offset, QSize(window_width, screen_height)));
        
    } else { //screen is relatively taller than app, or it's the same ratio
        
        int window_height = static_cast<int>(screen_width*1.0/this->screens[this->NT3state]->aspect_ratio);
        this->fullscreen_offset = QPoint(0, (screen_height - window_height)/2);
        this->setGeometry(QRect(this->fullscreen_offset, QSize(screen_width, window_height)));
    }
}

void NT3Window::stateEnd(NT3_state_enum next, bool stopMusic){
    Q_ASSERT(next < num_nt3_states);
    
    this->NT3state = next;
    
    if (stopMusic) this->music_player.stop();
    
    this->screens[next]->init();
    
    //forces the new screen object to consider the current window size
    QResizeEvent ev(this->size(), this->size()/2);
    this->resizeEvent(&ev);
}

void NT3Window::restartMusic(QMediaPlayer::State newstate){
    if (newstate == QMediaPlayer::StoppedState){
        this->music_player.play();
    }
}

void NT3Window::musicChange(QUrl new_url){
    disconnect(&this->music_player, &QMediaPlayer::stateChanged, this, &NT3Window::restartMusic);
    
    this->music_player.stop();
    this->music_player.setMedia(new_url);
    this->music_player.play();
    
    connect(&this->music_player, &QMediaPlayer::stateChanged, this, &NT3Window::restartMusic);
}

void NT3Window::render(QPainter &painter){
    Q_ASSERT(this->NT3state < num_nt3_states);
    
    if (fullscreen){
        painter.translate(this->fullscreen_offset);
    }
    
    this->screens[this->NT3state]->render(painter);
#ifdef TIME_FRAME_COMPS
    if (this->NT3state == GAMEA){
        GameA* game = static_cast<GameA*>(this->screens[GAMEA]);
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
    
    bool isfullscreennow = this->windowStates().testFlag(Qt::WindowMaximized);
    if (fullscreen && !isfullscreennow){
        this->setWindowState(Qt::WindowMaximized);
    } else if (!fullscreen && isfullscreennow){
        this->setWindowState(Qt::WindowNoState);
        this->setupWindow();
    }
    
    if (this->oldHue != hue){
        for (uint s = 0; s < num_nt3_states; ++s){
            this->screens[s]->colorizeResources();
        }
        this->oldHue = hue;
    }
    
    if (this->oldVolume != volume){
        this->music_player.setVolume(volume);
        this->oldVolume = volume;
    }
    
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