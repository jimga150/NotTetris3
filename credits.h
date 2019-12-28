#ifndef CREDITS_H
#define CREDITS_H

#include "nt3screen.h"

class Credits : public NT3Screen
{
    Q_OBJECT
public:
    explicit Credits(QObject *parent = nullptr);
    
    void init() override;
        
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
        
    const double credits_delay = 2; //seconds
    double time_passed = 0;
    
    const QString logo_path = ":/resources/graphics/logo.png";
    const QPixmap logo = QPixmap(logo_path);
};

#endif // CREDITS_H
