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
    this->setTitle("Not Tetris 3");

    QScreen* screen = this->screen();
    this->fps = screen->refreshRate();
    this->framerate = 1.0/this->fps;
    this->timeStep = this->framerate; //seconds
    this->expected_frame_time = static_cast<int>(ceil(this->framerate*this->millis_per_second));
    //printf("eft: %d\n", this->expected_frame_time);
    //printf("Using time step of %f ms\n", window.timeStep*window.millis_per_second);

    QRect screenRect = screen->availableGeometry();
    int screen_width = screenRect.width();
    int screen_height = screenRect.height();

    if (screen_width*1.0/screen_height > this->aspect_ratio){ //screen is relatively wider than the app
        int window_width = static_cast<int>(screen_height*this->aspect_ratio);
        this->setGeometry((screen_width - window_width)/2, 0, window_width, screen_height);
    } else { //screen is relatively taller than app, or it's the same ratio
        int window_height = static_cast<int>(screen_width*1.0/this->aspect_ratio);
        this->setGeometry(0, (screen_height - window_height)/2, screen_width, window_height);
    }

    if (this->gamebackground.isNull()){
        fprintf(stderr, "Resources not present, exiting...\n");
        this->close();
    }

    //key-action mappings
    this->accelDownKey = Qt::Key_Down;
    this->rotateStateTable.insert(Qt::Key_Z, ROTATECCW);
    this->rotateStateTable.insert(Qt::Key_X, ROTATECW);
    this->lateralMovementStateTable.insert(Qt::Key_Left, MOVELEFT);
    this->lateralMovementStateTable.insert(Qt::Key_Right, MOVERIGHT);

    for (uint r = 0; r < this->tetris_rows; r++){
        this->row_densities.push_back(0.0f);
    }

    b2Vec2 gravity(0.0f, this->gravity_g);
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

    double ar_error = width*1.0/height - aspect_ratio;
    bool aspect_ratio_respected = qAbs(ar_error) < this->aspect_ratio_epsilon;

    if (ar_error > 0){ //screen is relatively wider than the app
        this->graphicsscale = height*1.0/this->ui_field.height();
    } else if (ar_error < 0){ //screen is relatively skinnier than app
        this->graphicsscale = width*1.0/this->ui_field.width();
    }

    this->graphicsscale = qMax(this->min_graphics_scale, this->graphicsscale);
    //printf("Graphics scale is now %f\n", this->graphicsscale);

    this->scaled_ui_field.setX(static_cast<int>(this->ui_field.x()*this->graphicsscale));
    this->scaled_ui_field.setY(static_cast<int>(this->ui_field.y()*this->graphicsscale));
    this->scaled_ui_field.setWidth(static_cast<int>(this->ui_field.width()*this->graphicsscale));
    this->scaled_ui_field.setHeight(static_cast<int>(this->ui_field.height()*this->graphicsscale));

    this->scaled_tetris_field.setX(static_cast<int>(this->tetris_field.x()*this->graphicsscale));
    this->scaled_tetris_field.setY(static_cast<int>(this->tetris_field.y()*this->graphicsscale));
    this->scaled_tetris_field.setWidth(static_cast<int>(this->tetris_field.width()*this->graphicsscale));
    this->scaled_tetris_field.setHeight(static_cast<int>(this->tetris_field.height()*this->graphicsscale));

    if (!aspect_ratio_respected){
        this->resize(this->scaled_ui_field.width(), this->scaled_ui_field.height());
    }
}

void NT3Game::keyPressEvent(QKeyEvent* ev){
    //printf("Key pressed: %s\n", ev->text().toUtf8().constData());
    //fflush(stdout);

    int key = ev->key();

    if (this->lateralMovementStateTable.contains(key)){
        lateral_movement_state_enum requested_direction = this->lateralMovementStateTable.value(key);
        lateral_movement_state_enum other_direction = requested_direction == MOVERIGHT ? MOVELEFT : MOVERIGHT;

        switch(this->lateralMovementState){
        case NO_LATERAL_MOVEMENT:
            this->lateralMovementState = requested_direction;
            break;
        case BOTH_DIRECTIONS:
            //do nothing
            break;
        case MOVELEFT:
        case MOVERIGHT:
            if (this->lateralMovementState == other_direction){
                this->lateralMovementState = BOTH_DIRECTIONS;
            }
            break;
        default:
            fprintf(stderr, "Invalid Lateral Movement state\n");
            break;
        }
    } else if (this->rotateStateTable.contains(key)){
        rotate_state_enum requested_rotation = this->rotateStateTable.value(key);
        rotate_state_enum other_rotation = requested_rotation == ROTATECW ? ROTATECCW : ROTATECW;

        switch(this->rotateState){
        case NO_ROTATION:
            this->rotateState = requested_rotation;
            break;
        case BOTH_ROTATIONS:
            //do nothing
            break;
        case ROTATECW:
        case ROTATECCW:
            if (this->rotateState == other_rotation){
                this->rotateState = BOTH_ROTATIONS;
            }
            break;
        default:
            fprintf(stderr, "Invalid Rotation state\n");
            break;
        }
    } else if (key == this->accelDownKey){
        this->accelDownState = true;
    }
}

void NT3Game::keyReleaseEvent(QKeyEvent* ev){
    //printf("Key released: %s\n", ev->text().toUtf8().constData());

    int key = ev->key();

    if (this->lateralMovementStateTable.contains(key)){
        lateral_movement_state_enum unrequested_direction = this->lateralMovementStateTable.value(key);
        lateral_movement_state_enum other_direction = unrequested_direction == MOVERIGHT ? MOVELEFT : MOVERIGHT;

        switch(this->lateralMovementState){
        case NO_LATERAL_MOVEMENT:
            //do nothing
            break;
        case BOTH_DIRECTIONS:
            this->lateralMovementState = other_direction;
            break;
        case MOVELEFT:
        case MOVERIGHT:
            if (this->lateralMovementState == unrequested_direction){
                this->lateralMovementState = NO_LATERAL_MOVEMENT;
            }
            break;
        default:
            fprintf(stderr, "Invalid Lateral Movement state\n");
            break;
        }
    } else if (this->rotateStateTable.contains(key)){
        rotate_state_enum unrequested_rotation = this->rotateStateTable.value(key);
        rotate_state_enum other_rotation = unrequested_rotation == ROTATECW ? ROTATECCW : ROTATECW;

        switch(this->rotateState){
        case NO_ROTATION:
            //do nothing
            break;
        case BOTH_ROTATIONS:
            this->rotateState = other_rotation;
            break;
        case ROTATECW:
        case ROTATECCW:
            if (this->rotateState == unrequested_rotation){
                this->rotateState = NO_ROTATION;
            }
            break;
        default:
            fprintf(stderr, "Invalid Rotation state\n");
            break;
        }
    } else if (key == this->accelDownKey){
        this->accelDownState = false;
    }
}

void NT3Game::render(QPainter& painter)
{
    painter.setRenderHint(QPainter::Antialiasing);

    painter.drawPixmap(0, 0, this->scaled_ui_field.width(), this->scaled_ui_field.height(), this->gamebackground);

    painter.setPen(Qt::SolidLine);
    painter.setPen(this->debug_line_color);
    painter.setBrush(Qt::NoBrush);

    for (b2Body* b = this->world->GetBodyList(); b; b = b->GetNext()){
        if (!this->isAWall(b)){
            //printf("Body: (%f, %f)\n", b->GetPosition().x, b->GetPosition().y);
            this->drawTetrisPiece(&painter, b);
        }
        //this->drawBodyTo(&painter, b);
    }

    painter.setPen(Qt::NoPen);

    for (uint r = 0; r < this->tetris_rows; r++){
        double height = static_cast<double>(this->side_length)*this->graphicsscale;
        double top = height*r;

        double fill_fraction = static_cast<double>(this->row_densities.at(r)/this->line_clear_threshold);
        //if (fill_fraction > 1.0) printf("r = %u, ff = %f\n", r, fill_fraction);
        fill_fraction = qMin(fill_fraction, 1.0);
        double width = fill_fraction*this->row_fill_density_col_width*this->graphicsscale;

        QRectF status_box(0, top, width, height);

        int grey = static_cast<int>((1-fill_fraction)*255);
        painter.setBrush(QColor(grey, grey, grey));

        painter.drawRect(status_box);
    }
}

bool NT3Game::isAWall(b2Body* b){
    for (uint i = 0; i < num_walls; i++){
        if (b == this->walls[i]){
            return true;
        }
    }
    return false;
}


void NT3Game::doGameStep(){
    world->Step(static_cast<float32>(this->timeStep), this->velocityIterations, this->positionIterations);

    if (this->contactlistener->hasCurrentPieceCollided()){
        this->currentPiece->SetGravityScale(1);

        if (this->currentPiece->GetWorldCenter().y < 0){
            printf("Game lost!\n");
            this->close();
        }

        this->makeNewTetrisPiece();
    }

    switch(this->rotateState){
    case NO_ROTATION:
    case BOTH_ROTATIONS:
        //do nothing
        //printf("Dont rotate\n");
        break;
    case ROTATECW:
        //printf("Rotate CW\n");
        if (this->currentPiece->GetAngularVelocity() < this->wmax){
            this->currentPiece->ApplyTorque(this->torque, true);
        }
        break;
    case ROTATECCW:
        //printf("Rotate CCW\n");
        if (this->currentPiece->GetAngularVelocity() > -this->wmax){
            this->currentPiece->ApplyTorque(-this->torque, true);
        }
        break;
    default:
        fprintf(stderr, "Invalid Rotation state\n");
        break;
    }

    b2Vec2 linear_force_vect = b2Vec2(0, 0);

    switch(this->lateralMovementState){
    case NO_LATERAL_MOVEMENT:
    case BOTH_DIRECTIONS:
        //do nothing
        break;
    case MOVELEFT:
        linear_force_vect.x = -this->lateral_force;
        break;
    case MOVERIGHT:
        linear_force_vect.x = this->lateral_force;
        break;
    default:
        fprintf(stderr, "Invalid Lateral Movement state\n");
        break;
    }

    if (this->accelDownState || this->currentPiece->GetLinearVelocity().y < this->downward_velocity_regular){
        linear_force_vect.y = this->downward_force;
    } else {
        linear_force_vect.y = -this->upward_correcting_force;
    }
    this->currentPiece->ApplyForce(linear_force_vect, this->currentPiece->GetWorldCenter(), true);

    for (uint r = 0; r < this->tetris_rows; r++){
        this->row_densities.at(r) = this->getRowDensity(r);
    }
    //TODO: check against threshold, clear line if over

    //this->checkRowDensity(this->tetris_rows-1);
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
                    static_cast<double>(shape.m_vertices[0].y) //I HATE this indentation. Why, Qt?
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

    for (b2FixtureDef f : this->tetrisFixtures.at(type)){
        this->currentPiece->CreateFixture(&f);
    }
    this->contactlistener->currentPiece = this->currentPiece;
    this->bodytypes.insert(this->currentPiece, type);

    this->currentPiece->SetGravityScale(0);
    this->currentPiece->SetLinearVelocity(b2Vec2(0, this->downward_velocity_regular));
    this->currentPiece->SetAngularVelocity(0);
    this->currentPiece->SetLinearDamping(0);
}

float32 NT3Game::getRowDensity(uint row){
    float32 bot = row*this->side_length;
    float32 top = (row+1)*this->side_length;
    //printf("Checking for shapes between %f and %f\n", bot, top);

    float32 total_area = 0;

    for (b2Body* b = this->world->GetBodyList(); b; b = b->GetNext()){
        if (this->isAWall(b)) continue;

        for (b2Fixture* f = b->GetFixtureList(); f; f = f->GetNext()){
            Q_ASSERT(f->GetShape()->GetType() == b2Shape::e_polygon);
            b2PolygonShape* s = static_cast<b2PolygonShape*>(f->GetShape());
            //printf("Shape coords are: (%f, %f)\n", s->m_vertices[0].x, s->m_vertices[0].y);

            b2Transform t;
            t.Set(b->GetPosition(), b->GetAngle());

            std::vector<b2RayCastOutput> RC_outputs;
            std::vector<b2RayCastInput> RC_inputs;
            std::vector<bool> RC_hits;
            for (uint8 r = 0; r < num_ray_casts; r++){ //TODO: initialize all line clear ray casts at program start
                b2RayCastOutput output;
                RC_outputs.push_back(output);
                b2RayCastInput input;
                RC_inputs.push_back(input);
                switch(r){
                case TOPLEFT:
                    RC_inputs.at(r).p1.Set(0, top);
                    RC_inputs.at(r).p2.Set(this->tetris_field.width(), top);
                    RC_inputs.at(r).maxFraction = 1;
                    break;
                case TOPRIGHT:
                    RC_inputs.at(r).p1.Set(this->tetris_field.width(), top);
                    RC_inputs.at(r).p2.Set(0, top);
                    RC_inputs.at(r).maxFraction = 1;
                    break;
                case BOTTOMLEFT:
                    RC_inputs.at(r).p1.Set(0, bot);
                    RC_inputs.at(r).p2.Set(this->tetris_field.width(), bot);
                    RC_inputs.at(r).maxFraction = 1;
                    break;
                case BOTTOMRIGHT:
                    RC_inputs.at(r).p1.Set(this->tetris_field.width(), bot);
                    RC_inputs.at(r).p2.Set(0, bot);
                    RC_inputs.at(r).maxFraction = 1;
                    break;
                default:
                    fprintf(stderr, "Ray cast enum not defined\n");
                    break;
                } //end ray cast switch statement
                RC_hits.push_back(s->RayCast(&RC_outputs.at(r), RC_inputs.at(r), t, 0));
            } //end ray cast init

            if(RC_hits.at(TOPLEFT) || RC_hits.at(BOTTOMLEFT)){

                std::vector<b2Vec2> new_points;
                for (int i = 0; i < s->m_count; i++){

                    b2Vec2 p = b->GetWorldPoint(s->m_vertices[i]);
                    if (p.y <= top && p.y >= bot){
                        new_points.push_back(p);
                    }
                }

                if (RC_hits.at(TOPLEFT)){
                    new_points.push_back(
                                RC_inputs.at(TOPLEFT).p1 + //TODO: encapsulate this formula
                                RC_outputs.at(TOPLEFT).fraction * (RC_inputs.at(TOPLEFT).p2 - RC_inputs.at(TOPLEFT).p1)
                                );
                    new_points.push_back(
                                RC_inputs.at(TOPRIGHT).p1 +
                                RC_outputs.at(TOPRIGHT).fraction * (RC_inputs.at(TOPRIGHT).p2 - RC_inputs.at(TOPRIGHT).p1)
                                );
                }

                if (RC_hits.at(BOTTOMLEFT)){
                    new_points.push_back(
                                RC_inputs.at(BOTTOMLEFT).p1 +
                                RC_outputs.at(BOTTOMLEFT).fraction * (RC_inputs.at(BOTTOMLEFT).p2 - RC_inputs.at(BOTTOMLEFT).p1)
                                );
                    new_points.push_back(
                                RC_inputs.at(BOTTOMRIGHT).p1 +
                                RC_outputs.at(BOTTOMRIGHT).fraction * (RC_inputs.at(BOTTOMRIGHT).p2 - RC_inputs.at(BOTTOMRIGHT).p1)
                                );
                }

                int num_vertices = qMin(static_cast<int>(new_points.size()), b2_maxPolygonVertices);
                float32 area = this->poly_area(&new_points[0], num_vertices);
                total_area += area;

            } else { //If NEITHER of the ray casts hit
                b2Vec2 p = b->GetWorldPoint(s->m_vertices[0]);
                if (p.y > top && p.y < bot){
                    float32 area = this->poly_area(s->m_vertices, s->m_count);
                    total_area += area;
                }
            } //end neither ray cast hit
        } //end fixture loop
    } //end body loop

    //Q_ASSERT(total_area/this->side_length <= this->tetris_field.width());

    return total_area;
}

//This function is code modified directly from b2PolygonShape::Set() and b2PolygonShape::ComputeCentroid()
//so that it returns 0 on error whereas the original function fails an assert, crashing the program.
float32 NT3Game::poly_area(b2Vec2* vertices, int count){

    if(3 > count && count > b2_maxPolygonVertices){
        //printf("Polygon count is out of range: %d\n", count);
        return 0;
    }

    int32 n = b2Min(count, b2_maxPolygonVertices);

    // Perform welding and copy vertices into local buffer.
    b2Vec2 ps[b2_maxPolygonVertices];
    int32 tempCount = 0;
    for (int32 i = 0; i < n; ++i){
        b2Vec2 v = vertices[i];

        bool unique = true;
        for (int32 j = 0; j < tempCount; ++j){
            if (b2DistanceSquared(v, ps[j]) < ((0.5f * b2_linearSlop) * (0.5f * b2_linearSlop))){
                unique = false;
                break;
            }
        }

        if (unique){
            ps[tempCount++] = v;
        }
    }

    n = tempCount;
    if (n < 3){
        //printf("Polygon is degenerate (1st check).\n");
        return 0;
    }

    // Create the convex hull using the Gift wrapping algorithm
    // http://en.wikipedia.org/wiki/Gift_wrapping_algorithm

    // Find the right most point on the hull
    int32 i0 = 0;
    float32 x0 = ps[0].x;
    for (int32 i = 1; i < n; ++i){
        float32 x = ps[i].x;
        if (x > x0 || (x == x0 && ps[i].y < ps[i0].y)){
            i0 = i;
            x0 = x;
        }
    }

    int32 hull[b2_maxPolygonVertices];
    int32 m = 0;
    int32 ih = i0;

    for (;;){
        if (m >= b2_maxPolygonVertices){
            //printf("m >= %d\n", b2_maxPolygonVertices);
            return 0;
        }
        hull[m] = ih;

        int32 ie = 0;
        for (int32 j = 1; j < n; ++j){
            if (ie == ih){
                ie = j;
                continue;
            }

            b2Vec2 r = ps[ie] - ps[hull[m]];
            b2Vec2 v = ps[j] - ps[hull[m]];
            float32 c = b2Cross(r, v);
            if (c < 0.0f){
                ie = j;
            }

            // Collinearity check
            if (c == 0.0f && v.LengthSquared() > r.LengthSquared()){
                ie = j;
            }
        }

        ++m;
        ih = ie;

        if (ie == i0){
            break;
        }
    }

    if (m < 3){
        //printf("Polygon is degenerate (2nd check).\n");
        return 0 ;
    }

    // Copy vertices.
    b2Vec2 m_vertices[b2_maxPolygonVertices];
    for (int32 i = 0; i < m; ++i){
        m_vertices[i] = ps[hull[i]];
    }

    // Compute normals. Ensure the edges have non-zero length.
    for (int32 i = 0; i < m; ++i){
        int32 i1 = i;
        int32 i2 = i + 1 < m ? i + 1 : 0;
        b2Vec2 edge = m_vertices[i2] - m_vertices[i1];
        b2Assert(edge.LengthSquared() > b2_epsilon * b2_epsilon);
    }

    count = m;

    float32 area = 0.0f;

    // pRef is the reference point for forming triangles.
    // It's location doesn't change the result (except for rounding error).
    b2Vec2 pRef(0.0f, 0.0f);

    for (int32 i = 0; i < count; ++i){
        // Triangle vertices.
        b2Vec2 p1 = pRef;
        b2Vec2 p2 = m_vertices[i];
        b2Vec2 p3 = i + 1 < count ? m_vertices[i+1] : m_vertices[0];

        b2Vec2 e1 = p2 - p1;
        b2Vec2 e2 = p3 - p1;

        float32 D = b2Cross(e1, e2);

        float32 triangleArea = 0.5f * D;
        area += triangleArea;
    }

    // Centroid
    if (area > b2_epsilon){
        return area;
    }
    //printf("area <= %f\n", b2_epsilon);
    return 0;
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
            this->tetrisFixtures.at(t).at(s).density = this->density;
            this->tetrisFixtures.at(t).at(s).friction = this->piece_friction_k;
            this->tetrisFixtures.at(t).at(s).restitution = this->restitution;
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
    this->walls[GROUND]->CreateFixture(&edge, 0.0f)->SetFriction(this->ground_friction_k);

    edge.Set(b2Vec2(0, 0), b2Vec2(0, tetris_field.height()));

    this->walls[LEFTWALL] = world->CreateBody(&edgeBodyDef);
    this->walls[LEFTWALL]->CreateFixture(&edge, 0.0f);
    this->walls[LEFTWALL]->GetFixtureList()->SetFriction(0);

    edge.Set(b2Vec2(tetris_field.width(), 0), b2Vec2(tetris_field.width(), tetris_field.height()));

    this->walls[RIGHTWALL] = world->CreateBody(&edgeBodyDef);
    this->walls[RIGHTWALL]->CreateFixture(&edge, 0.0f);
    this->walls[RIGHTWALL]->GetFixtureList()->SetFriction(0);
}
