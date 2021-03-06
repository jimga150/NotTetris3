#include "imagefont.h"

ImageFont::ImageFont(const char* glyphs, QImage font_image){
    
    this->height_px = font_image.height();
    
    int glyph_index = 0;
    
    //printf("Glyph string is %s\n", glyphs);
    
    //assumes that the image starts with a column of the separation color
    QRgb separation_color = font_image.pixel(0, 0);
    
    //iterate over top row of image
    for (int x = 1; x < font_image.width(); ++x){
        
        //detect first column of new glyph
        if (font_image.pixel(x, 0) != separation_color){
            
            //find the end column of the glyph
            int past_x;
            for (past_x = x; past_x < font_image.width(); ++past_x){
                if (font_image.pixel(past_x, 0) == separation_color){
                    break;
                }
            }
            
            //make the pixel map for the glyph
            QPixmap glyph_image = QPixmap::fromImage(font_image.copy(x, 0, past_x - x, font_image.height()));
            
            //pair it with its proper character
            char c = glyphs[glyph_index];
            
            if (this->characters.contains(c)){
                fprintf(stderr, "Font already contains %c (%d), skipping...\n", c, c);
            } else {
                //printf("Insert %c (%d)\n", c, c);            
                this->characters.insert(c, glyph_image);
            }
            
            ++glyph_index;
            
            if (!c) break; //null terminator reached
            
            //skip to the next separator
            x = past_x - 1;
        }
    }
    
    this->background_color = this->getPixmapFor(' ').toImage().pixelColor(0, 0);
    
    this->font_height = font_image.height();
}

void ImageFont::print(QPainter* painter, QPoint start, alignment_enum alignment, QString string, double sx, double sy){
    
    painter->save();
    
    painter->translate(start);
    
    if (sy == 0){
        sy = sx;
    }
    painter->scale(sx, sy);
    
    int string_width = 0;
    for (int i = 0; i < string.length(); i++){
        QPixmap glyph_pixmap = this->getPixmapFor(static_cast<char>(string[i].unicode()));
        string_width += glyph_pixmap.width() + 1;
    }
    
    painter->setBrush(this->background_color);
    painter->setPen(this->background_color);
    
    if (alignment == RIGHT_ALIGN){
        
        painter->drawRect(QRect(-string_width, 0, string_width, this->font_height));
        
        for (int i = string.length()-1; i >= 0; i--){
            
            QPixmap glyph_pixmap = this->getPixmapFor(static_cast<char>(string[i].unicode()));
            
            painter->translate(-(glyph_pixmap.width()+1), 0);
            painter->drawPixmap(glyph_pixmap.rect(), glyph_pixmap);
        }
    } else {
        
        painter->drawRect(QRect(0, 0, string_width, this->font_height));
        
        for (int i = 0; i < string.length(); i++){
            
            QPixmap glyph_pixmap = this->getPixmapFor(static_cast<char>(string[i].unicode()));
            
            painter->drawPixmap(glyph_pixmap.rect(), glyph_pixmap);
            painter->translate(glyph_pixmap.width()+1, 0);
        }
    }
    
    painter->restore();
}

QPixmap ImageFont::getPixmapFor(char c){    
    if (this->characters.contains(c)){
        return this->characters.value(c);
    } 
    
    if (this->isAlpha(c)){
        char othercase = this->switchCase(c);        
        if (this->characters.contains(othercase)){
            return this->characters.value(othercase);
        }
        fprintf(stderr, "\'%c\' (%u) AND \'%c\' (%u) not found in glyph list for Image Font!\n", 
                c, c, othercase, othercase);
    } else {
        fprintf(stderr, "\'%c\' (%u) not found in glyph list for Image Font!\n", c, c);
    }
    return QPixmap();
}

char ImageFont::switchCase(char c){
    if (c <= 'Z'){
        return c + this->case_difference;
    }
    return c - this->case_difference;
}

bool ImageFont::isAlpha(char c){
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}