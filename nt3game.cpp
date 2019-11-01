/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the documentation of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "nt3game.h"

NT3Game::NT3Game()
{
    if (this->gamebackground.isNull()){
        printf("Couldnt find image at %s!\n", this->gamebackground_path.toUtf8().constData());
        return;
    }

    b2Vec2 gravity(0.0f, 4*9.8f);
    this->world = new b2World(gravity);
    this->contactlistener = new NT3ContactListener;
    this->world->SetContactListener(this->contactlistener);

    this->initializeTetrisPieceDefs();

    this->initializeTetrisPieceImages();

    this->initializeWalls();

    this->contactlistener->exceptions.push_back(this->walls[LEFTWALL]);
    this->contactlistener->exceptions.push_back(this->walls[RIGHTWALL]);

    this->makeNewTetrisPiece();
}

NT3Game::~NT3Game()
{
    if (this->world) delete world;
    if (this->contactlistener) delete contactlistener;
}

void NT3Game::resizeEvent(QResizeEvent* event){
    QSize newSize = event->size();
    int width = newSize.width();
    int height = newSize.height();

    if (width*1.0/height > aspect_ratio){ //screen is relatively wider than the app
        this->graphicsscale = height*1.0/this->ui_field.height();
    } else { //screen is relatively taller than app, or it's the same ratio
        this->graphicsscale = width*1.0/this->ui_field.width();
    }

    this->scaled_ui_field.setX(static_cast<int>(this->ui_field.x()*this->graphicsscale));
    this->scaled_ui_field.setY(static_cast<int>(this->ui_field.y()*this->graphicsscale));
    this->scaled_ui_field.setWidth(static_cast<int>(this->ui_field.width()*this->graphicsscale));
    this->scaled_ui_field.setHeight(static_cast<int>(this->ui_field.height()*this->graphicsscale));

    this->scaled_tetris_field.setX(static_cast<int>(this->tetris_field.x()*this->graphicsscale));
    this->scaled_tetris_field.setY(static_cast<int>(this->tetris_field.y()*this->graphicsscale));
    this->scaled_tetris_field.setWidth(static_cast<int>(this->tetris_field.width()*this->graphicsscale));
    this->scaled_tetris_field.setHeight(static_cast<int>(this->tetris_field.height()*this->graphicsscale));
}

void NT3Game::keyPressEvent(QKeyEvent* ev){
    printf("Key pressed: %s\n", ev->text().toUtf8().constData());
    fflush(stdout);

    switch(ev->key()){
    case Qt::Key_Z:
        if (this->currentPiece->GetAngularVelocity() < 3){
            this->currentPiece->ApplyTorque(3000000, true);
        }
        break;
    case Qt::Key_X:
        if (this->currentPiece->GetAngularVelocity() < -3){
            this->currentPiece->ApplyTorque(-3000000, true);
        }
        break;
    case Qt::Key_Up:
        //do nothing
        break;
    case Qt::Key_Down:

        break;
    case Qt::Key_Left:
        this->currentPiece->ApplyForce(b2Vec2(-700000, 0), this->currentPiece->GetWorldCenter(), true);
        break;
    case Qt::Key_Right:
        this->currentPiece->ApplyForce(b2Vec2(700000, 0), this->currentPiece->GetWorldCenter(), true);
        break;
    case Qt::Key_Space:

        break;
    }
}

void NT3Game::keyReleaseEvent(QKeyEvent* ev){

}


void NT3Game::render(QPainter& painter)
{
    painter.setRenderHint(QPainter::Antialiasing);

    painter.drawPixmap(0, 0, this->scaled_ui_field.width(), this->scaled_ui_field.height(), this->gamebackground);

    painter.setPen(Qt::SolidLine);
    painter.setPen(QColor(255, 0, 0));
    painter.setBrush(Qt::NoBrush);

    for (b2Body* b = this->world->GetBodyList(); b; b = b->GetNext()){
        this->drawTetrisPiece(&painter, b);
        //this->drawBodyTo(&painter, b);
    }
}


void NT3Game::doGameStep(){
    world->Step(static_cast<float32>(this->timeStep), this->velocityIterations, this->positionIterations);

    if (this->contactlistener->hasCurrentPieceCollided()){
        this->currentPiece->SetGravityScale(1);

        this->makeNewTetrisPiece();
    }
}

void NT3Game::drawBodyTo(QPainter* painter, b2Body* body){

    painter->save();
    painter->translate(
                this->scaled_tetris_field.x() + static_cast<double>(body->GetPosition().x)*this->graphicsscale,
                this->scaled_tetris_field.y() + static_cast<double>(body->GetPosition().y)*this->graphicsscale
                );
    painter->rotate(static_cast<double>(body->GetAngle())*rad_to_deg);

    for (b2Fixture* f = body->GetFixtureList(); f; f = f->GetNext()){
        switch(f->GetType()){
        case b2Shape::e_polygon:{
            b2PolygonShape shape = *static_cast<b2PolygonShape*>(f->GetShape());
            int numpoints = shape.m_count;
            std::vector<QPointF> points;
            for (int i = 0; i < numpoints; i++){
                points.push_back(
                            QPointF(
                                static_cast<double>(shape.m_vertices[i].x)*this->graphicsscale,
                                static_cast<double>(shape.m_vertices[i].y)*this->graphicsscale
                                )
                            );
                //printf("Point: (%f, %f)\n", points[i].x(), points[i].y());
            }
            painter->drawPolygon(&points[0], numpoints);
        }
            break;
        case b2Shape::e_circle:{
            b2CircleShape shape = *static_cast<b2CircleShape*>(f->GetShape());
            QPointF center(
                        static_cast<double>(shape.m_p.x)*this->graphicsscale,
                        static_cast<double>(shape.m_p.y)*this->graphicsscale
                        );
            double radius = static_cast<double>(shape.m_radius);
            radius *= this->graphicsscale;
            painter->drawEllipse(center, radius, radius);
        }
            break;
        case b2Shape::e_edge:{
            b2EdgeShape shape = *static_cast<b2EdgeShape*>(f->GetShape());
            QPointF p1 = QPointF(
                        static_cast<double>(shape.m_vertex1.x)*this->graphicsscale,
                        static_cast<double>(shape.m_vertex1.y)*this->graphicsscale
                        );
            QPointF p2 = QPointF(
                        static_cast<double>(shape.m_vertex2.x)*this->graphicsscale,
                        static_cast<double>(shape.m_vertex2.y)*this->graphicsscale
                        );
            painter->drawLine(p1, p2);
        }
            break;
        case b2Shape::e_chain:{
            b2ChainShape shape = *static_cast<b2ChainShape*>(f->GetShape());
            QPainterPath path;
            path.moveTo(
                        static_cast<double>(shape.m_vertices[0].x),
                        static_cast<double>(shape.m_vertices[0].y)
                        );
            for (int i = 1; i < shape.m_count; i++){
                path.lineTo(
                            static_cast<double>(shape.m_vertices[i].x)*this->graphicsscale,
                            static_cast<double>(shape.m_vertices[i].y)*this->graphicsscale
                            );
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

void NT3Game::drawTetrisPiece(QPainter* painter, b2Body* piece_body){

    for (uint i = 0; i < num_walls; i++){
        if (piece_body == this->walls[i]){
            return;
        }
    }

    tetris_piece_enum piece = this->bodytypes.value(piece_body, I);

    painter->save();

    painter->translate(
                this->scaled_tetris_field.x() + static_cast<double>(piece_body->GetPosition().x)*this->graphicsscale,
                this->scaled_tetris_field.y() + static_cast<double>(piece_body->GetPosition().y)*this->graphicsscale
                );
    painter->scale(this->graphicsscale, this->graphicsscale);
    painter->rotate(static_cast<double>(piece_body->GetAngle())*rad_to_deg);

    painter->drawPixmap(this->piece_rects.at(piece), this->piece_images.at(piece));

    painter->restore();
}

void NT3Game::makeNewTetrisPiece(){

    tetris_piece_enum type = static_cast<tetris_piece_enum>(this->rng.bounded(num_tetris_pieces));

    this->currentPiece = world->CreateBody(&this->tetrisBodyDef);
    //this->currentPiece->ApplyTorque(3000000, true);

    for (b2FixtureDef f : this->tetrisFixtures.at(type)){
        this->currentPiece->CreateFixture(&f);
    }
    this->contactlistener->currentPiece = this->currentPiece;
    this->bodytypes.insert(this->currentPiece, type);

    this->currentPiece->SetGravityScale(0);
    this->currentPiece->SetLinearVelocity(b2Vec2(0, 50));
}

void NT3Game::initializeTetrisPieceDefs(){

    this->tetrisBodyDef.type = b2_dynamicBody;
    this->tetrisBodyDef.allowSleep = true;
    this->tetrisBodyDef.awake = true;
    this->tetrisBodyDef.position.Set(static_cast<float32>(this->tetris_field.width()/2), -this->side_length*2);

    for (uint8 i = 0; i < num_tetris_pieces; i++){
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
    for (uint32 t = 0; t < this->tetrisShapes.size(); t++){
        this->tetrisFixtures.push_back(fixture_vector_template);
        for (uint32 s = 0; s < this->max_shapes_per_piece; s++){
            if (this->tetrisShapes.at(t).size() == s) break;
            this->tetrisFixtures.at(t).push_back(fixture_template);
            this->tetrisFixtures.at(t).at(s).shape = &this->tetrisShapes.at(t).at(s);
            this->tetrisFixtures.at(t).at(s).density = 10.0f;
            this->tetrisFixtures.at(t).at(s).friction = 0.3f;
        }
    }
}

void NT3Game::initializeTetrisPieceImages(){
    int side_length = static_cast<int>(this->side_length);
    for (uint8 piece = 0; piece < num_tetris_pieces; piece++){
        QString path = ":/resources/graphics/pieces/" + QString::number(piece) + ".png";
        this->piece_images.push_back(QPixmap(path));

        switch(piece){
        case I:
            this->piece_rects.push_back(QRect(-2*side_length, -side_length/2, 4*side_length, side_length));
            break;
        case O:
            this->piece_rects.push_back(QRect(-side_length, -side_length, 2*side_length, 2*side_length));
            break;
        case G:
        case L:
        case T:
            this->piece_rects.push_back(QRect(-3*side_length/2, -side_length/2, 3*side_length, 2*side_length));
            break;
        case Z:
        case S:
            this->piece_rects.push_back(QRect(-3*side_length/2, -side_length, 3*side_length, 2*side_length));
            break;
        default:
            fprintf(stderr, "Piece not defined: %u\n", piece);
            break;
        }
    }
}

void NT3Game::initializeWalls(){
    b2BodyDef edgeBodyDef;
    edgeBodyDef.position.Set(0, 0);

    b2EdgeShape edge;
    edge.Set(b2Vec2(0, tetris_field.height()), b2Vec2(tetris_field.width(), tetris_field.height()));

    this->walls[GROUND] = world->CreateBody(&edgeBodyDef);
    this->walls[GROUND]->CreateFixture(&edge, 0.0f);

    edge.Set(b2Vec2(0, 0), b2Vec2(0, tetris_field.height()));

    this->walls[LEFTWALL] = world->CreateBody(&edgeBodyDef);
    this->walls[LEFTWALL]->CreateFixture(&edge, 0.0f);

    edge.Set(b2Vec2(tetris_field.width(), 0), b2Vec2(tetris_field.width(), tetris_field.height()));

    this->walls[RIGHTWALL] = world->CreateBody(&edgeBodyDef);
    this->walls[RIGHTWALL]->CreateFixture(&edge, 0.0f);
}
