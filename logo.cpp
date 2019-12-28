#include "logo.h"

Logo::Logo(QObject *parent) : NT3Screen(parent)
{
    
}

void Logo::init(QScreen* screen){
    this->framerate = 1.0/screen->refreshRate();
    
    QRect screenRect = screen->availableGeometry();
    int screen_width = screenRect.width();
    int screen_height = screenRect.height();
    
    if (screen_width*1.0/screen_height > this->aspect_ratio){ //screen is relatively wider than the app
        
        int window_width = static_cast<int>(screen_height*this->aspect_ratio);
        emit this->setGeometry((screen_width - window_width)/2, 0, window_width, screen_height);
        
    } else { //screen is relatively taller than app, or it's the same ratio
        
        int window_height = static_cast<int>(screen_width*1.0/this->aspect_ratio);
        emit this->setGeometry(0, (screen_height - window_height)/2, screen_width, window_height);
    }
    
    this->scaled_logo_rect_final = TO_QRECT(logo_rect_final, 1);
    this->logo_offset_y = -logo_rect_final.y() - logo_rect_final.height();
    this->logo_offset_delta = -logo_offset_y/logo_slide_duration; //UI pixels/sec
}

void Logo::resizeEvent(QResizeEvent* event){
    QSize newSize = event->size();
    int width = newSize.width();
    int height = newSize.height();
    
    double ar_error = width*1.0/height - aspect_ratio;
    bool aspect_ratio_respected = qAbs(ar_error) < this->aspect_ratio_epsilon;
    
    if (ar_error > 0){ //screen is relatively wider than the app
        this->ui_scale = height*1.0/this->ui_field.height();
    } else if (ar_error < 0){ //screen is relatively skinnier than app
        this->ui_scale = width*1.0/this->ui_field.width();
    }
    
    this->ui_scale = qMax(this->min_graphics_scale, this->ui_scale);
    
    this->scaled_ui_field = TO_QRECT(this->ui_field, this->ui_scale);
    this->scaled_logo_rect_final = TO_QRECT(this->logo_rect_final, this->ui_scale);
    
    if (!aspect_ratio_respected){
        emit this->resize(this->scaled_ui_field.size());
    }
}    

void Logo::render(QPainter& painter){
    painter.setBrush(QColor(255, 255, 255));
    painter.drawRect(this->scaled_ui_field);
    
    int offset = static_cast<int>(this->logo_offset_y*this->ui_scale);
    painter.drawPixmap(this->scaled_logo_rect_final.translated(0, offset), this->logo);
}   

void Logo::keyPressEvent(QKeyEvent* ev){
    if (ev->key() == Qt::Key_Return || ev->key() == Qt::Key_Enter){
        emit this->stateEnd(MAINMENU);
    }
}   

void Logo::keyReleaseEvent(QKeyEvent* ev){
    Q_UNUSED(ev)
    //do nothing
}   

void Logo::doGameStep(){
    if (this->logo_offset_y < 0){
        this->logo_offset_y += this->framerate*this->logo_offset_delta; //seconds * pixels/sec = pixels
        if (this->logo_offset_y > 0){
            this->logo_offset_y = 0;
        }
    }
    
    this->time_passed += this->framerate;
    if (this->time_passed > this->logo_slide_duration + this->logo_delay){
        emit this->stateEnd(MAINMENU);
    }
}   