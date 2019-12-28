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
    
    QString credits_text[16] = {
        "'Tm and C2011 sy,not",
        "tetris 2 licensed to",
        "  stabyourself.net  ",
        "         and        ",
        "  sub-licensed to   ",
        "      maurice.      ",
        "                    ",
        " C2011 stabyourself ",
        "       dot net.     ",
        "                    ",
        "                    ",
        "all rights reserved.",
        "                    ",
        "  original concept, ",
        " design and program ",
        "by alexey pazhitnov#"
    };
};

#endif // CREDITS_H
