#ifndef NT3SCREEN_H
#define NT3SCREEN_H

#include <QResizeEvent>
#include <QPainter>
#include <QScreen>
#include <QMediaPlayer>
#include <QAudioOutput>

#include "common.h"
#include "imagefont.h"

class NT3Screen : public QObject
{
    Q_OBJECT
public:
    explicit NT3Screen(QObject *parent = nullptr);
    virtual ~NT3Screen();
    
    virtual void init();
    
    
    virtual void calcScaleFactors();    
    
    virtual void render(QPainter& painter);
    
    virtual void colorizeResources();
    
    
    virtual void keyPressEvent(QKeyEvent* ev);
    
    virtual void keyReleaseEvent(QKeyEvent* ev);
    
    virtual void doGameStep();
    
    virtual void resetBlinkTimer();
    
    
    bool lockAR(QSize newSize);
    
    QPixmap colorize(QPixmap pixmap);
    
    
    QUrl music_path;
        
    const double select_blink_rate = 0.29; //seconds
    double blink_timer = 0;
    bool blink_on;
    
    ImageFont BOW_font = ImageFont(
                             "0123456789abcdefghijklmnopqrstTuvwxyz.,'C-#_>:<! ", 
                             QImage(":/resources/graphics/font.png")
                             );
    ImageFont WOB_font = ImageFont(
                             "0123456789abcdefghijklmnopqrstTuvwxyz.,'C-#_>:<!+ ", 
                             QImage(":/resources/graphics/fontwhite.png")
                             );
    
    QRectF ui_field_in = QRectF(0, 0, 160, 144);
    QRectF ui_field_px = SCALE_QRECTF(ui_field_in, 1);
    
    double aspect_ratio = ui_field_in.width()*1.0/ui_field_in.height();
    double aspect_ratio_epsilon = aspect_ratio - (ui_field_in.width()-1)*1.0/ui_field_in.height();
    
    const double min_graphics_scale_px_in = 1;
    
    double ui_to_screen_scale_px_in = min_graphics_scale_px_in;
    
    const QString default_option_name = "error";
        
signals:    
    void close();
    
    void stateEnd(NT3_state_enum nextState, bool stopMusic = true);
        
    void resize(const QSize size);
    
    void changeMusic(QUrl new_path);
};

#endif // NT3SCREEN_H
