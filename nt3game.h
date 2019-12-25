#include <QScreen>
#include <QResizeEvent>

#include <QRandomGenerator>
#include <QElapsedTimer>
#include <QtMath>

#include "Box2D/Box2D.h"

#include "opengl2dwindow.h"
#include "nt3contactlistener.h"
#include "imagefont.h"

//#define TIME_GAME_FRAME 1

#define NUM_FRAMES_TO_SAVE 20

#define TO_QRECT(QRF, SCALE) QRect(static_cast<int>(QRF.x()*SCALE), static_cast<int>(QRF.y()*SCALE), static_cast<int>(QRF.width()*SCALE), static_cast<int>(QRF.height()*SCALE))

using namespace std;

enum nt3_state_enum{
    gameA = 0,
    row_clear_blinking,
    
    num_nt3_states
};

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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
static tetris_piece_enum default_tetris_piece = I;
#pragma GCC diagnostic push

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

enum ray_cast_enum{
    TOPLEFT = 0,
    TOPRIGHT,
    BOTTOMLEFT,
    BOTTOMRIGHT,
    
    num_ray_casts
};

enum line_cut_side_enum{
    BOTTOM = 0,
    TOP,
    
    num_line_cut_sides
};

struct row_sides_struct{
    float32 top = 0;
    float32 bottom = 0;
    
    row_sides_struct(){}
    
    row_sides_struct(uint row, float32 side_length){
        this->top = (row+1)*side_length + numeric_limits<float32>::epsilon();
        this->bottom = row*side_length - numeric_limits<float32>::epsilon();
    }
};

struct rayCastComplete{
    b2RayCastInput input;
    b2RayCastOutput output;
    bool hit;
    
    void doRayCast(b2Shape* s, b2Body* b){
        b2Transform t;
        t.Set(b->GetPosition(), b->GetAngle());
        this->hit = s->RayCast(&this->output, this->input, t, 0);
    }
};

struct tetrisPieceData{
    QPixmap image;
    QRect region;
};

class NT3Game : public OpenGL2DWindow
{
public:
    
    //constructor/destructor
    explicit NT3Game();
    ~NT3Game() override;
    
    
    //Destructor utility
    void freeUserDataOn(b2Body* b);
    
    
    //graphics
    void render(QPainter& painter) override;
    
    void drawBodyTo(QPainter* painter, b2Body *body);
    
    void drawTetrisPiece(QPainter* painter, b2Body *piece_body);
    
    void drawScore(QPainter* painter);
    
    
    //game logic
    void doGameStep() override;
    
    
    //calculating/removing rows
    float32 getRowArea(uint row);
    
    void clearRow(uint row);
    
    QPixmap maskImage(b2Body* b, tetrisPieceData* data);
    
    vector<rayCastComplete> getRayCasts(float32 top, float32 bot);
    
    b2Vec2 hit_point(rayCastComplete ray_cast);
    
    float32 poly_area(b2Vec2* vertices, int count);
    
    
    //adding new pieces
    void makeNewTetrisPiece();
    
    
    //utilities
    bool isAWall(b2Body* b);
    
    QString b2Vec2String(b2Vec2 vec);
    
    tetrisPieceData *getTetrisPieceData(b2Body* b);
    
    QPixmap enableAlphaChannel(QPixmap pixmap);
    
    void setGameState(nt3_state_enum newstate);
    
    
    //initialization functions
    void initializeTetrisPieceDefs();
    
    void initializeTetrisPieceImages();
    
    void initializeWalls();
    
    void init_BDC();
    //end functions
    
    
    
    //debug
    bool debug_framerate = true;
    
    bool debug_box2d = false;
    
    bool frame_review = false;
    
    
    //constants
    const int millis_per_second = 1000;
    const int micros_per_second = millis_per_second*1000;
    const int nanos_per_second = micros_per_second*1000;
    
    const double rad_to_deg = 180.0/M_PI;
    
    
    //calculated timings
    double fps;
    
    float32 timeStep;
    
    double framerate; //seconds
    
    
    //input states/params
    bool accelDownState = false;
    rotate_state_enum rotateState = NO_ROTATION;
    lateral_movement_state_enum lateralMovementState = NO_LATERAL_MOVEMENT;
    
    int freeze_key;
    int accelDownKey;
    QHash<int, rotate_state_enum> rotateStateTable;
    QHash<int, lateral_movement_state_enum> lateralMovementStateTable;
    
    
    //Box2d params
    const int32 default_velocityIterations = 8;
    const int32 default_positionIterations = 3;
    
    int32 velocityIterations = default_velocityIterations;
    int32 positionIterations = default_positionIterations;
    
    
    //Box2d data
    b2BodyDef tetrisBodyDef;
    vector<vector<b2FixtureDef>> tetrisFixtures;
    vector<vector<b2PolygonShape>> tetrisShapes;
    
    b2World* world = nullptr;
    NT3ContactListener* contactlistener = nullptr;
    vector<b2Body*> bodies;
    
    b2Body* walls[num_walls];
    
    b2Body* currentPiece = nullptr;
    
    //Index: Row box # (up to tetris_rows-1)
    vector<float32> row_areas;
    vector<QHash<b2Body*, float32>> body_area_contributions;
    
    
    //physical properties of graphics and world
    
    const uint tetris_rows = 18;
    const uint tetris_cols = 10;
    
    const uint row_fill_density_col_width = 6;
    
    //must be an even number greater than 1 and less than 7.6175362031343
    //so basically 2, 4, or 6 meters is it right now
    float32 side_length = 2.0f;  //meters
    double side_length_dbl = static_cast<double>(side_length);
    
    const QRectF tetris_field = QRectF(
                                    1.75*side_length_dbl, 
                                    0, 
                                    (tetris_cols + 0.25)*side_length_dbl, 
                                    tetris_rows*side_length_dbl
                                    );
    QRect scaled_tetris_field = TO_QRECT(tetris_field, 1);
    
    float32 square_area = side_length*side_length;
    
    const float32 min_poly_area = (1.0f/40.0f)*square_area;
    
    int polygon_radius_px = 1;
    
    const QRectF ui_field = QRectF(0, 0, 160, 144);
    QRect scaled_ui_field = TO_QRECT(ui_field, 1);
    
    const double aspect_ratio = ui_field.width()*1.0/ui_field.height();
    const double aspect_ratio_epsilon = aspect_ratio - (ui_field.width()-1)*1.0/ui_field.height();
    
    const double min_graphics_scale = 1;
    
    const double pis_factor = 2; //multiplier used on initial graphics scale to deterimine piece_image_scale
    double piece_image_scale = 10; //scale used to make tetris piece cutting smooth
    
    double ui_scale = min_graphics_scale;
    const double physics_to_ui_scale = ui_field.height()/tetris_field.height();
    double physics_scale = physics_to_ui_scale*ui_scale;
    
    b2Vec2 piece_start = b2Vec2(static_cast<float32>(this->tetris_field.width()/2), -this->side_length*2);
    
    QPoint score_display_right = QPoint(151, 24);
    
    QRect score_add_display = QRect(QPoint(109, 35), QSize(46, 10));
    
    QPoint sc_add_right_in_disp = QPoint(42, 1);
    
    int score_add_disp_offset = 0;
    
    QPoint lines_cleared_disp_offset = QPoint(144, 80);
    
    QPoint level_disp_offset = QPoint(144, 56);
    
    
    //Game state
    nt3_state_enum game_state = gameA;
    nt3_state_enum last_state = gameA;
    
    const float32 avgarea_divisor = square_area*10;
    
    int current_score = 0;
    
    int score_to_add = 0;
    
    int lines_cleared = 0;
    
    int current_level = 0;
    
    
    //physics constants
    
    const float32 old_g = 500;
    const float32 old_start_y = -64;
    const float32 old_game_height = 640;    
    float32 gravity_g = old_g*(static_cast<float32>(tetris_field.height()) - piece_start.y)/(old_game_height - old_start_y);
    
    float32 density = 1;//1.0f/900.0f;
    
     
    
    float32 wmax = 3.0f;
    float32 angular_accel = 9.55342974715892f;
    
    float32 lateral_accel = 11.25f*side_length;
    
    float32 downward_accel = 14.441875f*side_length;
    
    float32 upward_correcting_accel = 50.87875f*side_length; 
    
    float32 downward_velocity_max = 15.753125f*side_length; 
    float32 downward_velocity_regular = 2.86853125f*side_length;
    
    float32 piece_friction_k = 0.5f; //Box2D uses the same k for static and dynamic friction, unfortunately
    float32 ground_friction_k = 0.5f;
    
    float32 restitution = 0.01f;
    
    float32 linear_damping = 0.5f;
    float32 angular_damping = 0.1f;
    
    float32 line_clear_threshold = 8.1f*square_area;
    
    
    //piece params
    const uint32 max_shapes_per_piece = 4;
    
    QRandomGenerator rng = QRandomGenerator::securelySeeded();
    
    
    //resources
    QColor debug_line_color = QColor(255, 0, 0);
    
    QString gamebackground_path = ":/resources/graphics/gamebackgroundgamea.png";
    QPixmap gamebackground = QPixmap(gamebackground_path);
    
    vector<QPixmap> piece_images;
    vector<QRect> piece_rects;
    
    QPixmap default_piece_image;
    QRect default_piece_rect;
    
    QImage black_font_img = QImage(":/resources/graphics/font.png");
    ImageFont BOW_font = ImageFont("0123456789ABCDEFGHIJKLMNOPQRStTUVWXYZ.,'c-#_>:<! ", black_font_img);
    
    QImage white_font_img = QImage(":/resources/graphics/fontwhite.png");
    ImageFont WOB_font = ImageFont("0123456789ABCDEFGHIJKLMNOPQRStTUVWXYZ.,'c-#_>:<!+ ", white_font_img);
    
    
    //Line clear stuff
    QColor line_clear_color = QColor(0xd2af8f);
    
    const double lc_blink_toggle_time = 0.17142857142857; //seconds
    
    const uint num_blinks = 4; //number of blinks to do on row clear
    
    uint num_blinks_so_far = 0; //blink count accumulator
    
    vector<bool> rows_to_clear;
    
    double row_blink_accumulator = 0; //seconds
    
    bool blink_on = true; //blink state
    
    
    //freeze frame data
    bool freeze_frame = false;
    
    QPixmap saved_frames[NUM_FRAMES_TO_SAVE];
    
    int last_frame = 0;
    
protected:
    void resizeEvent(QResizeEvent* event) override;
    
    void keyPressEvent(QKeyEvent* ev) override;
    
    void keyReleaseEvent(QKeyEvent* ev) override;
    
};

