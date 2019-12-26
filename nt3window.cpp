#include "nt3window.h"

NT3Window::NT3Window()
{    
    connect(&this->game, &NT3Game::close, this, &QWindow::close);
    connect(&this->game, &NT3Game::setTitle, this, &QWindow::setTitle);
    connect(&this->game, &NT3Game::setGeometry, this, QOverload<int, int, int, int>::of(&QWindow::setGeometry));
    connect(&this->game, &NT3Game::resize, this, QOverload<const QSize&>::of(&QWindow::resize));
    connect(&this->game, &NT3Game::setExpectedFrameTime, this, &NT3Window::setExpectedFrameTime);
    
    this->game.startGame(this->screen());
}

NT3Window::~NT3Window(){
    
}

void NT3Window::render(QPainter &painter){
    this->game.render(painter);
    
#ifdef TIME_FRAME_COMPS
    if (this->game.debug_framerate){
        painter.setPen(this->game.debug_line_color);
        painter.drawText(QPointF(3*this->game.ui_scale, 12*this->game.ui_scale), QString::number(this->frame_times.render_time));
        painter.drawText(QPointF(3*this->game.ui_scale, 14*this->game.ui_scale), QString::number(this->frame_times.game_frame_time));
    }
#endif
}

void NT3Window::doGameStep(){
    this->game.doGameStep();
}

void NT3Window::resizeEvent(QResizeEvent* event){
    this->game.resizeEvent(event);
}

void NT3Window::keyPressEvent(QKeyEvent* ev){
    this->game.keyPressEvent(ev);
}

void NT3Window::keyReleaseEvent(QKeyEvent* ev){
    this->game.keyReleaseEvent(ev);
}

void NT3Window::setExpectedFrameTime(int eft){
    this->expected_frame_time = eft;
}