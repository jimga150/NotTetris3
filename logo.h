#ifndef LOGO_H
#define LOGO_H

#include <QScreen>
#include <QPainter>
#include <QResizeEvent>

#include "common.h"

class Logo : public QObject
{
    Q_OBJECT
public:
    explicit Logo(QObject *parent = nullptr);
    ~Logo();
    
    void init(QScreen* screen);
    
    void resizeEvent(QResizeEvent* event);    
    
    void render(QPainter& painter);
    
    void keyPressEvent(QKeyEvent* ev);
    
    void keyReleaseEvent(QKeyEvent* ev);
    
    void doGameStep();
    
    
    double framerate;
    double time_passed = 0; //seconds
    
    const QRectF ui_field = QRectF(0, 0, 160, 144);
    QRect scaled_ui_field = TO_QRECT(ui_field, 1);
    
    const double aspect_ratio = ui_field.width()*1.0/ui_field.height();
    const double aspect_ratio_epsilon = aspect_ratio - (ui_field.width()-1)*1.0/ui_field.height();
    
    const double min_graphics_scale = 1;
    
    double ui_scale = min_graphics_scale;
    
    
    const QString logo_path = ":/resources/graphics/stabyourselflogo.png";
    const QPixmap logo = QPixmap(logo_path);
    
    const double logo_slide_duration = 1.5; //seconds
    const double logo_delay = 1; //seconds
    
    const QRect logo_rect_final = QRect(QPoint(7, 58), logo.size());
    
    QRect scaled_logo_rect_final;
    double logo_offset_y;
    double logo_offset_delta; //UI pixels/sec
    
signals:
    void setTitle(QString title);
    
    void close();
    
    void stateEnd(NT3_state_enum nextState);
    
    void setGeometry(int x, int y, int w, int h);
    
    void resize(const QSize size);
    
    void setExpectedFrameTime(int millis);
    
};

#endif // LOGO_H
