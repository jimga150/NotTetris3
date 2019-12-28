#ifndef CREDITS_H
#define CREDITS_H

#include "nt3screen.h"

class Credits : public NT3Screen
{
    Q_OBJECT
public:
    explicit Credits(QObject *parent = nullptr);
    
    void init(QScreen* screen) override;
    
    void resizeEvent(QResizeEvent* event) override;    
    
    void render(QPainter& painter) override;
    
    void keyPressEvent(QKeyEvent* ev) override;
    
    void doGameStep() override;
    
    
    static const int credits_numlines = 16;
    QString credits_text[credits_numlines] = {
        "'TM AND c2011 SY,NOT",
        "TETRIS 2 LICENSED TO",
        "  STABYOURSELF.NET  ",
        "         AND        ",
        "  SUB-LICENSED TO   ",
        "      MAURICE.      ",
        "                    ",
        " c2011 STABYOURSELF ",
        "       DOT NET.     ",
        "                    ",
        "                    ",
        "ALL RIGHTS RESERVED.",
        "                    ",
        "  ORIGINAL CONCEPT, ",
        " DESIGN AND PROGRAM ",
        "BY ALEXEY PAZHITNOV#"
        
    };
    
    int font_height;
    
    const QRectF ui_field = QRectF(0, 0, 160, 144);
    QRect scaled_ui_field = TO_QRECT(ui_field, 1);
    
    const double aspect_ratio = ui_field.width()*1.0/ui_field.height();
    const double aspect_ratio_epsilon = aspect_ratio - (ui_field.width()-1)*1.0/ui_field.height();    
    const double min_graphics_scale = 1;
    
    double ui_scale;
    
    const double credits_delay = 200; //seconds
    double framerate;
    double time_passed = 0;
};

#endif // CREDITS_H
