#include "gameb.h"
#include "nt3window.h"

GameB::GameB(QObject *parent) : NT3Screen(parent)
{    
    if (this->gamebackground.isNull()){
        fprintf(stderr, "Resources not present, exiting...\n");
        emit this->close();
    }
    
    double fps = 1.0/framerate_s_f;
    this->timeStep = static_cast<float32>(framerate_s_f); //seconds
    
    float32 box2d_max_velocity = b2_maxTranslation*static_cast<float32>(fps);
    
    Q_ASSERT(this->downward_velocity_max < box2d_max_velocity);
    Q_ASSERT(this->downward_velocity_regular < box2d_max_velocity);
    
    this->rng = QRandomGenerator::securelySeeded();
    //this->rng = QRandomGenerator(9003);
    
    //key-action mappings
    this->freeze_key = Qt::Key_Space;
    this->accelDownKey = Qt::Key_Down;
    this->rotateStateTable.insert(Qt::Key_Z, ROTATECCW);
    this->rotateStateTable.insert(Qt::Key_X, ROTATECW);
    this->lateralMovementStateTable.insert(Qt::Key_Left, MOVELEFT);
    this->lateralMovementStateTable.insert(Qt::Key_Right, MOVERIGHT);
    
    this->sfx[BLOCK_MOVE].setSource(QUrl("qrc:/resources/sounds/effects/move.wav"));
    this->sfx[BLOCK_TURN].setSource(QUrl("qrc:/resources/sounds/effects/turn.wav"));
    
    this->sfx[BLOCK_FALL].setSource(QUrl("qrc:/resources/sounds/effects/blockfall.wav"));
    this->sfx[LINE_CLEAR].setSource(QUrl("qrc:/resources/sounds/effects/lineclear.wav"));
    this->sfx[FOUR_LINE_CLEAR].setSource(QUrl("qrc:/resources/sounds/effects/4lineclear.wav"));
    
    this->sfx[GAME_OVER_SOUND].setSource(QUrl("qrc:/resources/sounds/effects/gameover1.wav"));
    this->sfx[NEW_LEVEL].setSource(QUrl("qrc:/resources/sounds/effects/newlevel.wav"));
    
    this->sfx[PAUSE_SOUND].setSource(QUrl("qrc:/resources/sounds/effects/pause.wav"));
    
    if (this->frame_review){
        fprintf(stderr, "Warning: frame review turned on. All render times will be more than doubled!\n");
        fflush(stderr);
    }
}

GameB::~GameB()
{
    this->destroyWorld();
}

void GameB::destroyWorld(){
    if (this->world){
        for (b2Body* b = this->world->GetBodyList(); b; b = b->GetNext()){
            this->freeUserDataOn(b);
        }
        delete world;
    }
    if (this->contactlistener){
        delete this->contactlistener;
    }
    
    this->userData.clear();
    this->next_piece_for_display = nullptr;
    for (uint i = 0; i < num_walls; ++i){
        this->walls[i] = nullptr;
    }
    this->currentPiece = nullptr;
}


void GameB::init(){
    
    this->accelDownState = false;
    this->rotateState = NO_ROTATION;
    this->lateralMovementState = NO_LATERAL_MOVEMENT;
    
    this->game_state = gameB;
    this->paused = false;
    
    this->current_score = 0;
    this->score_to_add = 0;
    this->pieces_scored = 0;
    this->current_level = 0;
    
    for (uint s = 0; s < num_sound_effects; ++s){
        this->sfx[s].setVolume(volume*volume_sfx_multiplier);
    }
    
    this->destroyWorld();
    
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
    
    this->next_piece_type = static_cast<tetris_piece_enum>(this->rng.bounded(num_tetris_pieces));
    
    this->makeNewTetrisPiece();
    
    this->makeNewNextPiece();
}


void GameB::freeUserDataOn(b2Body* b){
    if (!b) return;
    /*int numremoved = this->userData.remove(b);
    if (numremoved == 0 && !this->isAWall(b)){
        printf("%p had no user data!\n", (void*)b);
        Q_ASSERT(false);
    }*/
}


void GameB::calcScaleFactors(){
    this->physics_scale = this->physics_to_ui_scale*this->ui_to_screen_scale_px_in;
    this->scaled_tetris_field = TO_QRECT(this->tetris_field, this->physics_scale);
    
    /*double var = this->physics_scale*this->tetris_field.x();
    printf("%f\n", var - static_cast<int>(var));
    fflush(stdout);*/
}

void GameB::render(QPainter& painter)
{
    painter.setRenderHint(QPainter::Antialiasing);
    
    if (this->freeze_frame){
        painter.drawPixmap(this->ui_field_px, this->saved_frames[this->last_frame]);
        return;
    } else if (this->frame_review){
        this->last_frame = (this->last_frame + 1) % NUM_FRAMES_TO_SAVE;
        this->saved_frames[this->last_frame] = QPixmap(this->ui_field_px.size());
        QPainter sf_painter(&this->saved_frames[this->last_frame]);
        
        this->frame_review = false;
        this->render(sf_painter);
        this->frame_review = true;
        
        sf_painter.end();
    }
    
    if (this->paused){
        painter.drawPixmap(this->ui_field_px, this->pause_frame);
        painter.drawPixmap(this->ui_field_px, this->pause_overlay);
        return;
    }
    
#ifdef TIME_RENDER_STEPS
    QElapsedTimer timer;
    timer.start();
#endif
    
    painter.drawPixmap(this->ui_field_px, this->gamebackground);
    
#ifdef TIME_RENDER_STEPS
    printf("BG: %lld ms \t", timer.elapsed());
    timer.restart();
#endif
    
    this->drawScore(&painter);
    
#ifdef TIME_RENDER_STEPS
    printf("Score: %lld ms \t", timer.elapsed());
    timer.restart();
#endif
    
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
    
#ifdef TIME_RENDER_STEPS
    printf("Bodies: %lld ms \t", timer.elapsed());
    timer.restart();
#endif
}

void GameB::colorizeResources(){
    this->gamebackground = this->colorize(this->gamebackground);
    this->default_piece_image = this->colorize(this->default_piece_image);
    for (QPixmap p : this->piece_images){
        p = this->colorize(p);
    }
    
    if (this->world){
        for (b2Body* b = this->world->GetBodyList(); b; b = b->GetNext()){
            tetrisPieceData tpd = this->getTetrisPieceData(b);
            tpd.image = this->colorize(tpd.image);
            this->setTetrisPieceData(b, tpd);
        }
    }
}

void GameB::drawBodyTo(QPainter* painter, b2Body* body){
    
    painter->save();
    painter->translate(
                this->scaled_tetris_field.x() + static_cast<double>(body->GetPosition().x)*this->physics_scale,
                this->scaled_tetris_field.y() + static_cast<double>(body->GetPosition().y)*this->physics_scale
                );
    
    QString ptrStr = QString("0x%1").arg(reinterpret_cast<quintptr>(body),QT_POINTER_SIZE * 2, 16, QChar('0'));
    //https://stackoverflow.com/questions/8881923/how-to-convert-a-pointer-value-to-qstring
    painter->drawText(QPoint(0, 0), ptrStr);
    //printf("\t%s: (%f, %f)\n", ptrStr.toUtf8().constData(), body->GetPosition().x, body->GetPosition().y);
    
    painter->rotate(static_cast<double>(body->GetAngle())*DEG_PER_RAD);
    
    for (b2Fixture* f = body->GetFixtureList(); f; f = f->GetNext()){
        switch(f->GetType()){
        case b2Shape::e_polygon:{
            b2PolygonShape shape = *static_cast<b2PolygonShape*>(f->GetShape());
            int numpoints = shape.m_count;
            vector<QPointF> points;
            for (int i = 0; i < numpoints; i++){
                points.push_back(
                            QPointF(
                                static_cast<double>(shape.m_vertices[i].x)*this->physics_scale,
                                static_cast<double>(shape.m_vertices[i].y)*this->physics_scale
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
                        static_cast<double>(shape.m_p.x)*this->physics_scale,
                        static_cast<double>(shape.m_p.y)*this->physics_scale
                        );
            double radius = static_cast<double>(shape.m_radius);
            radius *= this->ui_to_screen_scale_px_in;
            painter->drawEllipse(center, radius, radius);
        }
            break;
        case b2Shape::e_edge:{
            b2EdgeShape shape = *static_cast<b2EdgeShape*>(f->GetShape());
            QPointF p1 = QPointF(
                             static_cast<double>(shape.m_vertex1.x)*this->physics_scale,
                             static_cast<double>(shape.m_vertex1.y)*this->physics_scale
                             );
            QPointF p2 = QPointF(
                             static_cast<double>(shape.m_vertex2.x)*this->physics_scale,
                             static_cast<double>(shape.m_vertex2.y)*this->physics_scale
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
                            static_cast<double>(shape.m_vertices[i].x)*this->physics_scale,
                            static_cast<double>(shape.m_vertices[i].y)*this->physics_scale
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

void GameB::drawTetrisPiece(QPainter* painter, b2Body* piece_body){
    
    painter->save();
    
    painter->translate(
                this->scaled_tetris_field.x() + static_cast<double>(piece_body->GetPosition().x)*this->physics_scale,
                this->scaled_tetris_field.y() + static_cast<double>(piece_body->GetPosition().y)*this->physics_scale
                );
    painter->scale(this->physics_scale, this->physics_scale);
    painter->rotate(static_cast<double>(piece_body->GetAngle())*DEG_PER_RAD);
    
    tetrisPieceData body_data = this->getTetrisPieceData(piece_body);
    painter->drawPixmap(body_data.region, body_data.image);
    
    painter->restore();
}

void GameB::drawScore(QPainter* painter){
    
    this->BOW_font.print(painter, this->score_display_right*this->ui_to_screen_scale_px_in, RIGHT_ALIGN,
                         QString::number(this->current_score), this->ui_to_screen_scale_px_in);
    
    if (this->score_to_add > 0){
        
        QPixmap score_add_pm(this->score_add_display.size()*this->ui_to_screen_scale_px_in);
        score_add_pm.fill(Qt::black);
        
        QPainter score_add_painter(&score_add_pm);
        
        score_add_painter.translate(QPoint(0, this->score_add_disp_offset));
        
        this->WOB_font.print(&score_add_painter, this->sc_add_right_in_disp*this->ui_to_screen_scale_px_in, RIGHT_ALIGN,
                             "+" + QString::number(this->score_to_add), this->ui_to_screen_scale_px_in);
        score_add_painter.end();
        
        painter->save();
        painter->translate(this->score_add_display.topLeft()*this->ui_to_screen_scale_px_in);
        painter->drawPixmap(score_add_pm.rect(), score_add_pm);
        painter->restore();
    }
    
    this->BOW_font.print(painter, this->level_disp_offset*this->ui_to_screen_scale_px_in, RIGHT_ALIGN,
                         QString::number(this->current_level), this->ui_to_screen_scale_px_in);
    
    this->BOW_font.print(painter, this->lines_cleared_disp_offset*this->ui_to_screen_scale_px_in, RIGHT_ALIGN,
                         QString::number(this->pieces_scored), this->ui_to_screen_scale_px_in);
}


void GameB::keyPressEvent(QKeyEvent* ev){
    //printf("Key pressed: %s\n", ev->text().toUtf8().constData());
    //fflush(stdout);
    
    int key = ev->key();
    
    if (this->frame_review && this->freeze_frame){
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
        lateral_movement_state_enum previous_state = this->lateralMovementState;
        
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
        
        if (previous_state != this->lateralMovementState){
            this->sfx[BLOCK_MOVE].play();
        }
        
    } else if (this->rotateStateTable.contains(key)){
        rotate_state_enum requested_rotation = this->rotateStateTable.value(key);
        rotate_state_enum other_rotation = requested_rotation == ROTATECW ? ROTATECCW : ROTATECW;
        rotate_state_enum previous_state = this->rotateState;
        
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
        
        if (previous_state != this->rotateState){
            this->sfx[BLOCK_TURN].play();
        }
        
    } else if (key == this->accelDownKey){
        this->accelDownState = true;
    } else if (this->frame_review && key == this->freeze_key){
        
        this->freeze_frame = !this->freeze_frame;
        
        this->lateralMovementState = NO_LATERAL_MOVEMENT;
        this->rotateState = NO_ROTATION;
        this->accelDownState = false;
    } else if (key == Qt::Key_Enter || key == Qt::Key_Return){
        if (!this->paused){ // NOT paused, about to pause
            this->pause_frame = QPixmap(this->ui_field_px.size());
            QPainter painter(&this->pause_frame);
            this->render(painter);
            painter.end();
            
            ((NT3Window*)(this->parent()))->music_player.pause();
            this->sfx[PAUSE_SOUND].play();
        } else { // already paused and about to unpause
            ((NT3Window*)(this->parent()))->music_player.play();
        }
        this->paused = !this->paused;
    } else if (key == Qt::Key_Escape){
        emit this->stateEnd(P_GAMEOPTIONS);
    }
}

void GameB::keyReleaseEvent(QKeyEvent* ev){
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

void GameB::doGameStep(){
    
    if (this->freeze_frame) return;
    if (this->paused) return;
    
    this->next_piece_for_display->SetLinearVelocity(b2Vec2(0, 0));
    this->next_piece_for_display->SetAngularVelocity(this->next_piece_w);
    
#ifdef TIME_GAME_FRAME
    QElapsedTimer timer;
    timer.start();
#endif
    
    this->world->Step(this->timeStep, this->velocityIterations, this->positionIterations);
    
#ifdef TIME_GAME_FRAME
    printf("World step: %lld ms,\t", timer.elapsed());
    timer.restart();
#endif
    
    if (this->score_to_add > 0){
        if (--this->score_add_disp_offset < -10*this->ui_to_screen_scale_px_in){
            this->score_to_add = 0;
            this->score_add_disp_offset = 0;
        }
    }
    //printf("Score to add: %d; Offset: %d\n", this->score_to_add, this->score_add_disp_offset);
    
    bool touchdown = false;
    if (this->contactlistener->hasCurrentPieceCollided() && this->game_state == gameB){
        touchdown = true;
        this->currentPiece->SetGravityScale(1);
        
        if (this->score_to_add > 0){
            this->score_add_disp_offset = 0;
        }
        this->score_to_add = this->score_per_piece;
        this->current_score += this->score_per_piece;
        
        ++this->pieces_scored;
        if (this->pieces_scored/this->pieces_per_level > this->current_level){
            this->current_level = this->pieces_scored/this->pieces_per_level;
            this->tetrisBodyDef.linearVelocity = 
                    b2Vec2(
                        0, 
                        this->downward_velocity_regular + 
                        this->downward_velocity_level_increment*this->current_level
                        );
            this->sfx[NEW_LEVEL].play();
        }
        
        if (this->currentPiece->GetWorldCenter().y < 0){
            
            printf("Game lost!\n");
            this->game_state = flush_blocksB;
            
            this->world->DestroyBody(this->walls[GROUND]);
            
            emit this->changeMusic(QUrl());
            this->sfx[GAME_OVER_SOUND].play();
        }
        
        if (this->game_state != flush_blocksB){
            //start with making the new tetris piece and make the new next piece 
            //only if we're not clearing lines (found below)
            this->makeNewTetrisPiece();
        }
    }
    
    if (this->game_state != flush_blocksB){
        
        float32 inertia = this->currentPiece->GetInertia();
        
        switch(this->rotateState){
        case NO_ROTATION:
        case BOTH_ROTATIONS:
            //do nothing
            //printf("Dont rotate\n");
            break;
        case ROTATECW:
            //printf("Rotate CW\n");
            if (this->currentPiece->GetAngularVelocity() < this->wmax){
                this->currentPiece->ApplyTorque(this->angular_accel*inertia, true);
            }
            break;
        case ROTATECCW:
            //printf("Rotate CCW\n");
            if (this->currentPiece->GetAngularVelocity() > -this->wmax){
                this->currentPiece->ApplyTorque(-this->angular_accel*inertia, true);
            }
            break;
        default:
            fprintf(stderr, "Invalid Rotation state\n");
            break;
        }
        
        float32 mass = this->currentPiece->GetMass();
        b2Vec2 linear_force_vect = b2Vec2(0, 0);
        
        switch(this->lateralMovementState){
        case NO_LATERAL_MOVEMENT:
        case BOTH_DIRECTIONS:
            //do nothing
            break;
        case MOVELEFT:
            linear_force_vect.x = -this->lateral_accel*mass;
            break;
        case MOVERIGHT:
            linear_force_vect.x = this->lateral_accel*mass;
            break;
        default:
            fprintf(stderr, "Invalid Lateral Movement state\n");
            break;
        }
        
        float32 y_velocity = this->currentPiece->GetLinearVelocity().y;
        float32 downward_velocity_adjusted = 
                this->downward_velocity_regular + 
                this->downward_velocity_level_increment*this->current_level;
        
        if (!this->accelDownState && qAbs(y_velocity - downward_velocity_adjusted) <= 1){
            linear_force_vect.y = 0;
            this->currentPiece->SetLinearVelocity(b2Vec2(this->currentPiece->GetLinearVelocity().x, downward_velocity_adjusted));
        } else if (this->accelDownState || y_velocity < downward_velocity_adjusted){
            //printf("forcing downwards\n");
            linear_force_vect.y = this->downward_accel*mass;
        } else {
            //printf("slowing down...\n");
            linear_force_vect.y = -this->upward_correcting_accel*mass;
        }
        this->currentPiece->ApplyForce(linear_force_vect, this->currentPiece->GetWorldCenter(), true);
    } else {
        if (static_cast<double>(this->currentPiece->GetWorldCenter().y) > this->tetris_field.height()){
            
            // Copy score to window object for reference by future screens
            ((NT3Window*)(this->parent()))->gameB_score = this->current_score;
            
            //printf("Re-rendering frame for gameover screen...\n");
            
            //render the current frame onto a pixmap to use later during game over screen
            QPixmap lastframe(this->ui_field_px.size());
            QPainter sf_painter(&lastframe);
            this->render(sf_painter);
            sf_painter.end();
            
            ((NT3Window*)(this->parent()))->game_lastframe = lastframe;
            
            emit this->stateEnd(GAME_LOST);
        }
    }
    
#ifdef TIME_GAME_FRAME
    printf("Currpiece math: %lld ms\t", timer.elapsed());
    timer.restart();
#endif
    
    if (touchdown && this->game_state != flush_blocksB){
        this->makeNewNextPiece();
        this->sfx[BLOCK_FALL].play();
    }
    
#ifdef TIME_GAME_FRAME
    printf("Next Piece: %lld ms\n", timer.elapsed());
#endif
}


void GameB::makeNewTetrisPiece(){
    
    //set up current piece
    tetris_piece_enum type = this->next_piece_type;
    
    this->currentPiece = world->CreateBody(&this->tetrisBodyDef);
    this->contactlistener->currentPiece = this->currentPiece;
    
    for (b2FixtureDef f : this->tetrisFixtures.at(type)){
        this->currentPiece->CreateFixture(&f);
    }
    
    tetrisPieceData data(this->piece_images.at(type), this->piece_rects.at(type), false);
    this->setTetrisPieceData(this->currentPiece, data);    
}

void GameB::makeNewNextPiece(){
    this->next_piece_type = static_cast<tetris_piece_enum>(this->rng.bounded(num_tetris_pieces));
    
    this->destroyTetrisPiece(this->next_piece_for_display);
    
    this->next_piece_bodydef.position = b2Vec2(
                                            static_cast<float32>(
                                                this->next_piece_display_center.x()*1.0/this->physics_to_ui_scale - 
                                                this->tetris_field.x()
                                                ), 
                                            static_cast<float32>(
                                                this->next_piece_display_center.y()*1.0/this->physics_to_ui_scale - 
                                                this->tetris_field.y()
                                                )
                                            ) - this->center_of_mass_offsets.at(this->next_piece_type);
    this->next_piece_bodydef.angularVelocity = this->next_piece_w;
    
    this->next_piece_for_display = this->world->CreateBody(&this->next_piece_bodydef);
    this->next_piece_for_display->SetLinearVelocity(b2Vec2(0, 0));
    
    for (b2FixtureDef f : this->tetrisFixtures.at(this->next_piece_type)){
        this->next_piece_for_display->CreateFixture(&f);
    }
    
    tetrisPieceData data(this->piece_images.at(this->next_piece_type), this->piece_rects.at(this->next_piece_type), false);
    this->setTetrisPieceData(this->next_piece_for_display, data);
}


bool GameB::isAWall(b2Body* b){
    for (uint i = 0; i < num_walls; i++){
        if (b == this->walls[i]){
            return true;
        }
    }
    return false;
}

tetrisPieceData GameB::getTetrisPieceData(b2Body* b){
    
    tetrisPieceData ans = this->userData.value(b, this->default_data);
    
    /*if (ans == this->default_data && !this->isAWall(b)){ //VERY SLOW
        printf("%p has no data!!\n", (void*)b);
    }*/
    
    return ans;
    
    /*void* data = b->GetUserData();
    if (data == nullptr) return nullptr;
    
    return static_cast<tetrisPieceData*>(data);*/
}

void GameB::setTetrisPieceData(b2Body* b, tetrisPieceData tpd){
    this->userData.remove(b);
    this->userData.insert(b, tpd);
}

QPixmap GameB::enableAlphaChannel(QPixmap pixmap){
    if (pixmap.hasAlphaChannel()) return pixmap;
    
    QPixmap ans(pixmap.size());
    ans.fill(Qt::transparent);
    
    QPainter p(&ans);
    p.drawPixmap(ans.rect(), pixmap);
    p.end();
    
    return ans;
}

void GameB::destroyTetrisPiece(b2Body* b){
    if (!b) return;
    this->freeUserDataOn(b);
    this->world->DestroyBody(b);
}


void GameB::initializeTetrisPieceDefs(){
    
    this->tetrisBodyDef.type = b2_dynamicBody;
    
    this->tetrisBodyDef.allowSleep = true;
    this->tetrisBodyDef.awake = true;
    
    this->tetrisBodyDef.bullet = false;
    
    this->tetrisBodyDef.position = this->piece_start;
    
    this->tetrisBodyDef.gravityScale = 0;
    
    this->tetrisBodyDef.linearVelocity = b2Vec2(0, this->downward_velocity_regular);
    this->tetrisBodyDef.angularVelocity = 0;
    
    this->tetrisBodyDef.linearDamping = this->linear_damping;
    this->tetrisBodyDef.angularDamping = this->angular_damping;
    
    
    this->next_piece_bodydef = this->tetrisBodyDef;
    this->next_piece_bodydef.position = b2Vec2(
                                            static_cast<float32>(
                                                this->next_piece_display_center.x()*1.0/this->physics_to_ui_scale - 
                                                this->tetris_field.x()
                                                ), 
                                            static_cast<float32>(
                                                this->next_piece_display_center.y()*1.0/this->physics_to_ui_scale - 
                                                this->tetris_field.y()
                                                )
                                            );
    this->next_piece_bodydef.linearVelocity.SetZero();
    
    this->tetrisBodyDef.linearDamping = 0;
    this->tetrisBodyDef.angularDamping = 0;
    
    
    this->tetrisShapes.clear();
    for (uint8 i = 0; i < num_tetris_pieces; i++){
        this->tetrisShapes.push_back(vector<b2PolygonShape>());
    }
    
    b2PolygonShape shape_template;
    
    float32 half_length = this->side_length/2;
    
    this->tetrisShapes.at(I).push_back(shape_template);
    this->tetrisShapes.at(I).at(0).SetAsBox(
                this->side_length*2 - b2_polygonRadius, 
                half_length - b2_polygonRadius
                );
    
    this->tetrisShapes.at(O).push_back(shape_template);
    this->tetrisShapes.at(O).at(0).SetAsBox(
                this->side_length - b2_polygonRadius, 
                this->side_length - b2_polygonRadius
                );
    
    this->tetrisShapes.at(G).push_back(shape_template);
    this->tetrisShapes.at(G).push_back(shape_template);
    this->tetrisShapes.at(G).at(0).SetAsBox(
                3*half_length - b2_polygonRadius, 
                half_length - 0.5f*b2_polygonRadius, 
                b2Vec2(0, b2_polygonRadius), 
                0
                );
    this->tetrisShapes.at(G).at(1).SetAsBox(
                half_length - b2_polygonRadius, 
                half_length - 0.5f*b2_polygonRadius, 
                b2Vec2(this->side_length, this->side_length - b2_polygonRadius), 
                0
                );
    
    this->tetrisShapes.at(L).push_back(shape_template);
    this->tetrisShapes.at(L).push_back(shape_template);
    this->tetrisShapes.at(L).at(0).SetAsBox(
                3*half_length - b2_polygonRadius, 
                half_length - 0.5f*b2_polygonRadius, 
                b2Vec2(0, b2_polygonRadius), 
                0
                );
    this->tetrisShapes.at(L).at(1).SetAsBox(
                half_length - b2_polygonRadius, 
                half_length - 0.5f*b2_polygonRadius, 
                b2Vec2(-this->side_length, this->side_length - b2_polygonRadius), 
                0
                );
    
    this->tetrisShapes.at(Z).push_back(shape_template);
    this->tetrisShapes.at(Z).push_back(shape_template);
    this->tetrisShapes.at(Z).at(0).SetAsBox(
                this->side_length - b2_polygonRadius, 
                half_length - 0.5f*b2_polygonRadius, 
                b2Vec2(-half_length, -half_length + b2_polygonRadius), 
                0
                );
    this->tetrisShapes.at(Z).at(1).SetAsBox(
                this->side_length - b2_polygonRadius, 
                half_length - 0.5f*b2_polygonRadius, 
                b2Vec2(half_length, half_length - b2_polygonRadius), 
                0
                );
    
    this->tetrisShapes.at(S).push_back(shape_template);
    this->tetrisShapes.at(S).push_back(shape_template);
    this->tetrisShapes.at(S).at(0).SetAsBox(
                this->side_length - b2_polygonRadius, 
                half_length - 0.5f*b2_polygonRadius, 
                b2Vec2(half_length, -half_length + b2_polygonRadius), 
                0
                );
    this->tetrisShapes.at(S).at(1).SetAsBox(
                this->side_length - b2_polygonRadius, 
                half_length - 0.5f*b2_polygonRadius, 
                b2Vec2(-half_length, half_length - b2_polygonRadius), 
                0
                );
    
    this->tetrisShapes.at(T).push_back(shape_template);
    this->tetrisShapes.at(T).push_back(shape_template);
    this->tetrisShapes.at(T).at(0).SetAsBox(
                3*half_length - b2_polygonRadius, 
                half_length - 0.5f*b2_polygonRadius, 
                b2Vec2(0, b2_polygonRadius), 
                0
                );
    this->tetrisShapes.at(T).at(1).SetAsBox(
                half_length - b2_polygonRadius, 
                half_length - 0.5f*b2_polygonRadius, 
                b2Vec2(0, this->side_length - b2_polygonRadius), 
                0
                );
    
    b2FixtureDef fixture_template;
    fixture_template.density = this->density;
    fixture_template.friction = this->piece_friction_k;
    fixture_template.restitution = this->restitution;
    
    vector<b2FixtureDef> fixture_vector_template;
    
    this->tetrisFixtures.clear();
    this->center_of_mass_offsets.clear();
    
    for (uint32 t = 0; t < this->tetrisShapes.size(); t++){
        
        Q_ASSERT(this->tetrisShapes.at(t).size() <= this->max_shapes_per_piece);
        this->tetrisFixtures.push_back(fixture_vector_template);
        
        for (uint32 s = 0; s < this->max_shapes_per_piece; s++){
            
            if (this->tetrisShapes.at(t).size() == s) break;
            
            fixture_template.shape = &this->tetrisShapes.at(t).at(s);
            
            this->tetrisFixtures.at(t).push_back(fixture_template);
        }
        
        b2Body* testBody = this->world->CreateBody(&this->tetrisBodyDef);
        for (b2FixtureDef f : this->tetrisFixtures.at(t)){
            testBody->CreateFixture(&f);
        }
        this->center_of_mass_offsets.push_back(testBody->GetWorldCenter() - this->tetrisBodyDef.position);
        this->world->DestroyBody(testBody);
    }
}

void GameB::initializeTetrisPieceImages(){
    
    // TODO: this will use the CURRENT screen resolution to upscale the piece images, 
    // and will not handle the case where the game starts on a low res screen and moves to a higher res screen.
    // call init again?
    int screen_height = ((NT3Window*)(this->parent()))->screen()->availableGeometry().height();
    this->piece_image_scale = screen_height*1.0/tetris_field.height();
    
    this->piece_images.clear();
    this->piece_rects.clear();
    
    int side_length = static_cast<int>(this->side_length);
    for (uint8 piece = 0; piece < num_tetris_pieces; piece++){
        
        QString path = ":/resources/graphics/pieces/" + QString::number(piece) + ".png";
        QPixmap orig_pixmap = this->colorize(QPixmap(path));
        orig_pixmap = orig_pixmap.scaled(orig_pixmap.size()*this->piece_image_scale);
        
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
    this->default_data.image = this->default_piece_image;
    this->default_data.region = this->default_piece_rect;
}

void GameB::initializeWalls(){
    
    float32 t_height = static_cast<float32>(tetris_field.height());
    float32 t_width = static_cast<float32>(tetris_field.width());
    
    b2BodyDef edgeBodyDef;
    edgeBodyDef.position.Set(0, 0);
    
    b2EdgeShape edge;
    edge.Set(b2Vec2(0, t_height), b2Vec2(t_width, t_height));
    
    this->walls[GROUND] = world->CreateBody(&edgeBodyDef);
    this->walls[GROUND]->CreateFixture(&edge, 0.0f);
    this->walls[GROUND]->GetFixtureList()->SetFriction(this->ground_friction_k);
    this->walls[GROUND]->GetFixtureList()->SetRestitution(this->restitution);
    
    edge.Set(b2Vec2(0, 0), b2Vec2(0, t_height));
    
    this->walls[LEFTWALL] = world->CreateBody(&edgeBodyDef);
    this->walls[LEFTWALL]->CreateFixture(&edge, 0.0f);
    this->walls[LEFTWALL]->GetFixtureList()->SetFriction(0);
    this->walls[LEFTWALL]->GetFixtureList()->SetRestitution(this->restitution);
    
    edge.Set(b2Vec2(t_width, 0), b2Vec2(t_width, t_height));
    
    this->walls[RIGHTWALL] = world->CreateBody(&edgeBodyDef);
    this->walls[RIGHTWALL]->CreateFixture(&edge, 0.0f);
    this->walls[RIGHTWALL]->GetFixtureList()->SetFriction(0);
    this->walls[RIGHTWALL]->GetFixtureList()->SetRestitution(this->restitution);
}
