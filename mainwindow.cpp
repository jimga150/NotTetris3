#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    b2Vec2 gravity(0.0f, 98.0f);
    this->world = new b2World(gravity);
    this->contactlistener = new NT3ContactListener;
    this->world->SetContactListener(this->contactlistener);

    this->initializeTetrisPieceDefs();

    b2BodyDef groundBodyDef;
    groundBodyDef.position.Set(100.0f, 210.0f);
    this->groundBody = world->CreateBody(&groundBodyDef);

    b2PolygonShape groundBox;
    groundBox.SetAsBox(50.0f, 10.0f);
    this->groundBody->CreateFixture(&groundBox, 0.0f);

    this->thingy = this->makeTetrisPiece(T);

    connect(&this->frameTrigger, &QTimer::timeout, this, &MainWindow::calcNextFrame);
    this->frameTrigger.start(this->framerate*this->millis_per_second);
    this->frameTimer.start();
}

MainWindow::~MainWindow()
{
    delete ui;
    if (this->world) destroy(world);
    if (this->contactlistener) destroy(contactlistener);
}

void MainWindow::calcNextFrame(){
    printf("Frame took %lld ms\n", this->frameTimer.elapsed());
    this->frameTimer.restart();

    world->Step(this->timeStep, this->velocityIterations, this->positionIterations);
    //printf("World step took %lld ms\n", this->frameTimer.elapsed());

    //printf("(%f, %f)\n", this->thingy->GetPosition().x, this->thingy->GetPosition().y);

    this->update();
}

void MainWindow::paintEvent(QPaintEvent *){
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    //draw everything on painter, and then exit the function. The scene will automatically render if its allowed to return to the QT main loop

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(255, 255, 255));
    painter.fillRect(0, 0, width(), height(), Qt::white);

    painter.setPen(Qt::SolidLine);
    painter.setBrush(QColor(0, 0, 0));

    for (b2Body* b = this->world->GetBodyList(); b; b = b->GetNext()){
        this->drawBodyTo(&painter, b);
    }
}

void MainWindow::drawBodyTo(QPainter* painter, b2Body* body){

    painter->save();
    painter->translate(body->GetPosition().x, body->GetPosition().y);
    painter->rotate(body->GetAngle()*180.0/M_PI);

    for (b2Fixture* f = body->GetFixtureList(); f; f = f->GetNext()){
        switch(f->GetType()){
        case b2Shape::e_polygon:{
            b2PolygonShape shape = *(b2PolygonShape*)f->GetShape();
            int numpoints = shape.m_count;
            QPointF points[numpoints];
            for (int i = 0; i < numpoints; i++){
                points[i] = QPointF(shape.m_vertices[i].x, shape.m_vertices[i].y);
                //printf("Point: (%f, %f)\n", points[i].x(), points[i].y());
            }
            painter->drawPolygon(points, numpoints);
        }
            break;
        case b2Shape::e_circle:{
            b2CircleShape shape = *(b2CircleShape*)f->GetShape();
            QPointF center(shape.m_p.x, shape.m_p.y);
            painter->drawEllipse(center, shape.m_radius, shape.m_radius);
        }
            break;
        case b2Shape::e_edge:{
            b2EdgeShape shape = *(b2EdgeShape*)f->GetShape();
            painter->drawLine(shape.m_vertex1.x, shape.m_vertex1.y, shape.m_vertex2.x, shape.m_vertex2.y);
        }
            break;
        case b2Shape::e_chain:{
            b2ChainShape shape = *(b2ChainShape*)f->GetShape();
            QPainterPath path;
            path.moveTo(shape.m_vertices[0].x, shape.m_vertices[0].y);
            for (int i = 1; i < shape.m_count; i++){
                path.lineTo(shape.m_vertices[i].x, shape.m_vertices[i].y);
            }
            painter->drawPath(path);
        }
            break;
        default:
            fprintf(stderr, "Fixture in body has undefined shape type!\n");
            return;
        }
    }
    painter->restore();
}

void MainWindow::initializeTetrisPieceDefs(){

    this->tetrisBodyDef.type = b2_dynamicBody;
    this->tetrisBodyDef.allowSleep = true;
    this->tetrisBodyDef.awake = true;
    this->tetrisBodyDef.position.Set(100.0f+0.0f, 100.0f+4.0f);

    for (int i = 0; i < num_tetris_pieces; i++){
        std::vector<b2PolygonShape> polyshapevect;
        this->tetrisShapes.push_back(polyshapevect);
    }

    b2PolygonShape shape_template;

    this->tetrisShapes.at(I).push_back(shape_template);
    this->tetrisShapes.at(I).at(0).SetAsBox(this->side_length*2, this->side_length/2);

    this->tetrisShapes.at(O).push_back(shape_template);
    this->tetrisShapes.at(O).at(0).SetAsBox(this->side_length, this->side_length);

    this->tetrisShapes.at(G).push_back(shape_template);
    this->tetrisShapes.at(G).push_back(shape_template);
    this->tetrisShapes.at(G).at(0).SetAsBox(3*this->side_length/2, this->side_length/2);
    this->tetrisShapes.at(G).at(1).SetAsBox(this->side_length/2, this->side_length/2, b2Vec2(this->side_length, this->side_length), 0);

    this->tetrisShapes.at(L).push_back(shape_template);
    this->tetrisShapes.at(L).push_back(shape_template);
    this->tetrisShapes.at(L).at(0).SetAsBox(3*this->side_length/2, this->side_length/2);
    this->tetrisShapes.at(L).at(1).SetAsBox(this->side_length/2, this->side_length/2, b2Vec2(-this->side_length, this->side_length), 0);

    this->tetrisShapes.at(Z).push_back(shape_template);
    this->tetrisShapes.at(Z).push_back(shape_template);
    this->tetrisShapes.at(Z).at(0).SetAsBox(this->side_length, this->side_length/2, b2Vec2(-this->side_length/2, -this->side_length/2), 0);
    this->tetrisShapes.at(Z).at(1).SetAsBox(this->side_length, this->side_length/2, b2Vec2(this->side_length/2, this->side_length/2), 0);

    this->tetrisShapes.at(S).push_back(shape_template);
    this->tetrisShapes.at(S).push_back(shape_template);
    this->tetrisShapes.at(S).at(0).SetAsBox(this->side_length, this->side_length/2, b2Vec2(-this->side_length/2, this->side_length/2), 0);
    this->tetrisShapes.at(S).at(1).SetAsBox(this->side_length, this->side_length/2, b2Vec2(this->side_length/2, -this->side_length/2), 0);

    this->tetrisShapes.at(T).push_back(shape_template);
    this->tetrisShapes.at(T).push_back(shape_template);
    this->tetrisShapes.at(T).at(0).SetAsBox(3*this->side_length/2, this->side_length/2);
    this->tetrisShapes.at(T).at(1).SetAsBox(this->side_length/2, this->side_length/2, b2Vec2(0, this->side_length), 0);

    b2FixtureDef fixture_template;
    std::vector<b2FixtureDef> fixture_vector_template;
    for (int t = 0; t < this->tetrisShapes.size(); t++){
        this->tetrisFixtures.push_back(fixture_vector_template);
        for (int s = 0; s < this->max_shapes_per_piece; s++){
            if (this->tetrisShapes.at(t).size() == s) break;
            this->tetrisFixtures.at(t).push_back(fixture_template);
            this->tetrisFixtures.at(t).at(s).shape = &this->tetrisShapes.at(t).at(s);
            this->tetrisFixtures.at(t).at(s).density = 10.0f;
            this->tetrisFixtures.at(t).at(s).friction = 0.3f;
        }
    }
}

b2Body* MainWindow::makeTetrisPiece(tetris_piece_enum type){
    b2Body* ans = world->CreateBody(&this->tetrisBodyDef);
    ans->ApplyTorque(30000000, true);
    for (b2FixtureDef f : this->tetrisFixtures.at(type)){
        ans->CreateFixture(&f);
    }
    return ans;
}
