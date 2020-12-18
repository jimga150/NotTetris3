#include "gameover.h"

GameOver::GameOver(QObject *parent) : NT3Screen(parent)
{
    
}

void GameOver::colorizeResources(){
    this->gameover_overlay = this->colorize(this->gameover_overlay);
}

void GameOver::render(QPainter& painter){
    QPixmap last_frame = ((NT3Window*)(this->parent()))->gameA_lastframe;
    painter.drawPixmap(last_frame.rect(), last_frame);
    painter.drawPixmap(this->scaled_ui_field, this->gameover_overlay);
}

void GameOver::keyPressEvent(QKeyEvent* ev){
    
    int key = ev->key();
    
    int score = ((NT3Window*)(this->parent()))->gameA_score;
    
    switch(key){
    case Qt::Key_Return:
    case Qt::Key_Y:
    case Qt::Key_Z:
    case Qt::Key_X:
    case Qt::Key_W:
    case Qt::Key_Up:
    case Qt::Key_Down:
    case Qt::Key_Left:
    case Qt::Key_Right:
        this->stateEnd(P_GAMEOPTIONS);
        break;
    default:
        
        break;
    }
    
}
