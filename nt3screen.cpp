#include "nt3screen.h"

NT3Screen::NT3Screen(QObject *parent) : QObject(parent)
{
    //one-time initialization per screen
    //set music file location here
}

NT3Screen::~NT3Screen(){
    //one-time destruction at end of application's life
}

void NT3Screen::init(){
    //init is called every time this screen becomes active, so this should also reset any dirty variables
    this->resetBlinkTimer();
}

void NT3Screen::calcScaleFactors(){
    //set any variables needed
    //Dont override this if you don't need to do anything not already done in lockAR()
}

void NT3Screen::render(QPainter& painter){
    Q_UNUSED(painter)
}

void NT3Screen::keyPressEvent(QKeyEvent* ev){
    Q_UNUSED(ev)
}

void NT3Screen::keyReleaseEvent(QKeyEvent* ev){
    Q_UNUSED(ev)
}

void NT3Screen::doGameStep(){
    this->blink_timer += framerate_s_f;
    if (this->blink_timer > this->select_blink_rate){
        this->blink_on = !this->blink_on;
        this->blink_timer = 0;
    }
}

void NT3Screen::resetBlinkTimer(){
    this->blink_timer = 0;
    this->blink_on = true;
}

void NT3Screen::colorizeResources(){
    //call this->colorize() on every image you have
}

bool NT3Screen::lockAR(QSize newSize){
    int width = newSize.width();
    int height = newSize.height();
    
    double ar_error = width*1.0/height - aspect_ratio;
    bool aspect_ratio_respected = qAbs(ar_error) < this->aspect_ratio_epsilon;
    
    if (ar_error > 0){ //screen is relatively wider than the app
        this->ui_to_screen_scale_px_in = height*1.0/this->ui_field_in.height();
    } else { //screen is relatively skinnier than app
        this->ui_to_screen_scale_px_in = width*1.0/this->ui_field_in.width();
    }
    
    this->ui_to_screen_scale_px_in = qMax(this->min_graphics_scale_px_in, this->ui_to_screen_scale_px_in);
    this->ui_field_px = TO_QRECT(this->ui_field_in, this->ui_to_screen_scale_px_in);
    
    if (!aspect_ratio_respected && !fullscreen){
        emit this->resize(this->ui_field_px.size());
        return false;
    }
    return true;
}

QPixmap NT3Screen::colorize(QPixmap pixmap){
    QImage image = pixmap.toImage();
    
    //these are between 0 and 1
    double rr = qMax(qMin(6*abs(hue-0.5)-1,1.0),0.0);
    double rg = qMax(qMin(-6*abs(hue-(2.0/6.0))+2,1.0),0.0);
    double rb = qMax(qMin(-6*abs(hue-(4.0/6.0))+2,1.0),0.0);
    
    bool hasAlpha = image.hasAlphaChannel();
    
    QRgb* pixels = reinterpret_cast<QRgb*>(image.bits());
    int pixelCount = image.width()*image.height();
    
    for (int p = 0; p < pixelCount; p++) {
        QColor pixel = pixels[p];
        if (hasAlpha && pixel.alpha() == 0) continue;
        
        int gray = qGray(pixel.rgb());
        if (gray > 145 && gray < 213){
            pixel.setRed(static_cast<int>(145 + 64*rr));
            pixel.setGreen(static_cast<int>(145 + 64*rg));
            pixel.setBlue(static_cast<int>(145 + 64*rb));
        } else if (gray > 73 && gray < 117){
            pixel.setRed(static_cast<int>(73 + 43*rr));
            pixel.setGreen(static_cast<int>(73 + 43*rg));
            pixel.setBlue(static_cast<int>(73 + 43*rb));
        } else {
            continue;
        }
        pixels[p] = pixel.rgb();
    }
    
    return QPixmap::fromImage(image);
}
