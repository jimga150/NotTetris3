#ifndef LOGO_H
#define LOGO_H

#include "nt3screen.h"

class Logo : public NT3Screen
{
    Q_OBJECT
public:
    explicit Logo(QObject *parent = nullptr);
    
    void init(QScreen* screen) override;
    
    void resizeEvent(QResizeEvent* event) override;    
    
    void render(QPainter& painter) override;
    
    void keyPressEvent(QKeyEvent* ev) override;
    
    void keyReleaseEvent(QKeyEvent* ev) override;
    
    void doGameStep() override;
    
    
    const QRectF ui_field = QRectF(0, 0, 160, 144);
    QRect scaled_ui_field = TO_QRECT(ui_field, 1);
    
    const double aspect_ratio = ui_field.width()*1.0/ui_field.height();
    const double aspect_ratio_epsilon = aspect_ratio - (ui_field.width()-1)*1.0/ui_field.height();
    
    const double min_graphics_scale = 1;
    
    double ui_scale = min_graphics_scale;
    
    
    double framerate;
    double time_passed = 0; //seconds
    
    const QString logo_path = ":/resources/graphics/stabyourselflogo.png";
    const QPixmap logo = QPixmap(logo_path);
    
    const double logo_slide_duration = 1.5; //seconds
    const double logo_delay = 1; //seconds
    
    const QRect logo_rect_final = QRect(QPoint(7, 58), logo.size());
    
    QRect scaled_logo_rect_final;
    double logo_offset_y;
    double logo_offset_delta; //UI pixels/sec
};

#endif // LOGO_H
