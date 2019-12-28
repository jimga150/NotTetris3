#ifndef IMAGEFONT_H
#define IMAGEFONT_H

#include <QPixmap>
#include <QImage>
#include <QColor>
#include <QPainter>

#include <QHash>

enum alignment_enum{
    LEFT_ALIGN = 0,
    RIGHT_ALIGN,
    
    num_alignments
};

class ImageFont
{
public:
    ImageFont(const char* glyphs, QImage font_image);

    void print(QPainter* painter, QPoint start, alignment_enum alignment, QString string, double sx = 1.0, double sy = 0);


    QHash<char, QPixmap> characters;
    
    int height_px;
    
private:
    
    QPixmap getPixmapFor(char c);
};

#endif // IMAGEFONT_H
