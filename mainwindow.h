#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>
#include <QtWidgets>
#include <QThread>
#include <QMainWindow>

#include "Box2D/Box2D.h"

#include "nt3contactlistener.h"

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

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    void drawBodyTo(QPainter* painter, b2Body *body);

    void initializeTetrisPieceDefs();

    b2Body* makeTetrisPiece(tetris_piece_enum type);

    float32 side_length = 10;

    int max_shapes_per_piece = 2;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    Ui::MainWindow *ui;

    const int millis_per_second = 1000;

    double fps = 60;
    float32 timeStep = 1.0f / fps;
    double framerate = timeStep; //seconds

    int32 velocityIterations = 6;
    int32 positionIterations = 2;

    b2BodyDef tetrisBodyDef;
    std::vector<std::vector<b2FixtureDef>> tetrisFixtures;
    std::vector<std::vector<b2PolygonShape>> tetrisShapes;

    b2World* world = nullptr;
    NT3ContactListener* contactlistener = nullptr;
    std::vector<b2Body*> bodies;

    b2Body* groundBody = nullptr;
    b2Body* thingy = nullptr;

    QTimer frameTrigger;
    QElapsedTimer frameTimer;

private slots:
    void calcNextFrame();
};
#endif // MAINWINDOW_H
