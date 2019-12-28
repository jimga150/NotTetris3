#ifndef LOGO_H
#define LOGO_H

#include "nt3screen.h"

class Logo : public NT3Screen
{
    Q_OBJECT
public:
    explicit Logo(QObject *parent = nullptr);
    
    void init() override;
    
    void resizeEvent(QResizeEvent* event) override;    
    
    void render(QPainter& painter) override;
    
    void keyPressEvent(QKeyEvent* ev) override;
        
    void doGameStep() override;
    
    
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
