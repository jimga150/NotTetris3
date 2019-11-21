#include "nt3game.h"

NT3Game::NT3Game()
{
    this->setTitle("Not Tetris 3");

    QScreen* screen = this->screen();
    this->fps = screen->refreshRate();
    this->framerate = 1.0/this->fps;
    this->timeStep = static_cast<float32>(this->framerate); //seconds
    this->expected_frame_time = static_cast<int>(ceil(this->framerate*this->millis_per_second));
    //printf("eft: %d\n", this->expected_frame_time);
    //printf("Using time step of %f ms\n", window.timeStep*window.millis_per_second);

    QRect screenRect = screen->availableGeometry();
    int screen_width = screenRect.width();
    int screen_height = screenRect.height();

    if (screen_width*1.0/screen_height > this->aspect_ratio){ //screen is relatively wider than the app

        this->max_graphics_scale = this->mgs_factor*(screen_height*1.0/this->ui_field.height());

        int window_width = static_cast<int>(screen_height*this->aspect_ratio);
        this->setGeometry((screen_width - window_width)/2, 0, window_width, screen_height);

    } else { //screen is relatively taller than app, or it's the same ratio

        this->max_graphics_scale = this->mgs_factor*(screen_width*1.0/this->ui_field.width());

        int window_height = static_cast<int>(screen_width*1.0/this->aspect_ratio);
        this->setGeometry(0, (screen_height - window_height)/2, screen_width, window_height);
    }

    if (this->gamebackground.isNull()){
        fprintf(stderr, "Resources not present, exiting...\n");
        this->close();
    }

    //key-action mappings
    this->freeze_key = Qt::Key_Space;
    this->accelDownKey = Qt::Key_Down;
    this->rotateStateTable.insert(Qt::Key_Z, ROTATECCW);
    this->rotateStateTable.insert(Qt::Key_X, ROTATECW);
    this->lateralMovementStateTable.insert(Qt::Key_Left, MOVELEFT);
    this->lateralMovementStateTable.insert(Qt::Key_Right, MOVERIGHT);

    this->init_BDC();

    b2Vec2 gravity(0.0f, this->gravity_g);
    this->world = new b2World(gravity);
    this->world->SetAllowSleeping(true);
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
    if (this->world){
        for (b2Body* b = this->world->GetBodyList(); b; b = b->GetNext()){
            this->freeUserDataOn(b);
        }
        delete world;
    }
    if (this->contactlistener) delete contactlistener;
}

void NT3Game::freeUserDataOn(b2Body* b){
    tetrisPieceData* data = this->getTetrisPieceData(b);
    if (data) delete data;
    b->SetUserData(nullptr);
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

void NT3Game::render(QPainter& painter)
{
    if (this->freeze_frame){
        painter.drawPixmap(this->scaled_ui_field, this->saved_frames[this->last_frame]);
        return;
    }

    painter.setRenderHint(QPainter::Antialiasing);

    painter.drawPixmap(this->scaled_ui_field, this->gamebackground);

    painter.setPen(Qt::SolidLine);
    painter.setPen(this->debug_line_color);
    painter.setBrush(Qt::NoBrush);

    //printf("New frame:\n");
    for (b2Body* b = this->world->GetBodyList(); b; b = b->GetNext()){
        if (!this->isAWall(b) /*&& b->IsAwake()*/){
            //printf("Body: (%f, %f)\n", b->GetPosition().x, b->GetPosition().y);
            this->drawTetrisPiece(&painter, b);
        }
        if (debug_box2d){
            this->drawBodyTo(&painter, b);
        }
    }

    painter.setPen(Qt::NoPen);

    for (uint r = 0; r < this->tetris_rows; r++){
        double height = static_cast<double>(this->side_length)*this->graphicsscale;
        double top = height*r;

        double fill_fraction = static_cast<double>(this->row_densities.at(r)/this->line_clear_threshold);
        //if (fill_fraction > 1.0) printf("r = %u, ff = %f\n", r, fill_fraction);
        fill_fraction = qMin(fill_fraction, 1.0);
        double width = fill_fraction*this->row_fill_density_col_width*this->graphicsscale;

        int grey = static_cast<int>((1-fill_fraction)*255);
        painter.setBrush(QColor(grey, grey, grey));

        painter.drawRect(QRectF(0, top, width, height));
    }

#ifdef TIME_FRAME_COMPS
    if (this->debug_framerate){
        painter.setPen(this->debug_line_color);
        painter.drawText(QPointF(3*this->graphicsscale, 12*this->graphicsscale), QString::number(this->frame_times.render_time));
        painter.drawText(QPointF(3*this->graphicsscale, 14*this->graphicsscale), QString::number(this->frame_times.game_frame_time));
    }
#endif
}

void NT3Game::drawBodyTo(QPainter* painter, b2Body* body){

    painter->save();
    painter->translate(
                this->scaled_tetris_field.x() + static_cast<double>(body->GetPosition().x)*this->graphicsscale,
                this->scaled_tetris_field.y() + static_cast<double>(body->GetPosition().y)*this->graphicsscale
                );

    QString ptrStr = QString("0x%1").arg(reinterpret_cast<quintptr>(body),QT_POINTER_SIZE * 2, 16, QChar('0'));
    //https://stackoverflow.com/questions/8881923/how-to-convert-a-pointer-value-to-qstring
    painter->drawText(QPoint(0, 0), ptrStr);
    //printf("\t%s: (%f, %f)\n", ptrStr.toUtf8().constData(), body->GetPosition().x, body->GetPosition().y);

    painter->rotate(static_cast<double>(body->GetAngle())*rad_to_deg);

    for (b2Fixture* f = body->GetFixtureList(); f; f = f->GetNext()){
        switch(f->GetType()){
        case b2Shape::e_polygon:{
            b2PolygonShape shape = *static_cast<b2PolygonShape*>(f->GetShape());
            int numpoints = shape.m_count;
            vector<QPointF> points;
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

    painter->save();

    painter->translate(
                this->scaled_tetris_field.x() + static_cast<double>(piece_body->GetPosition().x)*this->graphicsscale,
                this->scaled_tetris_field.y() + static_cast<double>(piece_body->GetPosition().y)*this->graphicsscale
                );
    painter->scale(this->graphicsscale, this->graphicsscale);
    painter->rotate(static_cast<double>(piece_body->GetAngle())*rad_to_deg);

    tetrisPieceData* body_data = this->getTetrisPieceData(piece_body);
    if (body_data == nullptr){
        painter->drawPixmap(this->default_piece_rect, this->default_piece_image);
    } else {
        painter->drawPixmap(body_data->region, body_data->image);
    }

    painter->restore();
}


void NT3Game::keyPressEvent(QKeyEvent* ev){
    //printf("Key pressed: %s\n", ev->text().toUtf8().constData());
    //fflush(stdout);

    int key = ev->key();

    if (this->save_frames && this->freeze_frame){
        if (key == Qt::Key_Left){ //previous frame
            this->last_frame--;
            if (this->last_frame < 0) this->last_frame = NUM_FRAMES_TO_SAVE - 1;
        } else if (key == Qt::Key_Right){
            this->last_frame = (this->last_frame + 1) % NUM_FRAMES_TO_SAVE;
        }
    }

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
    } else if (this->save_frames && key == this->freeze_key){

        this->freeze_frame = !this->freeze_frame;

        this->lateralMovementState = NO_LATERAL_MOVEMENT;
        this->rotateState = NO_ROTATION;
        this->accelDownState = false;
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

void NT3Game::doGameStep(){

    if (this->freeze_frame){
        return;
    }
#ifdef TIME_GAME_FRAME
    QElapsedTimer timer;
    timer.start();
#endif

    if (save_frames){
        this->last_frame = (this->last_frame + 1) % NUM_FRAMES_TO_SAVE;
        this->saved_frames[this->last_frame] = QPixmap(this->scaled_ui_field.size());
        QPainter sf_painter(&this->saved_frames[this->last_frame]);
        this->render(sf_painter);
        sf_painter.end();
#ifdef TIME_GAME_FRAME
        printf("Re-render: %lld ms,\t", timer.elapsed());
#endif
    }

    /*if (this->row_cleared){
        this->freeze_frame = true;
    }*/

#ifdef TIME_GAME_FRAME
    timer.restart();
    world->Step(this->timeStep, this->velocityIterations, this->positionIterations);
    printf("World step: %lld ms,\t", timer.elapsed());
    timer.restart();
#endif

    bool touchdown = false;
    if (this->contactlistener->hasCurrentPieceCollided()){
        touchdown = true;
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

    if (this->row_cleared){
        this->init_BDC();
    }

#ifdef TIME_GAME_FRAME
    printf("Currpiece math: %lld ms\t", timer.elapsed());
    timer.restart();
#endif

    for (uint r = 0; r < this->tetris_rows; r++){
        this->row_densities.at(r) = this->getRowDensity(r);
    }

#ifdef TIME_GAME_FRAME
    printf("Row density: %lld ms\t", timer.elapsed());
    timer.restart();
#endif

    this->row_cleared = false;
    if (touchdown){
        for (uint r = 0; r < this->tetris_rows; r++){
            if (this->row_densities.at(r) > this->line_clear_threshold){
                //printf("Clearing row %u\n", r);
                this->clearRow(r);
                this->row_cleared = true;
            }
        }
    }
#ifdef TIME_GAME_FRAME
    printf("Row clears: %lld ms\n", timer.elapsed());
#endif
}


float32 NT3Game::getRowDensity(uint row){
    float32 bot = row*this->side_length;
    float32 top = (row+1)*this->side_length;

    vector<rayCastComplete> ray_casts = this->getRayCasts(top, bot);

    float32 total_area = 0;

    for (b2Body* b = this->world->GetBodyList(); b; b = b->GetNext()){
        if (this->isAWall(b)) continue;
        if (b == this->currentPiece) continue;

        if (this->body_density_contributions.at(row).contains(b)){
            if (!b->IsAwake()){
                total_area += this->body_density_contributions.at(row).value(b);
                continue;
            }
            this->body_density_contributions.at(row).remove(b);
        }

        float32 body_area = 0;
        for (b2Fixture* f = b->GetFixtureList(); f; f = f->GetNext()){
            Q_ASSERT(f->GetShape()->GetType() == b2Shape::e_polygon);
            b2PolygonShape* s = static_cast<b2PolygonShape*>(f->GetShape());

            for (uint8 r = 0; r < num_ray_casts; r++){
                ray_casts.at(r).doRayCast(s, b);
            }

            if(ray_casts.at(TOPLEFT).hit || ray_casts.at(BOTTOMLEFT).hit){
                vector<b2Vec2> new_points;

                for (int i = 0; i < s->m_count; i++){

                    b2Vec2 p = b->GetWorldPoint(s->m_vertices[i]);
                    if (p.y <= top && p.y >= bot){
                        new_points.push_back(p);
                    }

                }

                if (ray_casts.at(TOPLEFT).hit){
                    b2Vec2 topleft_hit = this->hit_point(ray_casts.at(TOPLEFT));
                    new_points.push_back(topleft_hit);

                    if (ray_casts.at(TOPRIGHT).hit){
                        b2Vec2 topright_hit = this->hit_point(ray_casts.at(TOPRIGHT));
                        new_points.push_back(topright_hit);
                    }
                }

                if (ray_casts.at(BOTTOMLEFT).hit){
                    b2Vec2 bottomleft_hit = this->hit_point(ray_casts.at(BOTTOMLEFT));
                    new_points.push_back(bottomleft_hit);

                    if (ray_casts.at(BOTTOMRIGHT).hit){
                        b2Vec2 bottomright_hit = this->hit_point(ray_casts.at(BOTTOMRIGHT));
                        new_points.push_back(bottomright_hit);
                    }
                }

                int num_vertices = qMin(static_cast<int>(new_points.size()), b2_maxPolygonVertices);
                float32 area = this->poly_area(&new_points[0], num_vertices);
                body_area += area;

            } else { //If NEITHER of the ray casts hit

                b2Vec2 p = b->GetWorldPoint(s->m_vertices[0]);
                if (p.y <= top && p.y >= bot){
                    float32 area = this->poly_area(s->m_vertices, s->m_count);
                    body_area += area;
                }

            } //end neither ray cast hit

        } //end fixture loop

        this->body_density_contributions.at(row).insert(b, body_area);
        total_area += body_area;

    } //end body loop

    //Q_ASSERT(total_area/this->side_length <= this->tetris_field.width());
    return total_area;
}

void NT3Game::clearRow(uint row){

    vector<float32> sides;
    for (uint i = 0; i < num_line_cut_sides; i++){
        switch(i){
        case TOP:
            sides.push_back((row+1)*this->side_length + numeric_limits<float32>::epsilon());
            break;
        case BOTTOM:
            sides.push_back(row*this->side_length - numeric_limits<float32>::epsilon());
            break;
        default:
            fprintf(stderr, "Line clear side %u not defined\n", i);
            break;
        }
    }

    vector<rayCastComplete> ray_casts = this->getRayCasts(sides.at(TOP), sides.at(BOTTOM));

    //make list of bodies affected by this row clear
    //for top and bottom lines:
    vector<b2Body*> affected_bodies;

    //find all bodies with shapes that cross the line
    for (b2Body* b = this->world->GetBodyList(); b; b = b->GetNext()){
        if (this->isAWall(b)) continue;

        bool affected = false;
        for (b2Fixture* f = b->GetFixtureList(); f; f = f->GetNext()){
            Q_ASSERT(f->GetShape()->GetType() == b2Shape::e_polygon);
            b2PolygonShape* s = static_cast<b2PolygonShape*>(f->GetShape());

            float32 v0_worldpoint_y = b->GetWorldPoint(s->m_vertices[0]).y;
            if (v0_worldpoint_y >= sides.at(BOTTOM) && v0_worldpoint_y <= sides.at(TOP)){
                affected = true;
                break;
            }

            ray_casts.at(TOPLEFT).doRayCast(s, b);
            ray_casts.at(BOTTOMLEFT).doRayCast(s, b);
            if (ray_casts.at(TOPLEFT).hit || ray_casts.at(BOTTOMLEFT).hit){
                affected = true;
                break;
            }

        }

        //for all of the affected bodies:
        if (!affected) continue;

        //printf("%p is affected by row clear\n", reinterpret_cast<void*>(b));
        affected_bodies.push_back(b);

        //for shapes in those bodies:
        vector<b2Fixture*> fixtures_to_destroy;
        vector<b2PolygonShape> shapes_to_make;
        for (b2Fixture* f = b->GetFixtureList(); f; f = f->GetNext()){
            //printf("New Fixture\n");
            Q_ASSERT(f->GetShape()->GetType() == b2Shape::e_polygon);
            b2PolygonShape* s = static_cast<b2PolygonShape*>(f->GetShape());

            for (uint side = 0; side < num_line_cut_sides; side++){
                /*switch(side){
                case TOP:
                    printf("Checking Top side...\n");
                    break;
                case BOTTOM:
                    printf("Checking Bottom side...\n");
                    break;
                default:
                    fprintf(stderr, "Side not defined! %u\n", side);
                    break;
                }*/

                //get list of points on outside of row clear
                vector<b2Vec2> new_points;
                for (int i = 0; i < s->m_count; i++){
                    b2Vec2 p = b->GetWorldPoint(s->m_vertices[i]);
                    bool outside;
                    switch(side){
                    case TOP:
                        outside = p.y > sides.at(TOP);
                        break;
                    case BOTTOM:
                        outside = p.y < sides.at(BOTTOM);
                        break;
                    default:
                        outside = false;
                        fprintf(stderr, "Line clear side %u not defined\n", side);
                        break;
                    }

                    if (outside){
                        //printf("%s is outside the line clear\n", this->b2Vec2String(p).toUtf8().constData());
                        new_points.push_back(s->m_vertices[i]);
                    }
                }

                //combine with list of points where shape hits line (must convert to local points)
                b2Vec2 hit_worldpoint;
                switch(side){
                case TOP:
                    ray_casts.at(TOPLEFT).doRayCast(s, b);
                    if (!ray_casts.at(TOPLEFT).hit) break;

                    hit_worldpoint = this->hit_point(ray_casts.at(TOPLEFT));
                    //printf("Added %s to point list for this shape\n", this->b2Vec2String(hit_worldpoint).toUtf8().constData());
                    new_points.push_back(b->GetLocalPoint(hit_worldpoint));

                    ray_casts.at(TOPRIGHT).doRayCast(s, b);
                    if (ray_casts.at(TOPRIGHT).hit){
                        hit_worldpoint = this->hit_point(ray_casts.at(TOPRIGHT));
                        //printf("Added %s to point list for this shape\n", this->b2Vec2String(hit_worldpoint).toUtf8().constData());
                        new_points.push_back(b->GetLocalPoint(hit_worldpoint));
                    }
                    break;
                case BOTTOM:
                    ray_casts.at(BOTTOMLEFT).doRayCast(s, b);
                    if (!ray_casts.at(BOTTOMLEFT).hit) break;

                    hit_worldpoint = this->hit_point(ray_casts.at(BOTTOMLEFT));
                    //printf("Added %s to point list for this shape\n", this->b2Vec2String(hit_worldpoint).toUtf8().constData());
                    new_points.push_back(b->GetLocalPoint(hit_worldpoint));

                    ray_casts.at(BOTTOMRIGHT).doRayCast(s, b);
                    if(ray_casts.at(BOTTOMRIGHT).hit){
                        hit_worldpoint = this->hit_point(ray_casts.at(BOTTOMRIGHT));
                        //printf("Added %s to point list for this shape\n", this->b2Vec2String(hit_worldpoint).toUtf8().constData());
                        new_points.push_back(b->GetLocalPoint(hit_worldpoint));
                    }
                    break;
                default:
                    fprintf(stderr, "Line clear side %u not defined\n", side);
                    break;
                }

                //validate points: if invalid, continue to next shape. this shape is just getting destroyed.
                int new_count = qMin(static_cast<int>(new_points.size()), b2_maxPolygonVertices);
                //printf("Trimming points: %ld --> %d\n", new_points.size(), new_count);
                if (!(this->poly_area(&new_points[0], new_count) > 0)){
                    //printf("Portion of shape outside line cut was too small, discarding\n");
                    continue;
                }

                //printf("Portion of shape outside line cut IS valid\n");

                //if they ARE valid:
                //make new shape with new points
                b2PolygonShape new_shape;
                new_shape.Set(&new_points[0], new_count);
                shapes_to_make.push_back(new_shape);

            } //end loop though both sides

            //remove original shape (later)
            fixtures_to_destroy.push_back(f);

            //add it to the current body (later)


        } //end shape cutting loop

        for (b2Fixture* f : fixtures_to_destroy){
            b->DestroyFixture(f);
        }

        b2FixtureDef f_def = this->tetrisFixtures.at(0).at(0);
        for (b2PolygonShape s : shapes_to_make){
            /*printf("Adding shape to new body:\n");
            for (uint i = 0; i < s.m_count; i++){
                printf("\t%s\n", this->b2Vec2String(b->GetWorldPoint(s.m_vertices[i])).toUtf8().constData());
            }*/
            f_def.shape = &s;
            b->CreateFixture(&f_def);
        }

    }//end body reshape loop



    //(Now all bodies have been cut, but are still one rigid body each. they need to be split)
    vector<b2Body*> bodies_to_destroy;
    for (b2Body* b : affected_bodies){
        //each vector in shape_groups represents a number of shapes that are touching,
        //directly or indirectly via each other
        vector<vector<b2PolygonShape*>> shape_groups;

        //transform doesnt matter when testing overlaps because all these shapes are part of one body to start
        b2Transform t;
        t.SetIdentity();

        for (b2Fixture* f = b->GetFixtureList(); f; f = f->GetNext()){
            Q_ASSERT(f->GetShape()->GetType() == b2Shape::e_polygon);
            b2PolygonShape* s = static_cast<b2PolygonShape*>(f->GetShape());

            bool found_touch = false;
            for (uint g = 0; g < shape_groups.size(); g++){

                for (b2PolygonShape* vs : shape_groups.at(g)){

                    //if shape is touching vshape
                    if (b2TestOverlap(s, 0, vs, 0, t, t)){
                        //add shape to vector(v)
                        shape_groups.at(g).push_back(s);
                        found_touch = true;
                        break;
                    }
                }
                if (found_touch) break;
            }
            //if shape wasn't touching any other shapes in the vector
            if (!found_touch){
                //add shape to its own svector, add that svector to the vector
                shape_groups.push_back(vector<b2PolygonShape*>());
                shape_groups.back().push_back(s);
            }
        } //end shape grouping looping

        b2BodyDef new_body_def = this->tetrisBodyDef;
        new_body_def.gravityScale = 1.0f;
        new_body_def.linearVelocity.SetZero();
        new_body_def.angle = b->GetAngle();
        new_body_def.position = b->GetPosition();

        for (vector<b2PolygonShape*> group : shape_groups){

            //make new body
            b2Body* new_body = this->world->CreateBody(&new_body_def);

            b2FixtureDef fixture_def = this->tetrisFixtures.at(0).at(0);

            //add shapes in svector to body
            for (b2PolygonShape* s : group){
                fixture_def.shape = s;
                new_body->CreateFixture(&fixture_def);
            }

            tetrisPieceData* orig_body_data = this->getTetrisPieceData(b);
            if (orig_body_data != nullptr){
                tetrisPieceData* data = new tetrisPieceData(*orig_body_data);
                Q_ASSERT(data->image.hasAlphaChannel());
                data->image = this->maskImage(data->image, new_body, data->region);
                Q_ASSERT(data->image.hasAlphaChannel());
                new_body->SetUserData(data);
            }
        }

        //delete original body (later)
        bodies_to_destroy.push_back(b);

    } //end body separation loop

    for (b2Body* b : bodies_to_destroy){
        this->freeUserDataOn(b);
        this->world->DestroyBody(b);
    }

    //fflush(stdout);
}

QPixmap NT3Game::maskImage(QPixmap pixmap, b2Body* b, QRect rect){
    if (!pixmap.hasAlphaChannel()){
        printf("Piece has no alpha channel!\n");
        fflush(stdout);
    }

    QImage image = pixmap.toImage();

    float32 scale = 1.0f/static_cast<float32>(this->max_graphics_scale);

    b2Vec2 offset(rect.x(), rect.y());

    b2Transform t;
    t.SetIdentity();

    /*printf("New body:\n");
    for (b2Fixture* f = b->GetFixtureList(); f; f = f->GetNext()){
        printf("New fixture:\n");
        Q_ASSERT(f->GetShape()->GetType() == b2Shape::e_polygon);
        b2PolygonShape* s = static_cast<b2PolygonShape*>(f->GetShape());

        for (int i = 0; i < s->m_count; i++){
            printf("%s\n", this->b2Vec2String(s->m_vertices[i]).toUtf8().constData());
        }
    }*/

    for (int y = 0; y < image.height(); y++){
        for (int x = 0; x < image.width(); x++){
            if (qAlpha(image.pixel(x, y)) == 0){
                continue;
            }

            b2Vec2 vec(x, y);
            //printf("%s --> ", this->b2Vec2String(vec).toUtf8().constData());
            vec *= scale;
            vec += offset;
            //printf("%s\n", this->b2Vec2String(vec).toUtf8().constData());
            //vec = b->GetWorldPoint(vec);

            bool pass = false;
            for (b2Fixture* f = b->GetFixtureList(); f; f = f->GetNext()){
                Q_ASSERT(f->GetShape()->GetType() == b2Shape::e_polygon);
                b2PolygonShape* s = static_cast<b2PolygonShape*>(f->GetShape());

                if (s->TestPoint(t, vec)){
                    pass = true;
                    //printf("Pass!\n");
                    break;
                }
            }

            if (!pass){
                image.setPixelColor(x, y, QColor(0, 0, 255, 0)); //set to transparent
            }
        }
    }
    return QPixmap::fromImage(image);
}

vector<rayCastComplete> NT3Game::getRayCasts(float32 top, float32 bot){
    vector<rayCastComplete> ray_casts;

    float32 left = -this->side_length;
    float32 right = this->tetris_field.width() + this->side_length;

    for (uint8 r = 0; r < num_ray_casts; r++){
        rayCastComplete ray_cast;
        ray_casts.push_back(ray_cast);
        ray_casts.at(r).input.maxFraction = 1;
        switch(r){
        case TOPLEFT:
            ray_casts.at(r).input.p1.Set(left, top);
            ray_casts.at(r).input.p2.Set(right, top);
            break;
        case TOPRIGHT:
            ray_casts.at(r).input.p1.Set(right, top);
            ray_casts.at(r).input.p2.Set(left, top);
            break;
        case BOTTOMLEFT:
            ray_casts.at(r).input.p1.Set(left, bot);
            ray_casts.at(r).input.p2.Set(right, bot);
            break;
        case BOTTOMRIGHT:
            ray_casts.at(r).input.p1.Set(right, bot);
            ray_casts.at(r).input.p2.Set(left, bot);
            break;
        default:
            fprintf(stderr, "Ray cast enum not defined\n");
            break;
        } //end ray cast switch statement
    } //end ray cast init

    return ray_casts;
}

b2Vec2 NT3Game::hit_point(rayCastComplete ray_cast){
    Q_ASSERT(ray_cast.hit);
    return ray_cast.input.p1 + ray_cast.output.fraction * (ray_cast.input.p2 - ray_cast.input.p1);
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
        // In most compiliers, x == x0 will generat a warning regarding comparing floating point values with ==.
        //This warning may be safely ignored because this is a case that lies
        //outside the typical mistakes of programmers using floating point.
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
        return 0;
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
        if(!(edge.LengthSquared() > b2_epsilon * b2_epsilon)){
            return 0;
        }
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
    if (area > this->min_poly_area){
        return area;
    }
    //printf("area <= %f\n", b2_epsilon);
    return 0;
}


void NT3Game::makeNewTetrisPiece(){

    tetris_piece_enum type = static_cast<tetris_piece_enum>(this->rng.bounded(num_tetris_pieces));

    this->currentPiece = world->CreateBody(&this->tetrisBodyDef);
    this->contactlistener->currentPiece = this->currentPiece;

    for (b2FixtureDef f : this->tetrisFixtures.at(type)){
        this->currentPiece->CreateFixture(&f);
    }

    tetrisPieceData* data = new tetrisPieceData;
    data->image = this->piece_images.at(type);
    data->region = this->piece_rects.at(type);
    this->currentPiece->SetUserData(data);
}


bool NT3Game::isAWall(b2Body* b){
    for (uint i = 0; i < num_walls; i++){
        if (b == this->walls[i]){
            return true;
        }
    }
    return false;
}

QString NT3Game::b2Vec2String(b2Vec2 vec){
    return QString("(%1, %2)").arg(static_cast<double>(vec.x)).arg(static_cast<double>(vec.y));
}

tetrisPieceData* NT3Game::getTetrisPieceData(b2Body* b){

    void* data = b->GetUserData();
    if (data == nullptr) return nullptr;

    return static_cast<tetrisPieceData*>(data);
}

QPixmap NT3Game::enableAlphaChannel(QPixmap pixmap){
    if (pixmap.hasAlphaChannel()) return pixmap;

    QPixmap ans(pixmap.size());
    ans.fill(Qt::transparent);

    QPainter p(&ans);
    p.drawPixmap(ans.rect(), pixmap);
    p.end();

    return ans;
}


void NT3Game::initializeTetrisPieceDefs(){


    this->tetrisBodyDef.type = b2_dynamicBody;

    this->tetrisBodyDef.allowSleep = true;
    this->tetrisBodyDef.awake = true;

    this->tetrisBodyDef.bullet = false;

    this->tetrisBodyDef.position.Set(static_cast<float32>(this->tetris_field.width()/2), -this->side_length*2);

    this->tetrisBodyDef.gravityScale = 0;

    this->tetrisBodyDef.linearVelocity = b2Vec2(0, this->downward_velocity_regular);
    this->tetrisBodyDef.angularVelocity = 0;

    this->tetrisBodyDef.linearDamping = 0.5f;
    this->tetrisBodyDef.angularDamping = 0.1f;


    for (uint8 i = 0; i < num_tetris_pieces; i++){
        this->tetrisShapes.push_back(vector<b2PolygonShape>());
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
    fixture_template.density = this->density;
    fixture_template.friction = this->piece_friction_k;
    fixture_template.restitution = this->restitution;

    vector<b2FixtureDef> fixture_vector_template;
    for (uint32 t = 0; t < this->tetrisShapes.size(); t++){
        this->tetrisFixtures.push_back(fixture_vector_template);
        for (uint32 s = 0; s < this->max_shapes_per_piece; s++){
            if (this->tetrisShapes.at(t).size() == s) break;
            fixture_template.shape = &this->tetrisShapes.at(t).at(s);
            this->tetrisFixtures.at(t).push_back(fixture_template);
        }
    }
}

void NT3Game::initializeTetrisPieceImages(){
    int side_length = static_cast<int>(this->side_length);
    for (uint8 piece = 0; piece < num_tetris_pieces; piece++){

        QString path = ":/resources/graphics/pieces/" + QString::number(piece) + ".png";
        QPixmap orig_pixmap = QPixmap(path);
        orig_pixmap = orig_pixmap.scaled(orig_pixmap.size()*this->max_graphics_scale);

        this->piece_images.push_back(this->enableAlphaChannel(orig_pixmap));

        /*printf("%u: orig_pixmap: %u, this->piece_images.back(): %u\n",
               piece, orig_pixmap.hasAlphaChannel(), this->piece_images.back().hasAlphaChannel());*/

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
    this->default_piece_image = this->piece_images.at(default_tetris_piece);
    this->default_piece_rect = this->piece_rects.at(default_tetris_piece);
}

void NT3Game::initializeWalls(){
    b2BodyDef edgeBodyDef;
    edgeBodyDef.position.Set(0, 0);

    b2EdgeShape edge;
    edge.Set(b2Vec2(0, tetris_field.height()), b2Vec2(tetris_field.width(), tetris_field.height()));

    this->walls[GROUND] = world->CreateBody(&edgeBodyDef);
    this->walls[GROUND]->CreateFixture(&edge, 0.0f);
    this->walls[GROUND]->GetFixtureList()->SetFriction(this->ground_friction_k);
    this->walls[GROUND]->GetFixtureList()->SetRestitution(this->restitution);

    edge.Set(b2Vec2(0, 0), b2Vec2(0, tetris_field.height()));

    this->walls[LEFTWALL] = world->CreateBody(&edgeBodyDef);
    this->walls[LEFTWALL]->CreateFixture(&edge, 0.0f);
    this->walls[LEFTWALL]->GetFixtureList()->SetFriction(0);
    this->walls[LEFTWALL]->GetFixtureList()->SetRestitution(this->restitution);

    edge.Set(b2Vec2(tetris_field.width(), 0), b2Vec2(tetris_field.width(), tetris_field.height()));

    this->walls[RIGHTWALL] = world->CreateBody(&edgeBodyDef);
    this->walls[RIGHTWALL]->CreateFixture(&edge, 0.0f);
    this->walls[RIGHTWALL]->GetFixtureList()->SetFriction(0);
    this->walls[RIGHTWALL]->GetFixtureList()->SetRestitution(this->restitution);
}

void NT3Game::init_BDC(){
    this->row_densities.clear();
    this->body_density_contributions.clear();
    for (uint r = 0; r < this->tetris_rows; r++){
        this->row_densities.push_back(0.0f);
        this->body_density_contributions.push_back(QHash<b2Body*, float32>());
    }
}
