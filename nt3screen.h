#ifndef NT3SCREEN_H
#define NT3SCREEN_H

#include <QResizeEvent>
#include <QPainter>
#include <QScreen>

#include "common.h"
#include "imagefont.h"

class NT3Screen : public QObject
{
    Q_OBJECT
public:
    explicit NT3Screen(QObject *parent = nullptr);
    virtual ~NT3Screen();
    
    virtual void init(QScreen* screen);
    
    
    virtual void resizeEvent(QResizeEvent* event);    
    
    virtual void render(QPainter& painter);
    
    
    virtual void keyPressEvent(QKeyEvent* ev);
    
    virtual void keyReleaseEvent(QKeyEvent* ev);
    
    virtual void doGameStep();
    
    bool lockAR(QSize newSize);
    
    
    ImageFont BOW_font = ImageFont(
                             "0123456789ABCDEFGHIJKLMNOPQRStTUVWXYZ.,'c-#_>:<! ", 
                             QImage(":/resources/graphics/font.png")
                             );
    ImageFont WOB_font = ImageFont(
                             "0123456789ABCDEFGHIJKLMNOPQRStTUVWXYZ.,'c-#_>:<!+ ", 
                             QImage(":/resources/graphics/fontwhite.png")
                             );
    
    QRectF ui_field = QRectF(0, 0, 160, 144);
    QRect scaled_ui_field = TO_QRECT(ui_field, 1);
    
    double aspect_ratio = ui_field.width()*1.0/ui_field.height();
    double aspect_ratio_epsilon = aspect_ratio - (ui_field.width()-1)*1.0/ui_field.height();
    
    const double min_graphics_scale = 1;
    
    double ui_scale = min_graphics_scale;
    
signals:    
    void close();
    
    void stateEnd(NT3_state_enum nextState);
    
    void setGeometry(int x, int y, int w, int h);
    
    void resize(const QSize size);    
};

#endif // NT3SCREEN_H
