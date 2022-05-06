#include "gamea.h"
#include "nt3window.h"

GameA::GameA(QObject *parent) : NT3Screen(parent)
{    
    if (this->gamebackground.isNull()){
        fprintf(stderr, "Resources not present, exiting...\n");
        emit this->close();
    }
    
    double fps = 1.0/framerate_s_f;
    this->timeStep_s = static_cast<float32>(framerate_s_f);
    
    float32 box2d_max_velocity_m_s = b2_maxTranslation*static_cast<float32>(fps);
    
    Q_ASSERT(this->downward_velocity_max_m_s < box2d_max_velocity_m_s);
    Q_ASSERT(this->downward_velocity_regular_m_s < box2d_max_velocity_m_s);
    
    this->rng = QRandomGenerator::securelySeeded();
    //this->rng = QRandomGenerator(9002);
    
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

GameA::~GameA()
{
    this->destroyWorld();
}

void GameA::destroyWorld(){
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
    this->bodies_to_destroy.clear();
    this->init_BDC();
}


void GameA::init(){
    
    this->accelDownState = false;
    this->rotateState = NO_ROTATION;
    this->lateralMovementState = NO_LATERAL_MOVEMENT;
    
    this->game_state = gameA;
    this->paused = false;
    this->skip_falling = false;
    
    this->current_score = 0;
    this->score_to_add = 0;
    this->lines_cleared = 0;
    this->current_level = 0;
    this->new_level_reached = false;
    
    this->num_blinks_so_far = 0;
    this->row_blink_accumulator_s = 0;
    this->row_blink_on = true;
    
    for (uint s = 0; s < num_sound_effects; ++s){
        this->sfx[s].setVolume(volume*volume_sfx_multiplier);
    }
    
    this->rows_to_clear.clear();
    for (uint i = 0; i < this->tetris_rows; i++){
        this->rows_to_clear.push_back(false);
    }

    this->clear_diag_cut = false;
    this->diag_top_m = FP_NAN;
    this->diag_bot_m = FP_NAN;
    this->diag_slope = FP_NAN;

    this->shake_field = false;
    this->shake_time_acc_s = 0;

    this->destroyWorld();
    
    b2Vec2 gravity_m_s2(0.0f, this->gravity_g_m_s2);
    this->world = new b2World(gravity_m_s2);
    this->world->SetAllowSleeping(true);
    
    this->contactlistener = new NT3ContactListener;
    this->world->SetContactListener(this->contactlistener);
    
    this->initializeTetrisPieceDefs();
    
    this->initializeTetrisPieceImages();
    
    this->initializeWalls();
    
    this->contactlistener->exceptions.push_back(this->walls[LEFTWALL]);
    this->contactlistener->exceptions.push_back(this->walls[RIGHTWALL]);
    
    this->next_piece_type = static_cast<tetris_piece_enum>(this->rng.bounded(num_tetris_pieces));
    
    this->makeNewTetrisPiece(false);
}


void GameA::freeUserDataOn(b2Body* b){
    if (!b) return;
    /*int numremoved = this->userData.remove(b);
    if (numremoved == 0 && !this->isAWall(b)){
        printf("%p had no user data!\n", (void*)b);
        Q_ASSERT(false);
    }*/
}


void GameA::calcScaleFactors(){
    this->physics_to_screen_scale_px_m = this->physics_to_ui_scale_in_m*this->ui_to_screen_scale_px_in;
    this->tetris_field_px = SCALE_QRECTF(this->tetris_field_m, this->physics_to_screen_scale_px_m);
    //    printf("physics_to_screen_scale [px/m]: %f\n", this->physics_to_screen_scale_px_m);
}

void GameA::render(QPainter& painter)
{
    painter.setRenderHint(QPainter::Antialiasing);
    
    if (this->freeze_frame){
        painter.drawPixmap(this->ui_field_px.toRect(), this->saved_frames[this->last_frame]);
        return;
    } else if (this->frame_review){
        this->last_frame = (this->last_frame + 1) % NUM_FRAMES_TO_SAVE;
        this->saved_frames[this->last_frame] = QPixmap(this->ui_field_px.size().toSize());
        QPainter sf_painter(&this->saved_frames[this->last_frame]);
        
        this->frame_review = false;
        this->render(sf_painter);
        this->frame_review = true;
        
        sf_painter.end();
    }
    
    if (this->paused){
        painter.drawPixmap(this->ui_field_px.toRect(), this->pause_frame);
        painter.drawPixmap(this->ui_field_px.toRect(), this->pause_overlay);
        return;
    }
    
    // check to see if lines are being cleared right now.
    // cause if they are, then we need to not try and read
    // b2Body objects from the world, since theyre currently being
    // modified which can lead to seg faults.
    if (this->game_state == row_clear_blinking){
        painter.drawPixmap(this->line_clear_freezeframe.rect(), this->line_clear_freezeframe);
        
        if (this->row_blink_on){
            
            painter.setBrush(this->line_clear_color);
            
            for (uint r = 0; r < this->tetris_rows; r++){
                
                if (this->rows_to_clear.at(r)){
                    
                    // This code is used very nearly exactly in clearRows()
                    // this will mark the bottom row
                    row_sides_struct bottom_row(r, this->side_length_m);
                    float32 bottom_y_m = bottom_row.bottom_m;
                    
                    // figure out how many more rows (in a row) need to be cleared
                    // to group them all into one call
                    uint cr;
                    for (cr = r; cr < this->tetris_rows && this->rows_to_clear.at(cr); ++cr){}
                    
                    // last row in contiguous block sees the top Y value
                    row_sides_struct top_row(cr - 1, this->side_length_m);
                    float32 top_y_m = top_row.top_m;
                    
                    painter.drawRect(this->physRectToScrnRect(b2Vec2(0, bottom_y_m), b2Vec2(this->tetris_field_m.width(), top_y_m - bottom_y_m)));
                    
                    r = cr - 1;
                }
            }

            if (this->clear_diag_cut){

                vector<rayCastComplete> rcs = this->getRayCasts(this->diag_top_m, this->diag_bot_m, this->diag_slope);

                float32 x1_m = 0;
                float32 x2_m = this->tetris_field_m.width();

                float32 y1_m = this->diag_slope*(x1_m - rcs.at(BOTTOMLEFT).input.p1.x) + rcs.at(BOTTOMLEFT).input.p1.y;
                float32 y2_m = this->diag_slope*(x1_m - rcs.at(TOPLEFT).input.p1.x) + rcs.at(TOPLEFT).input.p1.y;
                float32 y3_m = this->diag_slope*(x2_m - rcs.at(TOPRIGHT).input.p1.x) + rcs.at(TOPRIGHT).input.p1.y;
                float32 y4_m = this->diag_slope*(x2_m - rcs.at(BOTTOMRIGHT).input.p1.x) + rcs.at(BOTTOMRIGHT).input.p1.y;

                vector<b2Vec2> points_m;
                points_m.push_back(b2Vec2(x1_m, y1_m));
                points_m.push_back(b2Vec2(x1_m, y2_m));
                points_m.push_back(b2Vec2(x2_m, y3_m));
                points_m.push_back(b2Vec2(x2_m, y4_m));

                QList<QPointF> points_px;

                for (uint i = 0; i < points_m.size(); ++i){
                    points_px.append(this->physPtToScrnPt(points_m.at(i)));
                }

                painter.drawPolygon(QPolygonF(points_px));
            }
        }
        
    } else { // NOT clearing lines right now, render as normal.
        
#ifdef TIME_RENDER_STEPS
        QElapsedTimer timer;
        timer.start();
#endif
        
        painter.drawPixmap(this->ui_field_px.toRect(), this->gamebackground);
        
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
            if (this->currentPiece == b) continue;
            if (!this->isAWall(b) /*&& b->IsAwake()*/){
                //printf("Body: (%f, %f)\n", b->GetPosition().x, b->GetPosition().y);
                this->drawTetrisPiece(&painter, b);
            }
            if (debug_box2d){
                this->drawBodyTo(&painter, b);
            }
        }

        //draw currentPiece last
        this->drawTetrisPiece(&painter, this->currentPiece);
        if (debug_box2d){
            this->drawBodyTo(&painter, this->currentPiece);
        }
        
#ifdef TIME_RENDER_STEPS
        printf("Bodies: %lld ms \t", timer.elapsed());
        timer.restart();
#endif
        
        painter.setPen(Qt::NoPen);
        
        for (uint r = 0; r < this->tetris_rows; r++){
            double height_px = this->side_length_dbl_m*this->physics_to_screen_scale_px_m;
            double top_px = height_px*r;
            
            double fill_fraction = static_cast<double>(this->row_areas_m2.at(r)/this->line_clear_threshold);
            //if (fill_fraction > 1.0) printf("r = %u, ff = %f\n", r, fill_fraction);
            fill_fraction = qMin(fill_fraction, 1.0);
            double width_px = fill_fraction*this->row_fill_density_col_width_in*this->ui_to_screen_scale_px_in;
            
            int grey = static_cast<int>((1-fill_fraction)*255);
            painter.setBrush(QColor(grey, grey, grey));
            
            painter.drawRect(QRectF(0, top_px, width_px, height_px));
        }
        
#ifdef TIME_RENDER_STEPS
        printf("Row Densities: %lld ms \t", timer.elapsed());
        timer.restart();
#endif
    }
}

void GameA::colorizeResources(){
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
    
    // to find the right color for the line clear bars, we need to
    // synthesize an image with its color and colorize it, then extract the color obtained.
    QImage color_sample_img(1, 1, QImage::Format_ARGB32);
    color_sample_img.setPixel(0, 0, this->line_clear_color.rgb());
    QPixmap color_sample = QPixmap::fromImage(color_sample_img);
    
    color_sample = this->colorize(color_sample);
    
    QColor new_color = color_sample.toImage().pixelColor(0, 0);
    
    //printf("Before: 0x%08x; after: 0x%08x\n", this->line_clear_color.rgb(), new_color.rgb());
    this->line_clear_color = new_color;
}

void GameA::drawBodyTo(QPainter* painter, b2Body* body){
    
    painter->save();
    QPointF body_center_px = this->physPtToScrnPt(body->GetPosition());
    painter->translate(body_center_px.x(), body_center_px.y());
    
    //https://stackoverflow.com/questions/8881923/how-to-convert-a-pointer-value-to-qstring
    QString ptrStr = QString("0x%1").arg(reinterpret_cast<quintptr>(body),QT_POINTER_SIZE * 2, 16, QChar('0'));
    //QString coordstr = QString("(%1, %2)").arg(body->GetPosition().x).arg(body->GetPosition().y);
    //printf("\t%s: %s\n", ptrStr.toUtf8().constData(), coordstr.toUtf8().constData());

    painter->drawText(QPoint(0, 0), ptrStr);
    
    painter->rotate(static_cast<double>(body->GetAngle())*DEG_PER_RAD);

    tetrisPieceData data = this->getTetrisPieceData(body);
    if (data.is_powerup()){
        painter->save();
        painter->setPen(QColor(Qt::blue));
    }
    
    for (b2Fixture* f = body->GetFixtureList(); f; f = f->GetNext()){
        switch(f->GetType()){
        case b2Shape::e_polygon:{
            b2PolygonShape shape = *static_cast<b2PolygonShape*>(f->GetShape());
            int numpoints = shape.m_count;
            vector<QPointF> points;
            for (int i = 0; i < numpoints; i++){
                points.push_back(
                            QPointF(
                                static_cast<double>(shape.m_vertices[i].x)*this->physics_to_screen_scale_px_m,
                                static_cast<double>(shape.m_vertices[i].y)*this->physics_to_screen_scale_px_m
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
                        static_cast<double>(shape.m_p.x)*this->physics_to_screen_scale_px_m,
                        static_cast<double>(shape.m_p.y)*this->physics_to_screen_scale_px_m
                        );
            double radius = static_cast<double>(shape.m_radius);
            radius *= this->ui_to_screen_scale_px_in;
            painter->drawEllipse(center, radius, radius);
        }
            break;
        case b2Shape::e_edge:{
            b2EdgeShape shape = *static_cast<b2EdgeShape*>(f->GetShape());
            QPointF p1 = QPointF(
                        static_cast<double>(shape.m_vertex1.x)*this->physics_to_screen_scale_px_m,
                        static_cast<double>(shape.m_vertex1.y)*this->physics_to_screen_scale_px_m
                        );
            QPointF p2 = QPointF(
                        static_cast<double>(shape.m_vertex2.x)*this->physics_to_screen_scale_px_m,
                        static_cast<double>(shape.m_vertex2.y)*this->physics_to_screen_scale_px_m
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
                            static_cast<double>(shape.m_vertices[i].x)*this->physics_to_screen_scale_px_m,
                            static_cast<double>(shape.m_vertices[i].y)*this->physics_to_screen_scale_px_m
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

    b2MassData mdata;
    body->GetMassData(&mdata);
    b2Vec2 com_m = body->GetLocalCenter();
    painter->drawEllipse(QPointF(com_m.x, com_m.y)*this->physics_to_screen_scale_px_m, 10, 5);

    if (data.is_powerup()){
        painter->restore();
    }
    painter->restore();
}

void GameA::drawTetrisPiece(QPainter* painter, b2Body* piece_body){
    
    painter->save();

    QPointF body_center_px = this->physPtToScrnPt(piece_body->GetPosition());
    painter->translate(body_center_px.x(), body_center_px.y());

    painter->scale(this->physics_to_screen_scale_px_m, this->physics_to_screen_scale_px_m);
    painter->rotate(static_cast<double>(piece_body->GetAngle())*DEG_PER_RAD);
    
    tetrisPieceData body_data = this->getTetrisPieceData(piece_body);
    painter->drawPixmap(body_data.region_m.toRect(), body_data.image);

    painter->restore();

    if (body_data.powerup == DIAG_CUT){

        painter->save();
        painter->setPen(Qt::black);

        float32 dist_inc_m = 1;
        float32 next_x_m = 0;

        float32 slope = tan(piece_body->GetAngle());

        b2Vec2 p1_m, next_pt_m;
        float32 next_y_m;
        QList<QPointF> points;

        bool edge_reached = false;
        bool cancel_line = false;
        while(!edge_reached){

            if (next_x_m > this->tetris_field_m.width()){
                next_x_m = this->tetris_field_m.width();
                edge_reached = true;
            }

            next_y_m = slope*(next_x_m - piece_body->GetPosition().x) + piece_body->GetPosition().y;
            if (next_y_m < -5*this->tetris_field_m.height() || next_y_m > 5*this->tetris_field_m.height()){
                cancel_line = true;
                break;
            }
            next_pt_m.Set(next_x_m, next_y_m);

            points.push_back(this->physPtToScrnPt(next_pt_m));

            next_x_m += dist_inc_m/sqrt(slope*slope + 1);
        }

        if (!cancel_line) painter->drawLines(points);

        painter->restore();
    }
}

void GameA::drawScore(QPainter* painter){
    
    this->BOW_font.print(painter, this->score_display_right_in*this->ui_to_screen_scale_px_in, RIGHT_ALIGN,
                         QString::number(this->current_score), this->ui_to_screen_scale_px_in);
    
    if (this->score_to_add > 0){
        
        QPixmap score_add_pm(this->score_add_display_in.size()*this->ui_to_screen_scale_px_in);
        score_add_pm.fill(Qt::black);
        
        QPainter score_add_painter(&score_add_pm);
        
        score_add_painter.translate(QPoint(0, this->score_add_disp_offset_in));
        
        this->WOB_font.print(&score_add_painter, this->sc_add_right_in_disp_in*this->ui_to_screen_scale_px_in, RIGHT_ALIGN,
                             "+" + QString::number(this->score_to_add), this->ui_to_screen_scale_px_in);
        score_add_painter.end();
        
        painter->save();
        painter->translate(this->score_add_display_in.topLeft()*this->ui_to_screen_scale_px_in);
        painter->drawPixmap(score_add_pm.rect(), score_add_pm);
        painter->restore();
    }
    
    this->BOW_font.print(painter, this->level_disp_offset_in*this->ui_to_screen_scale_px_in, RIGHT_ALIGN,
                         QString::number(this->current_level), this->ui_to_screen_scale_px_in);
    
    this->BOW_font.print(painter, this->lines_cleared_disp_offset_in*this->ui_to_screen_scale_px_in, RIGHT_ALIGN,
                         QString::number(this->lines_cleared), this->ui_to_screen_scale_px_in);
}


void GameA::keyPressEvent(QKeyEvent* ev){
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

        if (!this->paused && this->game_state == flush_blocks){
            this->skip_falling = true;
            return;
        } else if (!this->paused){ // NOT paused, about to pause

            this->pause_frame = QPixmap(this->ui_field_px.size().toSize());
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

void GameA::keyReleaseEvent(QKeyEvent* ev){
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

void GameA::doGameStep(){
    
    if (this->freeze_frame) return;
    if (this->paused) return;

    QPainter tmp_painter;

    switch(this->game_state){
    case start_row_clear_blinking: //If the blinks just started
        //render the current frame onto a pixmap to use later during line blinking
        this->line_clear_freezeframe = QPixmap(this->ui_field_px.size().toSize());
        tmp_painter.begin(&this->line_clear_freezeframe);
        this->render(tmp_painter);
        tmp_painter.end();

        // ok now set it to what it should be
        this->game_state = row_clear_blinking;

        //check and clear any rows that need be cleared (in another thread, resolved later)
        this->line_clearing_thread = QtConcurrent::run(&GameA::clearRows, this);

        this->currentPiece->SetLinearVelocity(b2Vec2(0, 0));

        //dont break, continue to row_clear_blinking state

    case row_clear_blinking:

        this->row_blink_accumulator_s += framerate_s_f;
        if (this->row_blink_accumulator_s > this->lc_blink_toggle_time_s){ //If its time to toggle
            this->row_blink_accumulator_s = 0;

            this->row_blink_on = !this->row_blink_on;
            if (!this->row_blink_on){ //if falling edge on row blink

                ++this->num_blinks_so_far;
                if (this->num_blinks_so_far >= this->num_blinks){ //if we've reached the number of prescribed blinks
                    this->num_blinks_so_far = 0;

                    //reset row clear vector so graphics doesnt keep drawing it
                    for (uint r = 0; r < this->tetris_rows; r++){
                        if (this->rows_to_clear.at(r)){
                            this->rows_to_clear.at(r) = false;
                        }
                    }
                    this->clear_diag_cut = false;

                    // block until line clearing thread is done
                    //                    QElapsedTimer wff_timer;
                    //                    wff_timer.start();

                    this->line_clearing_thread.waitForFinished();

                    //                    printf("Waited %lld ms for thread to return.\n", wff_timer.elapsed());
                    //                    fflush(stdout);

                    // Convert queued QImages to QPixmaps (since this step is not allowed
                    // inside a QtConcurrent thread) and store them
                    for (b2Body* b = this->world->GetBodyList(); b; b = b->GetNext()){
                        tetrisPieceData tpd = this->getTetrisPieceData(b);
                        tpd.resolveImage();
                        this->setTetrisPieceData(b, tpd);
                    }

                    if (this->shake_field){
                        this->game_state = shake_field_state;
                        this->shake_field = false;
                    } else {
                        this->game_state = gameA;
                    }

                    this->makeNewTetrisPiece(this->new_level_reached);

                    if (this->new_level_reached){
                        this->sfx[NEW_LEVEL].play();
                        this->new_level_reached = false;
                    }

                    this->init_BDC();
                }
            }
        }

        break;

    case shake_field_state:
    {
        this->shake_time_acc_s += framerate_s_f;

        float32 t_mult = 2.0*M_PI*3.0;
        float32 side_speed = (t_mult/2.0)*sin(t_mult*this->shake_time_acc_s);
        b2Vec2 v(side_speed, 0);

        if (this->shake_time_acc_s > this->shake_time_max_s){
            this->game_state = gameA;
            v.Set(0, 0);

            this->walls[LEFTWALL]->SetTransform(b2Vec2(0, 0), this->walls[LEFTWALL]->GetAngle());
            this->walls[RIGHTWALL]->SetTransform(b2Vec2(0, 0), this->walls[RIGHTWALL]->GetAngle());
            this->walls[GROUND]->SetTransform(b2Vec2(0, 0), this->walls[GROUND]->GetAngle());

            this->shake_time_acc_s = 0;
        }

        this->walls[LEFTWALL]->SetLinearVelocity(v);
        this->walls[RIGHTWALL]->SetLinearVelocity(v);
        this->walls[GROUND]->SetLinearVelocity(v);
    }
        break;

    case flush_blocks:

        if (static_cast<double>(this->currentPiece->GetWorldCenter().y) > this->tetris_field_m.height()
                || this->skip_falling){

            printf("Skip falling: %d\n", this->skip_falling);

            // Copy score to window object for reference by future screens
            ((NT3Window*)(this->parent()))->gameA_score = this->current_score;

            //printf("Re-rendering frame for gameover screen...\n");

            //render the current frame onto a pixmap to use later during game over screen
            QPixmap lastframe(this->ui_field_px.size().toSize());
            tmp_painter.begin(&lastframe);
            this->render(tmp_painter);
            tmp_painter.end();

            ((NT3Window*)(this->parent()))->game_lastframe = lastframe;

            emit this->stateEnd(GAME_LOST);
        }

        break;

    case gameA:

        if (this->contactlistener->hasCurrentPieceCollided()){
            this->currentPiece->SetGravityScale(1);

            if (this->currentPiece->GetWorldCenter().y < 0){

                printf("Game lost!\n");
                this->game_state = flush_blocks;

                this->world->DestroyBody(this->walls[GROUND]);

                emit this->changeMusic(QUrl());
                this->sfx[GAME_OVER_SOUND].play();

                break;
            }

            //prepare powerup stuff
            switch(this->getTetrisPieceData(this->currentPiece).powerup){
            case DIAG_CUT:
            {
                this->clear_diag_cut = true;
                float32 angle_rad = this->currentPiece->GetAngle();

                b2Vec2 bp_m = this->currentPiece->GetWorldCenter();

                this->diag_slope = static_cast<float32>(tan(static_cast<double>(angle_rad)));

                //adjust so that if a line were to be drawn perpendicular to the two cut lines,
                //the distance between the two intersection points would be equal to the side length of a block.
                //work:
                //f1(x) = m*x
                //f2(x) = m*x + b <-- b is the answer we're solving for
                //f3(x) = tan(arctan(m) + pi/2)*x <-- the perpendicular line
                //A = intersection of f1 and f3 = (0, 0)
                //B = inxion of f2 and f3 = (-b*m/(m^2+1), b/(m^2+1))
                //dist(A, B) = side length = sl
                //dist(A, B) = sqrt(B.x^2 + B.y^2) = abs(b)/sqrt(m^2+1)
                //assume that b will always be positive, remove the abs
                //b/sqrt(m^2+1) = sl
                //b = sl*sqrt(m^2+1)
                float32 y_offset_m = this->side_length_m*static_cast<float32>(sqrt(static_cast<double>(this->diag_slope*this->diag_slope + 1)));
                this->diag_top_m = bp_m.y + y_offset_m/2 - this->diag_slope*(bp_m.x - this->raycast_left_m);
                this->diag_bot_m = bp_m.y - y_offset_m/2 - this->diag_slope*(bp_m.x - this->raycast_left_m);

                if (isnan(this->diag_slope) || isinf(this->diag_slope) ||
                        this->diag_top_m < -5*this->tetris_field_m.height() || this->diag_top_m > 5*this->tetris_field_m.height()){ //TODO: standardize this threshold
                    this->clear_diag_cut = false;
                    fprintf(stderr, "Diagonal cut not performed, powerup piece landed nearly stright up!\n");
                    //TODO: maybe play a little error sound here
                }
            }
                break;
            case EARTHQUAKE:

                this->shake_field = true;

                break;
            case NOT_A_POWERUP:
                //do nothing
                break;
            default:
                fprintf(stderr, "powerup enum not defined: %d\n", this->getTetrisPieceData(this->currentPiece).powerup);
                break;
            }

            //TODO: destroy current piece?

            float32 average_area_m2 = 0;
            int num_lines_removed = 0;

            for (uint r = 0; r < this->tetris_rows; r++){
                if (this->row_areas_m2.at(r) > this->line_clear_threshold){
                    this->rows_to_clear.at(r) = true;
                    ++num_lines_removed;
                    average_area_m2 += this->row_areas_m2.at(r);
                }
            }

            if (num_lines_removed > 0 || this->clear_diag_cut){

                for (b2Body* b = this->world->GetBodyList(); b; b = b->GetNext()){
                    if (b == this->next_piece_for_display) continue;
                    b->SetLinearVelocity(b2Vec2(0, 0));
                    b->SetAngularVelocity(0);
                }

                this->game_state = start_row_clear_blinking;

                this->lines_cleared += num_lines_removed;
                if (this->lines_cleared/this->lines_per_level > this->current_level){
                    this->current_level = this->lines_cleared/this->lines_per_level;
                    this->tetrisBodyDef.linearVelocity =
                            b2Vec2(
                                0,
                                this->downward_velocity_regular_m_s +
                                this->downward_velocity_level_increment_m_s*this->current_level
                                );
                    this->new_level_reached = true;
                }

                average_area_m2 /= num_lines_removed*this->avgarea_divisor;

                //this equation is:
                //(where n = num_lines_removed)

                //ceil( ( (3*n)^(average_area^10) )*20 + (n^2)*40 )

                //(average_area^10) becomes very small typically, ~10^-19
                //for the usual case of average area, the scores look something like this:
                //n = 1: score += 60
                //n = 2: score += 180
                //n = 3: score += 380
                this->score_to_add = qCeil(qPow((num_lines_removed*3), qPow(static_cast<double>(average_area_m2), 10.0))*20 +
                                           qPow(num_lines_removed, 2)*40);

                this->current_score += this->score_to_add;

                this->score_add_disp_offset_in = 0;

                if (num_lines_removed > 3){
                    this->sfx[FOUR_LINE_CLEAR].play();
                } else {
                    this->sfx[LINE_CLEAR].play();
                }

            } else {
                //not clearing any lines, so carry on

                if (this->shake_field){
                    this->game_state = shake_field_state;
                    this->shake_field = false;
                }

                this->makeNewTetrisPiece(false);
                this->sfx[BLOCK_FALL].play();

            }
        }

    { //process controls
        float32 inertia = this->currentPiece->GetInertia();

        switch(this->rotateState){
        case NO_ROTATION:
        case BOTH_ROTATIONS:
            //do nothing
            //printf("Dont rotate\n");
            break;
        case ROTATECW:
            //printf("Rotate CW\n");
            if (this->currentPiece->GetAngularVelocity() < this->wmax_rad_s){
                this->currentPiece->ApplyTorque(this->angular_accel_rad_s2*inertia, true);
            }
            break;
        case ROTATECCW:
            //printf("Rotate CCW\n");
            if (this->currentPiece->GetAngularVelocity() > -this->wmax_rad_s){
                this->currentPiece->ApplyTorque(-this->angular_accel_rad_s2*inertia, true);
            }
            break;
        default:
            fprintf(stderr, "Invalid Rotation state\n");
            break;
        }

        float32 mass_kg = this->currentPiece->GetMass();
        b2Vec2 linear_force_vect_N = b2Vec2(0, 0);

        switch(this->lateralMovementState){
        case NO_LATERAL_MOVEMENT:
        case BOTH_DIRECTIONS:
            //do nothing
            break;
        case MOVELEFT:
            linear_force_vect_N.x = -this->lateral_accel_m_s2*mass_kg;
            break;
        case MOVERIGHT:
            linear_force_vect_N.x = this->lateral_accel_m_s2*mass_kg;
            break;
        default:
            fprintf(stderr, "Invalid Lateral Movement state\n");
            break;
        }

        float32 y_velocity_m_s = this->currentPiece->GetLinearVelocity().y;
        float32 downward_velocity_adjusted_m_s =
                this->downward_velocity_regular_m_s +
                this->downward_velocity_level_increment_m_s*this->current_level;

        if (!this->accelDownState && qAbs(y_velocity_m_s - downward_velocity_adjusted_m_s) <= 1){
            linear_force_vect_N.y = 0;
            this->currentPiece->SetLinearVelocity(b2Vec2(this->currentPiece->GetLinearVelocity().x, downward_velocity_adjusted_m_s));
        } else if (this->accelDownState || y_velocity_m_s < downward_velocity_adjusted_m_s){
            //printf("forcing downwards\n");
            linear_force_vect_N.y = this->downward_accel_m_s2*mass_kg;
        } else {
            //printf("slowing down...\n");
            linear_force_vect_N.y = -this->upward_correcting_accel_m_s2*mass_kg;
        }
        this->currentPiece->ApplyForce(linear_force_vect_N, this->currentPiece->GetWorldCenter(), true);
    }


        break;

    default:
        fprintf(stderr, "Game state not defined: %d\n", this->game_state);
        break;
    }

    if (this->game_state == gameA || this->game_state == shake_field_state || this->game_state == flush_blocks){
        //step world

#ifdef TIME_GAME_FRAME
        QElapsedTimer timer;
        timer.start();
#endif

        this->world->Step(this->timeStep_s, this->velocityIterations, this->positionIterations);

#ifdef TIME_GAME_FRAME
        printf("World step: %lld ms,\t", timer.elapsed());
        timer.restart();
#endif

        if (this->score_to_add > 0){
            if (--this->score_add_disp_offset_in < -10*this->ui_to_screen_scale_px_in){
                this->score_to_add = 0;
            }
        }
        //printf("Score to add: %d; Offset: %d\n", this->score_to_add, this->score_add_disp_offset);

#ifdef TIME_GAME_FRAME
        printf("Currpiece math: %lld ms\t", timer.elapsed());
        timer.restart();
#endif

        for (uint r = 0; r < this->tetris_rows; r++){
            this->row_areas_m2.at(r) = this->getRowArea_m2(r);
        }

#ifdef TIME_GAME_FRAME
        printf("Row density: %lld ms\t", timer.elapsed());
        timer.restart();
#endif
    }
}


void GameA::clearRows(){
    for (uint r = 0; r < this->tetris_rows; r++){
        if (this->rows_to_clear.at(r)){
            
            // this will mark the bottom row
            row_sides_struct bottom_row(r, this->side_length_m);
            float32 bottom_y = bottom_row.bottom_m;
            
            // figure out how many more rows (in a row) need to be cleared
            // to group them all into one call
            uint cr;
            for (cr = r; cr < this->tetris_rows && this->rows_to_clear.at(cr); ++cr){}
            
            // last row in contiguous block sees the top Y value
            row_sides_struct top_row(cr - 1, this->side_length_m);
            float32 top_y = top_row.top_m;
            
            // clear it
            this->clearYRange(top_y, bottom_y);
            
            // skip to the next row after these and continue
            r = cr - 1;
        }
    }

    if (this->clear_diag_cut){
        this->clearDiagRange(this->diag_top_m, this->diag_bot_m, this->diag_slope);
    }

    for (b2Body* b : this->bodies_to_destroy){
        this->destroyTetrisPiece(b);
    }
    this->bodies_to_destroy.clear();
}

float32 GameA::getRowArea_m2(uint row){
    
    row_sides_struct sides(row, this->side_length_m);
    
    vector<rayCastComplete> ray_casts = this->getRayCasts(sides.top_m, sides.bottom_m, 0);
    
    float32 total_area_m2 = 0;
    
    for (b2Body* b = this->world->GetBodyList(); b; b = b->GetNext()){
        if (this->isAWall(b)) continue;
        if (b == this->currentPiece) continue;
        if (b == this->next_piece_for_display) continue;
        
        if (this->body_area_contributions_m2.at(row).contains(b)){
            if (!b->IsAwake()){
                total_area_m2 += this->body_area_contributions_m2.at(row).value(b);
                continue;
            }
            this->body_area_contributions_m2.at(row).remove(b);
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
                    if (p.y <= sides.top_m && p.y >= sides.bottom_m){
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
                float32 area = this->poly_area_m2(&new_points[0], num_vertices);
                body_area += area;
                
            } else { //If NEITHER of the ray casts hit
                
                b2Vec2 p = b->GetWorldPoint(s->m_vertices[0]);
                if (p.y <= sides.top_m && p.y >= sides.bottom_m){
                    float32 area = this->poly_area_m2(s->m_vertices, s->m_count);
                    body_area += area;
                }
                
            } //end neither ray cast hit
            
        } //end fixture loop
        
        this->body_area_contributions_m2.at(row).insert(b, body_area);
        total_area_m2 += body_area;
        
    } //end body loop
    
    //Q_ASSERT(total_area/this->side_length <= this->tetris_field.width());
    return total_area_m2;
}

void GameA::clearDiagRange(float32 top_y_m, float32 bottom_y_m, float32 slope){

    vector<rayCastComplete> ray_casts = this->getRayCasts(top_y_m, bottom_y_m, slope);

    //adjust top and bottom Y values to be where the cut sides intersect with the edge of the field
    top_y_m = top_y_m - slope*raycast_left_m;
    bottom_y_m = bottom_y_m - slope*raycast_left_m;

    //make list of bodies affected by this row clear
    //for top and bottom lines:
    vector<b2Body*> affected_bodies;

    //find all bodies with shapes that cross the line or are inside the lines
    for (b2Body* b = this->world->GetBodyList(); b; b = b->GetNext()){
        if (this->isAWall(b)) continue;
        if (this->next_piece_for_display == b) continue;

        //if this body is already marked for deletion, don't bother.
        bool todestroy = false;
        for (b2Body* btd : this->bodies_to_destroy){
            if (b == btd){
                todestroy = true;
                break;
            }
        }
        if (todestroy) continue;

        bool affected = false;
        for (b2Fixture* f = b->GetFixtureList(); f; f = f->GetNext()){
            Q_ASSERT(f->GetShape()->GetType() == b2Shape::e_polygon);
            b2PolygonShape* s = static_cast<b2PolygonShape*>(f->GetShape());

            float32 v0_worldpoint_y = b->GetWorldPoint(s->m_vertices[0]).y;
            float32 v0_wp_y_intersect = v0_worldpoint_y - slope*(b->GetWorldPoint(s->m_vertices[0]).x);
            if (v0_wp_y_intersect >= bottom_y_m && v0_wp_y_intersect <= top_y_m){
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
                    float32 py_intersect = p.y - slope*p.x;
                    bool outside;
                    switch(side){
                    case TOP:
                        outside = py_intersect > top_y_m;
                        break;
                    case BOTTOM:
                        outside = py_intersect < bottom_y_m;
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
                if (!(this->poly_area_m2(&new_points[0], new_count) > 0)){
                    //printf("Portion of shape outside line cut was too small, discarding\n");
                    continue;
                }

                //printf("Portion of shape outside line cut IS valid\n");

                //if they ARE valid:
                //make new shape with new points
                b2PolygonShape new_shape;
                new_shape.Set(&new_points[0], new_count);
                shapes_to_make.push_back(new_shape);
            }//end loop though both sides

            //remove original shape (later)
            fixtures_to_destroy.push_back(f);

            //add it to the current body (later)


        } //end shape cutting loop


        for (b2Fixture* f : fixtures_to_destroy){
            b->DestroyFixture(f);
        }

        b2FixtureDef f_def = this->tetrisFixtures.at(0).at(0);
        for (b2PolygonShape& s : shapes_to_make){
            /*printf("Adding shape to new body:\n");
            for (uint i = 0; i < s.m_count; i++){
                printf("\t%s\n", this->b2Vec2String(b->GetWorldPoint(s.m_vertices[i])).toUtf8().constData());
            }*/
            f_def.shape = &s;
            b->CreateFixture(&f_def);
        }

    }//end body reshape loop


    //(Now all bodies have been cut, but are still one rigid body each. they need to be split)
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

        for (vector<b2PolygonShape*>& group : shape_groups){

            //make new body
            b2Body* new_body = this->world->CreateBody(&new_body_def);

            b2FixtureDef fixture_def = this->tetrisFixtures.at(0).at(0);

            //add shapes in svector to body
            for (b2PolygonShape* s : group){
                fixture_def.shape = s;
                new_body->CreateFixture(&fixture_def);
            }

            tetrisPieceData data = this->getTetrisPieceData(b);

            QImage tomask = data.get_image();

            QImage masked = this->maskImage(new_body, tomask, data.region_m);

            Q_ASSERT(masked.hasAlphaChannel());

            data.image_in_waiting = masked;

            this->setTetrisPieceData(new_body, data);
        }

        //delete original body (later)
        this->bodies_to_destroy.push_back(b);

    } //end body separation loop
}

void GameA::clearYRange(float32 top_y_m, float32 bottom_y_m){
    this->clearDiagRange(top_y_m, bottom_y_m, 0);
}

QImage GameA::maskImage(b2Body* b, QImage orig_image, QRectF region_m){
    
    Q_ASSERT(orig_image.hasAlphaChannel());
    
    float32 scale = 1.0f/static_cast<float32>(this->piece_image_scale*this->physics_to_ui_scale_in_m);
    
    b2Vec2 offset_m(region_m.x(), region_m.y());
    
    b2Transform t;
    t.SetIdentity();
    
    int width = orig_image.width();
    int height = orig_image.height();
    
    QImage ans = QImage(orig_image.size(), orig_image.format());
    ans.fill(Qt::transparent);
    
    QRgb* orig_pixels = reinterpret_cast<QRgb*>(orig_image.bits());
    QRgb* anspixels = reinterpret_cast<QRgb*>(ans.bits());
    
    //this line will verify that the body hasnt been removed from the world
    //if (!this->userData.contains(b)) return ans;
    
    for (b2Fixture* f = b->GetFixtureList(); f; f = f->GetNext()){
        Q_ASSERT(f->GetShape()->GetType() == b2Shape::e_polygon);
        b2PolygonShape* s = static_cast<b2PolygonShape*>(f->GetShape());
        
        b2AABB aabb;
        s->ComputeAABB(&aabb, t, 0);
        
        //calculate start and end pixel indices based on the AABB calculated, using the rect offset and scale
        int startx_px = qFloor(static_cast<double>((aabb.lowerBound.x - offset_m.x)/scale));
        int endx_px = qCeil(static_cast<double>((aabb.upperBound.x - offset_m.x)/scale));
        
        int starty_px = qFloor(static_cast<double>((aabb.lowerBound.y - offset_m.y)/scale));
        int endy_px = qCeil(static_cast<double>((aabb.upperBound.y - offset_m.y)/scale));
        
        //make sure nothing runs out of image bounds
        startx_px = qMax(0, startx_px);
        endx_px = qMin(width - 1, endx_px);
        
        starty_px = qMax(0, starty_px);
        endy_px = qMin(height - 1, endy_px);
        
        for (int y = starty_px; y < endy_px; y++){
            for (int x = startx_px; x < endx_px; x++){
                
                int pix_index = x + width*y;
                if (QColor(orig_pixels[pix_index]).alpha() == 0){
                    //printf("Alpha was already 0 at (%d, %d)\n", x, y);
                    continue;
                }
                
                b2Vec2 center(x, y);
                center *= scale;
                center += offset_m;
                
                if (this->TestPointRadius(s, t, center)){
                    //printf("Test point pass at (%d, %d)\n", x, y);
                    anspixels[pix_index] = orig_pixels[pix_index];
                }/* else { // only shows the rects being discriminated
                    anspixels[pix_index] = QColor(255, 255, 0).rgb();
                }*/
            }
        }
        //this line will verify that the body hasnt been removed from the world (pt. 2)
        //if (!this->userData.contains(b)) return ans;
    }
    Q_ASSERT(ans.hasAlphaChannel());
    
    return ans;
}

//modified from b2PolygonShape::TestPoint, this will return true if the point falls within the radius of the polygon
bool GameA::TestPointRadius(b2PolygonShape* s, const b2Transform& xf, const b2Vec2& p) const{
    b2Vec2 pLocal = b2MulT(xf.q, p - xf.p);
    
    for (int32 i = 0; i < s->m_count; ++i){
        float32 dot = b2Dot(s->m_normals[i], pLocal - s->m_vertices[i]);
        if (dot > b2_polygonRadius){
            return false;
        }
    }
    return true;
}

vector<rayCastComplete> GameA::getRayCasts(float32 top, float32 bot, float32 slope){
    vector<rayCastComplete> ray_casts;

    float32 left_top = top;
    float32 left_bot = bot;
    float32 right_top = slope*(this->raycast_right_m - this->raycast_left_m) + left_top;
    float32 right_bot = slope*(this->raycast_right_m - this->raycast_left_m) + left_bot;

    //    if (angle_rad != 0.0){
    //        printf("Ray casts:\n%f\t%f\n%f\t%f\n", left_top, right_top, left_bot, right_bot);
    //        fflush(stdout);
    //    }
    
    for (uint8 r = 0; r < num_ray_casts; r++){
        rayCastComplete ray_cast;
        ray_casts.push_back(ray_cast);
        ray_casts.at(r).input.maxFraction = 1;
        switch(r){
        case TOPLEFT:
            ray_casts.at(r).input.p1.Set(this->raycast_left_m, left_top);
            ray_casts.at(r).input.p2.Set(this->raycast_right_m, right_top);
            break;
        case TOPRIGHT:
            ray_casts.at(r).input.p1.Set(this->raycast_right_m, right_top);
            ray_casts.at(r).input.p2.Set(this->raycast_left_m, left_top);
            break;
        case BOTTOMLEFT:
            ray_casts.at(r).input.p1.Set(this->raycast_left_m, left_bot);
            ray_casts.at(r).input.p2.Set(this->raycast_right_m, right_bot);
            break;
        case BOTTOMRIGHT:
            ray_casts.at(r).input.p1.Set(this->raycast_right_m, right_bot);
            ray_casts.at(r).input.p2.Set(this->raycast_left_m, left_bot);
            break;
        default:
            fprintf(stderr, "Ray cast enum not defined\n");
            break;
        } //end ray cast switch statement
    } //end ray cast init
    
    return ray_casts;
}

b2Vec2 GameA::hit_point(rayCastComplete ray_cast){
    Q_ASSERT(ray_cast.hit);
    return ray_cast.input.p1 + ray_cast.output.fraction * (ray_cast.input.p2 - ray_cast.input.p1);
}

//This function is code modified directly from b2PolygonShape::Set() and b2PolygonShape::ComputeCentroid()
//so that it returns 0 on error whereas the original function fails an assert, crashing the program.
float32 GameA::poly_area_m2(b2Vec2* vertices, int count){
    
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
    if (area > this->min_poly_area_m2){
        return area;
    }
    //printf("area <= %f\n", b2_epsilon);
    return 0;
}


void GameA::makeNewTetrisPiece(bool is_powerup){
    
    //set up current piece
    tetris_piece_enum type = this->next_piece_type;
    
    this->currentPiece = world->CreateBody(&this->tetrisBodyDef);
    this->contactlistener->currentPiece = this->currentPiece;
    
    for (b2FixtureDef f : this->tetrisFixtures.at(type)){
        this->currentPiece->CreateFixture(&f);
    }

    powerup_type_enum powerup = NOT_A_POWERUP;
    
    QPixmap piece_img;
    if (is_powerup){
        piece_img = this->pwu_piece_images.at(type);
        //ensure the result of this is not 0 (NOT_A_POWERUP)
        //powerup = static_cast<powerup_type_enum>(this->rng.bounded(num_powerup_types - 1) + 1); //TODO: make powerups random again
        powerup = EARTHQUAKE;
    } else {
        piece_img = this->piece_images.at(type);
    }
    tetrisPieceData data(piece_img, this->piece_rects_m.at(type), powerup);
    this->setTetrisPieceData(this->currentPiece, data);

    this->makeNewNextPiece();
}

void GameA::makeNewNextPiece(){
    this->next_piece_type = static_cast<tetris_piece_enum>(this->rng.bounded(num_tetris_pieces));
    
    this->destroyTetrisPiece(this->next_piece_for_display);
    
    this->next_piece_bodydef.position = b2Vec2(
                static_cast<float32>(
                    this->next_piece_display_center_in.x()*1.0/this->physics_to_ui_scale_in_m -
                    this->tetris_field_m.x()
                    ),
                static_cast<float32>(
                    this->next_piece_display_center_in.y()*1.0/this->physics_to_ui_scale_in_m -
                    this->tetris_field_m.y()
                    )
                ) - this->center_of_mass_offsets.at(this->next_piece_type);
    this->next_piece_bodydef.angularVelocity = this->next_piece_w_rad_s;
    
    this->next_piece_for_display = this->world->CreateBody(&this->next_piece_bodydef);
    
    for (b2FixtureDef f : this->tetrisFixtures.at(this->next_piece_type)){
        this->next_piece_for_display->CreateFixture(&f);
    }
    
    tetrisPieceData data(this->piece_images.at(this->next_piece_type), this->piece_rects_m.at(this->next_piece_type), NOT_A_POWERUP);
    this->setTetrisPieceData(this->next_piece_for_display, data);

    this->next_piece_for_display->SetLinearVelocity(b2Vec2(0, 0));
    this->next_piece_for_display->SetAngularVelocity(this->next_piece_w_rad_s);
}


bool GameA::isAWall(b2Body* b){
    for (uint i = 0; i < num_walls; i++){
        if (b == this->walls[i]){
            return true;
        }
    }
    return false;
}

QString GameA::b2Vec2String(b2Vec2 vec){
    return QString("(%1, %2)").arg(static_cast<double>(vec.x)).arg(static_cast<double>(vec.y));
}

tetrisPieceData GameA::getTetrisPieceData(b2Body* b){
    
    tetrisPieceData ans = this->userData.value(b, this->default_data);
    
    /*if (ans == this->default_data && !this->isAWall(b)){ //VERY SLOW
        printf("%p has no data!!\n", (void*)b);
    }*/
    
    return ans;
    
    /*void* data = b->GetUserData();
    if (data == nullptr) return nullptr;
    
    return static_cast<tetrisPieceData*>(data);*/
}

void GameA::setTetrisPieceData(b2Body* b, tetrisPieceData tpd){
    this->userData.remove(b);
    this->userData.insert(b, tpd);
}

QPixmap GameA::enableAlphaChannel(QPixmap pixmap){
    if (pixmap.hasAlphaChannel()) return pixmap;
    
    QPixmap ans(pixmap.size());
    ans.fill(Qt::transparent);
    
    QPainter p(&ans);
    p.drawPixmap(ans.rect(), pixmap);
    p.end();
    
    return ans;
}

void GameA::destroyTetrisPiece(b2Body* b){
    if (!b) return;
    this->freeUserDataOn(b);
    this->world->DestroyBody(b);
}

QPointF GameA::physPtToScrnPt(b2Vec2 worldPoint_m){
    //take a point in meters from the playable tetris field (where (0, 0) is the top left corner of where the blocks can be)
    //and convert it to a point in pixels in the game window
    return QPointF(
                this->tetris_field_px.x() + static_cast<double>(worldPoint_m.x)*this->physics_to_screen_scale_px_m,
                this->tetris_field_px.y() + static_cast<double>(worldPoint_m.y)*this->physics_to_screen_scale_px_m
                );
}

QRectF GameA::physRectToScrnRect(b2Vec2 topLeft_m, b2Vec2 size_m){
    QSizeF size_px;
    size_px.setWidth(size_m.x*this->physics_to_screen_scale_px_m);
    size_px.setHeight(size_m.y*this->physics_to_screen_scale_px_m);
    return QRectF(this->physPtToScrnPt(topLeft_m), size_px);
}

b2Vec2 GameA::scrnPtToPhysPt(QPointF screenPoint_px){
    //take a point in pixels from the game window
    //and convert it to a point in meters in the playable tetris field (where (0, 0) is the top left corner of where the blocks can be)
    return b2Vec2(
                (screenPoint_px.x() - this->tetris_field_px.x())/this->physics_to_screen_scale_px_m,
                (screenPoint_px.y() - this->tetris_field_px.y())/this->physics_to_screen_scale_px_m
                );
}


void GameA::initializeTetrisPieceDefs(){
    
    this->tetrisBodyDef.type = b2_dynamicBody;
    
    this->tetrisBodyDef.allowSleep = true;
    this->tetrisBodyDef.awake = true;
    
    this->tetrisBodyDef.bullet = false;
    
    this->tetrisBodyDef.position = this->piece_start_m;
    
    this->tetrisBodyDef.gravityScale = 0;
    
    this->tetrisBodyDef.linearVelocity = b2Vec2(0, this->downward_velocity_regular_m_s);
    this->tetrisBodyDef.angularVelocity = 0;
    
    this->tetrisBodyDef.linearDamping = this->linear_damping_1_s;
    this->tetrisBodyDef.angularDamping = this->angular_damping_1_s;
    
    
    this->next_piece_bodydef = this->tetrisBodyDef;
    this->next_piece_bodydef.position = b2Vec2(
                static_cast<float32>(
                    this->next_piece_display_center_in.x()*1.0/this->physics_to_ui_scale_in_m -
                    this->tetris_field_m.x()
                    ),
                static_cast<float32>(
                    this->next_piece_display_center_in.y()*1.0/this->physics_to_ui_scale_in_m -
                    this->tetris_field_m.y()
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
    
    float32 half_length = this->side_length_m/2;
    
    this->tetrisShapes.at(I).push_back(shape_template);
    this->tetrisShapes.at(I).at(0).SetAsBox(
                this->side_length_m*2 - b2_polygonRadius,
                half_length - b2_polygonRadius
                );
    
    this->tetrisShapes.at(O).push_back(shape_template);
    this->tetrisShapes.at(O).at(0).SetAsBox(
                this->side_length_m - b2_polygonRadius,
                this->side_length_m - b2_polygonRadius
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
                b2Vec2(this->side_length_m, this->side_length_m - b2_polygonRadius),
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
                b2Vec2(-this->side_length_m, this->side_length_m - b2_polygonRadius),
                0
                );
    
    this->tetrisShapes.at(Z).push_back(shape_template);
    this->tetrisShapes.at(Z).push_back(shape_template);
    this->tetrisShapes.at(Z).at(0).SetAsBox(
                this->side_length_m - b2_polygonRadius,
                half_length - 0.5f*b2_polygonRadius,
                b2Vec2(-half_length, -half_length + b2_polygonRadius),
                0
                );
    this->tetrisShapes.at(Z).at(1).SetAsBox(
                this->side_length_m - b2_polygonRadius,
                half_length - 0.5f*b2_polygonRadius,
                b2Vec2(half_length, half_length - b2_polygonRadius),
                0
                );
    
    this->tetrisShapes.at(S).push_back(shape_template);
    this->tetrisShapes.at(S).push_back(shape_template);
    this->tetrisShapes.at(S).at(0).SetAsBox(
                this->side_length_m - b2_polygonRadius,
                half_length - 0.5f*b2_polygonRadius,
                b2Vec2(half_length, -half_length + b2_polygonRadius),
                0
                );
    this->tetrisShapes.at(S).at(1).SetAsBox(
                this->side_length_m - b2_polygonRadius,
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
                b2Vec2(0, this->side_length_m - b2_polygonRadius),
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

void GameA::initializeTetrisPieceImages(){
    
    // TODO: this will use the CURRENT screen resolution to upscale the piece images,
    // and will not handle the case where the game starts on a low res screen and moves to a higher res screen.
    // call init again?
    int screen_height = ((NT3Window*)(this->parent()))->screen()->availableGeometry().height();
    this->piece_image_scale = screen_height*1.0/tetris_field_m.height();
    
    this->piece_images.clear();
    this->piece_rects_m.clear();
    
    for (uint8 piece = 0; piece < num_tetris_pieces; piece++){
        
        QString path = ":/resources/graphics/pieces/" + QString::number(piece) + ".png";
        QPixmap orig_pixmap = this->colorize(QPixmap(path));
        orig_pixmap = orig_pixmap.scaled(orig_pixmap.size()*this->piece_image_scale);
        
        this->piece_images.push_back(this->enableAlphaChannel(orig_pixmap));
        
        /*printf("%u: orig_pixmap: %u, this->piece_images.back(): %u\n",
               piece, orig_pixmap.hasAlphaChannel(), this->piece_images.back().hasAlphaChannel());*/

        path = ":/resources/graphics/pieces/powerup.png";
        orig_pixmap = this->colorize(QPixmap(path));
        orig_pixmap = orig_pixmap.scaled(orig_pixmap.size()*this->piece_image_scale);
        QImage orig_img = orig_pixmap.toImage();
        QSize piece_size = this->piece_images.back().size();
        QImage pwu_pieceimage = QImage(piece_size, QImage::Format_ARGB32);
        pwu_pieceimage.fill(Qt::transparent);
        QPainter painter(&pwu_pieceimage);
        
        switch(piece){
        case I:
            painter.drawImage(QPoint(0, 0), orig_img);
            painter.drawImage(QPoint(orig_img.width(), 0), orig_img);
            painter.drawImage(QPoint(orig_img.width()*2, 0), orig_img);
            painter.drawImage(QPoint(orig_img.width()*3, 0), orig_img);
            this->piece_rects_m.push_back(QRectF(-2*this->side_length_m, -this->side_length_m/2, 4*this->side_length_m, this->side_length_m));
            break;
        case O:
            painter.drawImage(QPoint(0, 0), orig_img);
            painter.drawImage(QPoint(orig_img.width(), 0), orig_img);
            painter.drawImage(QPoint(orig_img.width(), orig_img.height()), orig_img);
            painter.drawImage(QPoint(0, orig_img.height()), orig_img);
            this->piece_rects_m.push_back(QRectF(-this->side_length_m, -this->side_length_m, 2*this->side_length_m, 2*this->side_length_m));
            break;
        case G:
            painter.drawImage(QPoint(0, 0), orig_img);
            painter.drawImage(QPoint(orig_img.width(), 0), orig_img);
            painter.drawImage(QPoint(orig_img.width()*2, 0), orig_img);
            painter.drawImage(QPoint(orig_img.width()*2, orig_img.height()), orig_img);
            this->piece_rects_m.push_back(QRectF(-3*this->side_length_m/2, -this->side_length_m/2, 3*this->side_length_m, 2*this->side_length_m));
            break;
        case L:
            painter.drawImage(QPoint(0, 0), orig_img);
            painter.drawImage(QPoint(orig_img.width(), 0), orig_img);
            painter.drawImage(QPoint(orig_img.width()*2, 0), orig_img);
            painter.drawImage(QPoint(0, orig_img.height()), orig_img);
            this->piece_rects_m.push_back(QRectF(-3*this->side_length_m/2, -this->side_length_m/2, 3*this->side_length_m, 2*this->side_length_m));
            break;
        case T:
            painter.drawImage(QPoint(0, 0), orig_img);
            painter.drawImage(QPoint(orig_img.width(), 0), orig_img);
            painter.drawImage(QPoint(orig_img.width()*2, 0), orig_img);
            painter.drawImage(QPoint(orig_img.width(), orig_img.height()), orig_img);
            this->piece_rects_m.push_back(QRectF(-3*this->side_length_m/2, -this->side_length_m/2, 3*this->side_length_m, 2*this->side_length_m));
            break;
        case Z:
            painter.drawImage(QPoint(0, 0), orig_img);
            painter.drawImage(QPoint(orig_img.width(), 0), orig_img);
            painter.drawImage(QPoint(orig_img.width(), orig_img.height()), orig_img);
            painter.drawImage(QPoint(orig_img.width()*2, orig_img.height()), orig_img);
            this->piece_rects_m.push_back(QRectF(-3*this->side_length_m/2, -this->side_length_m, 3*this->side_length_m, 2*this->side_length_m));
            break;
        case S:
            painter.drawImage(QPoint(0, orig_img.height()), orig_img);
            painter.drawImage(QPoint(orig_img.width(), 0), orig_img);
            painter.drawImage(QPoint(orig_img.width(), orig_img.height()), orig_img);
            painter.drawImage(QPoint(orig_img.width()*2, 0), orig_img);
            this->piece_rects_m.push_back(QRectF(-3*this->side_length_m/2, -this->side_length_m, 3*this->side_length_m, 2*this->side_length_m));
            break;
        default:
            fprintf(stderr, "Piece not defined: %u\n", piece);
            break;
        }

        painter.end();
        pwu_piece_images.push_back(this->enableAlphaChannel(QPixmap::fromImage(pwu_pieceimage)));
    }

    this->default_piece_image = this->piece_images.at(default_tetris_piece);
    this->default_piece_rect_m = this->piece_rects_m.at(default_tetris_piece);
    this->default_data.image = this->default_piece_image;
    this->default_data.region_m = this->default_piece_rect_m;
}

void GameA::initializeWalls(){
    
    float32 t_height = static_cast<float32>(tetris_field_m.height());
    float32 t_width = static_cast<float32>(tetris_field_m.width());
    
    b2BodyDef edgeBodyDef;
    edgeBodyDef.type = b2_kinematicBody;
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

void GameA::init_BDC(){
    this->row_areas_m2.clear();
    this->body_area_contributions_m2.clear();
    for (uint r = 0; r < this->tetris_rows; r++){
        this->row_areas_m2.push_back(0.0f);
        this->body_area_contributions_m2.push_back(QHash<b2Body*, float32>());
    }
}
