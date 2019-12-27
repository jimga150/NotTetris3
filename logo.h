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
    
    
    const QRectF ui_field = QRectF(0, 0, 160, 144);
    QRect scaled_ui_field = TO_QRECT(ui_field, 1);
    
    const double aspect_ratio = ui_field.width()*1.0/ui_field.height();
    const double aspect_ratio_epsilon = aspect_ratio - (ui_field.width()-1)*1.0/ui_field.height();
    
    const double min_graphics_scale = 1;
    
    double ui_scale = min_graphics_scale;
    
    
    QString logo_path = ":/resources/graphics/stabyourselflogo.png";
    QPixmap logo = QPixmap(logo_path);
    
    QRect logo_rect_final = QRect(QPoint(7, 58), logo.size());
    QRect scaled_logo_rect_final = TO_QRECT(logo_rect_final, 1);
    int logo_offset_y = -logo_rect_final.y() - logo_rect_final.height();
    
signals:
    void setTitle(QString title);
    
    void close();
    
    void stateEnd(NT3_state_enum nextState);
    
    void setGeometry(int x, int y, int w, int h);
    
    void resize(const QSize size);
    
    void setExpectedFrameTime(int millis);
    
};

#endif // LOGO_H
