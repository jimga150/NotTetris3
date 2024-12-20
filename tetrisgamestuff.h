#ifndef TETRISGAMESTUFF_H
#define TETRISGAMESTUFF_H

#include <QPixmap>
#include <QImage>
#include <QPainter>

enum tetris_piece_enum{
    I = 0, //Long skinny piece
    O, //2x2 square
    G, //¬ piece, or a backwards L. G stands for capital Gamma.
    L,
    Z,
    S,
    T,
    
    num_tetris_pieces
};

const static tetris_piece_enum default_tetris_piece = I;

enum wall_enum{
    GROUND = 0,
    LEFTWALL,
    RIGHTWALL,
    
    num_walls
};

enum rotate_state_enum{
    NO_ROTATION = 0,
    ROTATECCW,
    ROTATECW,
    BOTH_ROTATIONS,
    
    num_rotate_states
};

enum lateral_movement_state_enum{
    NO_LATERAL_MOVEMENT = 0,
    MOVELEFT,
    MOVERIGHT,
    BOTH_DIRECTIONS,
    
    num_lateral_movement_states
};

enum sound_effect_enum{
    BLOCK_TURN = 0,
    BLOCK_MOVE,
    BLOCK_FALL,
    LINE_CLEAR,
    FOUR_LINE_CLEAR,
    GAME_OVER_SOUND,
    PAUSE_SOUND,
    NEW_LEVEL,
    
    num_sound_effects
};

enum powerup_type_enum{
    NOT_A_POWERUP = 0,
    DIAG_CUT,
    EARTHQUAKE,

    num_powerup_types
};

struct tetrisPieceData {
    
    QPixmap image;
    
    QImage image_in_waiting;
    
    QRectF region_m;

    powerup_type_enum powerup;
    
    tetrisPieceData(){}
    
    tetrisPieceData(QPixmap image, QRectF region, powerup_type_enum powerup){
        this->image = image;
        this->region_m = region;
        this->powerup = powerup;
    }
    
    bool operator==(const tetrisPieceData& other) const{
        return this->powerup == other.powerup && this->region_m == other.region_m && this->image.toImage() == other.image.toImage();
    }
    
    QImage get_image(){
        if (this->image_in_waiting.isNull()){
            return this->image.toImage();
        }
        return this->image_in_waiting;
    }
    
    void resolveImage(){
        if (!this->image_in_waiting.isNull()){
            
            this->image = QPixmap(this->image_in_waiting.size());
            this->image.fill(Qt::transparent);
            
            QPainter p(&this->image);
            p.drawImage(this->image_in_waiting.rect(), this->image_in_waiting);
            p.end();
            
            this->image_in_waiting = QImage();
        }
    }

    bool is_powerup(){
        return this->powerup != NOT_A_POWERUP;
    }
    
};

#endif // TETRISGAMESTUFF_H
