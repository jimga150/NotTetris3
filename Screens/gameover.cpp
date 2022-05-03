#include "gameover.h"
#include "nt3window.h"

GameOver::GameOver(QObject *parent) : NT3Screen(parent)
{
    this->gameover_sound.setSource(QUrl(this->gameover_sound_path));
}

void GameOver::init(){
    this->gameover_sound.setVolume(volume*volume_sfx_multiplier);
    this->gameover_sound.play();
    
    uint score_A = ((NT3Window*)(this->parent()))->gameA_score;
    uint score_B = ((NT3Window*)(this->parent()))->gameB_score;
    if (score_A > score_B){
        this->last_state = GAMEA;
    } else {
        this->last_state = GAMEB;
    }
}

void GameOver::colorizeResources(){
    this->game_a_overlay = this->colorize(this->game_a_overlay);
    this->game_b_overlay = this->colorize(this->game_b_overlay);
}

void GameOver::render(QPainter& painter){
    QPixmap last_frame = ((NT3Window*)(this->parent()))->game_lastframe;
    painter.drawPixmap(last_frame.rect(), last_frame);
    
    if (this->last_state == GAMEA){
        painter.drawPixmap(this->ui_field_px, this->game_a_overlay);
    } else {
        painter.drawPixmap(this->ui_field_px, this->game_b_overlay);
    }
    
}

void GameOver::keyPressEvent(QKeyEvent* ev){
    
    int key = ev->key();
        
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
