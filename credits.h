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
    
    const double credits_delay = 2; //seconds
    double time_passed = 0;
    
    const QString logo_path = ":/resources/graphics/logo.png";
    const QPixmap logo = QPixmap(logo_path);
    
    const QPoint logo_pos = QPoint(32, 80);
};

#endif // CREDITS_H
