#include "logo.h"

Logo::Logo(QObject *parent) : NT3Screen(parent)
{
    
}

void Logo::init(){
    this->logo_offset_y = -logo_rect_final.y() - logo_rect_final.height();
    this->logo_offset_delta = -logo_offset_y/logo_slide_duration; //UI pixels/sec
}

void Logo::calcScaleFactors(){
    this->scaled_logo_rect_final = TO_QRECT(this->logo_rect_final, this->ui_scale);
}    

void Logo::render(QPainter& painter){
    painter.setBrush(QColor(255, 255, 255));
    painter.drawRect(this->scaled_ui_field);
    
    int offset = static_cast<int>(this->logo_offset_y*this->ui_scale);
    painter.drawPixmap(this->scaled_logo_rect_final.translated(0, offset), this->logo);
}   

void Logo::keyPressEvent(QKeyEvent* ev){
    if (ev->key() == Qt::Key_Return){
        emit this->stateEnd(MAINMENU);
    }
}   

void Logo::doGameStep(){
    if (this->logo_offset_y < 0){
        this->logo_offset_y += framerate*this->logo_offset_delta; //seconds * pixels/sec = pixels
        if (this->logo_offset_y > 0){
            this->logo_offset_y = 0;
        }
    }
    
    this->time_passed += framerate;
    if (this->time_passed > this->logo_slide_duration + this->logo_delay){
        emit this->stateEnd(CREDITS);
    }
}   