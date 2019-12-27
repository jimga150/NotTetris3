#include "nt3window.h"

NT3Window::NT3Window()
{    
    connect(&this->game, &NT3Game::close, this, &QWindow::close);
    connect(&this->game, &NT3Game::setTitle, this, &QWindow::setTitle); //TODO: move this to nt3window only
    connect(&this->game, &NT3Game::setGeometry, this, QOverload<int, int, int, int>::of(&QWindow::setGeometry));
    connect(&this->game, &NT3Game::resize, this, QOverload<const QSize&>::of(&QWindow::resize));
    connect(&this->game, &NT3Game::setExpectedFrameTime, this, &NT3Window::setExpectedFrameTime);
    
    connect(&this->logo, &Logo::close, this, &QWindow::close);
    connect(&this->logo, &Logo::setTitle, this, &QWindow::setTitle); //TODO: move this to nt3window only
    connect(&this->logo, &Logo::setGeometry, this, QOverload<int, int, int, int>::of(&QWindow::setGeometry));
    connect(&this->logo, &Logo::resize, this, QOverload<const QSize&>::of(&QWindow::resize));
    connect(&this->logo, &Logo::setExpectedFrameTime, this, &NT3Window::setExpectedFrameTime);
    connect(&this->logo, &Logo::stateEnd, this, &NT3Window::stateEnd);
    
    this->logo.init(this->screen());
}

NT3Window::~NT3Window(){
    
}

void NT3Window::stateEnd(NT3_state_enum next){
    this->NT3state = next;
    switch(this->NT3state){
    case LOGO:
        this->logo.init(this->screen());
        break;
    case CREDITS:
        
        break;
    case MAINMENU:
        
        break;
    case GAMEA:
        this->game.startGame(this->screen());
        break;
    default:
        fprintf(stderr, "Invalid NT3 state: %u\n", this->NT3state);
        this->close();
        break;
    }
}

void NT3Window::render(QPainter &painter){
    switch(this->NT3state){
    case LOGO:
        this->logo.render(painter);
        break;
    case CREDITS:
        
        break;
    case MAINMENU:
        
        break;
    case GAMEA:
        this->game.render(painter);
        
    #ifdef TIME_FRAME_COMPS
        if (this->game.debug_framerate){
            painter.setPen(this->game.debug_line_color);
            painter.drawText(QPointF(3*this->game.ui_scale, 12*this->game.ui_scale), QString::number(this->frame_times.render_time));
            painter.drawText(QPointF(3*this->game.ui_scale, 14*this->game.ui_scale), QString::number(this->frame_times.game_frame_time));
        }
    #endif
        break;
    default:
        fprintf(stderr, "Invalid NT3 state: %u\n", this->NT3state);
        this->close();
        break;
    }
}

void NT3Window::doGameStep(){
    switch(this->NT3state){
    case LOGO:
        this->logo.doGameStep();
        break;
    case CREDITS:
        
        break;
    case MAINMENU:
        
        break;
    case GAMEA:
        this->game.doGameStep();
        break;
    default:
        fprintf(stderr, "Invalid NT3 state: %u\n", this->NT3state);
        this->close();
        break;
    }
}

void NT3Window::resizeEvent(QResizeEvent* event){
    switch(this->NT3state){
    case LOGO:
        this->logo.resizeEvent(event);
        break;
    case CREDITS:
        
        break;
    case MAINMENU:
        
        break;
    case GAMEA:
        this->game.resizeEvent(event);
        break;
    default:
        fprintf(stderr, "Invalid NT3 state: %u\n", this->NT3state);
        this->close();
        break;
    }
}

void NT3Window::keyPressEvent(QKeyEvent* ev){
    switch(this->NT3state){
    case LOGO:
        this->logo.keyPressEvent(ev);
        break;
    case CREDITS:
        
        break;
    case MAINMENU:
        
        break;
    case GAMEA:
        this->game.keyPressEvent(ev);
        break;
    default:
        fprintf(stderr, "Invalid NT3 state: %u\n", this->NT3state);
        this->close();
        break;
    }
}

void NT3Window::keyReleaseEvent(QKeyEvent* ev){
    switch(this->NT3state){
    case LOGO:
        this->logo.keyReleaseEvent(ev);
        break;
    case CREDITS:
        
        break;
    case MAINMENU:
        
        break;
    case GAMEA:
        this->game.keyReleaseEvent(ev);
        break;
    default:
        fprintf(stderr, "Invalid NT3 state: %u\n", this->NT3state);
        this->close();
        break;
    }
}

void NT3Window::setExpectedFrameTime(int eft){
    this->expected_frame_time = eft;
}