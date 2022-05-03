#include "logo.h"

Logo::Logo(QObject *parent) : NT3Screen(parent)
{
    this->ba_ding.setSource(QUrl("qrc:/resources/sounds/effects/boot.wav"));
}

void Logo::init(){
    this->bd_played = false;
    this->ba_ding.setVolume(volume*volume_sfx_multiplier);
    this->logo_offset_y = -logo_rect_final_in.y() - logo_rect_final_in.height();
    this->logo_offset_delta = -logo_offset_y/logo_slide_duration; //UI pixels/sec
}

void Logo::calcScaleFactors(){
    this->logo_rect_final_px = SCALE_QRECTF(this->logo_rect_final_in, this->ui_to_screen_scale_px_in);
}    

void Logo::render(QPainter& painter){
    painter.setBrush(QColor(255, 255, 255));
    painter.drawRect(this->ui_field_px);
    
    double offset = this->logo_offset_y*this->ui_to_screen_scale_px_in;
    painter.drawPixmap(this->logo_rect_final_px.translated(0, offset).toRect(), this->logo);
}   

void Logo::keyPressEvent(QKeyEvent* ev){
    if (ev->key() == Qt::Key_Return){
        emit this->stateEnd(MAINMENU);
    }
}   

void Logo::doGameStep(){
    if (this->logo_offset_y < 0){
        this->logo_offset_y += framerate_s_f*this->logo_offset_delta; //seconds * pixels/sec = pixels
        if (this->logo_offset_y > 0){
            this->logo_offset_y = 0;
        }
    }
    
    this->time_passed += framerate_s_f;
    if (this->time_passed > this->logo_slide_duration + this->logo_delay){
        emit this->stateEnd(CREDITS);
    } else if (this->time_passed > this->logo_slide_duration && !this->bd_played){
        this->ba_ding.play();
        this->bd_played = true;
    }
}   
