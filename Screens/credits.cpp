#include "credits.h"

Credits::Credits(QObject *parent) : NT3Screen(parent)
{
    
}

void Credits::init(){
    this->time_passed = 0;
}

void Credits::render(QPainter& painter){
    painter.setBrush(QColor(255, 255, 255));
    painter.drawRect(this->ui_field_px);
    for (int r = 0; r < this->credits_numlines; r++){
        this->BOW_font.print(&painter, QPoint(0, (r+1)*this->BOW_font.height_px)*this->ui_to_screen_scale_px_in, LEFT_ALIGN, 
                             this->credits_text[r], this->ui_to_screen_scale_px_in);
    }
    painter.drawPixmap(QRect(this->logo_pos*this->ui_to_screen_scale_px_in, this->logo.size()*this->ui_to_screen_scale_px_in), this->logo);
}   

void Credits::keyPressEvent(QKeyEvent* ev){
    if (ev->key() == Qt::Key_Return){
        emit this->stateEnd(MAINMENU);
    }
}  

void Credits::doGameStep(){
    this->time_passed += framerate_s_f;
    if (this->time_passed > this->credits_delay){
        emit this->stateEnd(MAINMENU);
    }
}   
