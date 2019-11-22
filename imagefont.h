#ifndef IMAGEFONT_H
#define IMAGEFONT_H

#include <QPixmap>
#include <QImage>
#include <QColor>
#include <QPainter>

#include <QHash>

class ImageFont
{
public:
    ImageFont(const char* glyphs, QImage font_image);

    void print(QPainter* painter, QPoint start, bool right_align, QString string, double sx = 1.0, double sy = 0);


    QHash<char, QPixmap> characters;
    
private:
    
    QPixmap getPixmapFor(char c);
};

#endif // IMAGEFONT_H
