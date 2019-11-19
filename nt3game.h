#include <QScreen>
#include <QResizeEvent>

#include <QRandomGenerator>

#include "Box2D/Box2D.h"

#include "opengl2dwindow.h"
#include "nt3contactlistener.h"

#define NUM_FRAMES_TO_SAVE 20

#define STD_VECTOR_ACCESS_TRYCATCH(STATEMENT) try { \
    STATEMENT; \
    } catch (std::out_of_range e) { \
    printf("Line %d: vector access failed: %s\n", __LINE__, e.what()); \
    }

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
    tetris_piece_enum type = I;
};

class NT3Game : public OpenGL2DWindow
{
public:

    //constructor/destructor
    explicit NT3Game();
    ~NT3Game() override;


    //graphics
    void render(QPainter& painter) override;

    void drawBodyTo(QPainter* painter, b2Body *body, bool debug_graphics);

    void drawTetrisPiece(QPainter* painter, b2Body *piece_body);


    //game logic
    void doGameStep() override;


    //calculating/removing rows
    float32 getRowDensity(uint row);

    void clearRow(uint row);

    std::vector<rayCastComplete> getRayCasts(float32 top, float32 bot);

    b2Vec2 hit_point(rayCastComplete ray_cast);

    float32 poly_area(b2Vec2* vertices, int count);


    //adding new pieces
    void makeNewTetrisPiece();


    //utilities
    bool isAWall(b2Body* b);

    QString b2Vec2String(b2Vec2 vec);

    tetrisPieceData *getTetrisPieceData(b2Body* b);


    //initialization functions
    void initializeTetrisPieceDefs();

    void initializeTetrisPieceImages();

    void initializeWalls();

    void init_BDC();
    //end functions


    //debug
    bool debug_graphics = true;


    //constants
    const int millis_per_second = 1000;
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
    std::vector<std::vector<b2FixtureDef>> tetrisFixtures;
    std::vector<std::vector<b2PolygonShape>> tetrisShapes;

    b2World* world = nullptr;
    NT3ContactListener* contactlistener = nullptr;
    std::vector<b2Body*> bodies;

    b2Body* walls[num_walls];

    b2Body* currentPiece = nullptr;

    //Index: Row box # (up to tetris_rows-1)
    std::vector<float32> row_densities;
    std::vector<QHash<b2Body*, float32>> body_density_contributions;


    //physical properties of graphics and world

    const uint tetris_rows = 18;
    const uint tetris_cols = 10;

    const uint row_fill_density_col_width = 6;

    const QRect tetris_field = QRect(14, 0, 82, 144);
    QRect scaled_tetris_field = tetris_field;

    float32 side_length = static_cast<float32>(tetris_field.height()*1.0/tetris_rows);

    const QRect ui_field = QRect(0, tetris_field.y(), 160, tetris_field.height());
    QRect scaled_ui_field = ui_field;

    const double aspect_ratio = ui_field.width()*1.0/ui_field.height();
    const double aspect_ratio_epsilon = aspect_ratio - (ui_field.width()-1)*1.0/ui_field.height();

    const double min_graphics_scale = 1;
    double graphicsscale = 1;

    float32 piece_start_y = -side_length/2;


    //physics constants

    const float32 old_g = 500;
    const float32 old_start_y = -64;
    const float32 old_game_height = 640;
    float32 gravity_g = old_g*(-piece_start_y+tetris_field.height())/(-old_start_y+old_game_height);

    float32 density = 1.0f/900.0f;

    float32 wmax = 3.0f;
    float32 torque = 35*side_length;

    float32 lateral_force = 4.375f*side_length;

    float32 downward_force = 2.5f*side_length;
    float32 upward_correcting_force = 8*side_length;

    float32 downward_velocity_max = 15.625f*side_length;
    float32 downward_velocity_regular = 3.125f*side_length;

    float32 piece_friction_k = 0.5f; //Box2D uses the same k for static and dynamic friction, unfortunately
    float32 ground_friction_k = 0.5f;

    float32 restitution = 0.001f;

    float32 line_clear_threshold = 8.1f*side_length*side_length;


    //piece params
    const uint32 max_shapes_per_piece = 2;

    QRandomGenerator rng = QRandomGenerator::securelySeeded();


    //resources
    QColor debug_line_color = QColor(255, 0, 0);

    QString gamebackground_path = ":/resources/graphics/gamebackgroundgamea.png";
    QPixmap gamebackground = QPixmap(gamebackground_path);

    QString gameafield_path = ":/resources/graphics/gameafield.png";
    QPixmap gameafield = QPixmap(gameafield_path);

    std::vector<QPixmap> piece_images;
    std::vector<QRect> piece_rects;


    //freeze frame data
    bool row_cleared = false;
    bool freeze_frame = false;
    QPixmap saved_frames[NUM_FRAMES_TO_SAVE];
    int last_frame = 0;

protected:
    void resizeEvent(QResizeEvent* event) override;

    void keyPressEvent(QKeyEvent* ev) override;

    void keyReleaseEvent(QKeyEvent* ev) override;

};

