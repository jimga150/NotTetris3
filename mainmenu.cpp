#include "mainmenu.h"

MainMenu::MainMenu(QObject* parent) : NT3Screen(parent)
{
    for (uint s = 0; s < num_mm_selections; ++s){
        switch(s){
        case P:
            this->selectionPoints[s] = QPoint(1, 124);
            break;
        case PP:
            this->selectionPoints[s] = QPoint(47, 124);
            break;
        case OPTIONS:
            this->selectionPoints[s] = QPoint(93, 124);
            break;
        default:
            fprintf(stderr, "Unknown main menu selection type: %u\n", s);
            break;
        }
    }
}

void MainMenu::render(QPainter& painter){
    painter.drawPixmap(this->scaled_ui_field, this->background);
    this->BOW_font.print(&painter, this->selectionPoints[this->currentSelection]*this->ui_scale, 
            LEFT_ALIGN, ">", this->ui_scale);
}

void MainMenu::keyPressEvent(QKeyEvent* ev){
    mm_selection_enum last_option = static_cast<mm_selection_enum>(num_mm_selections - 1);
    
    switch (ev->key()) {
    case Qt::Key_Left:
        if (!this->currentSelection){
            this->currentSelection = last_option;
        } else {
            this->currentSelection = static_cast<mm_selection_enum>(this->currentSelection - 1);
        }
        break;
    case Qt::Key_Right:
        if (this->currentSelection == last_option){
            this->currentSelection = static_cast<mm_selection_enum>(0);
        } else {
            this->currentSelection = static_cast<mm_selection_enum>(this->currentSelection + 1);
        }
        break;
    case Qt::Key_Return:
        switch(this->currentSelection){
        case P:
            emit this->stateEnd(P_GAMEOPTIONS);
            break;
        case PP:
            emit this->stateEnd(PP_GAMEOPTIONS);
            break;
        case OPTIONS:
            emit this->stateEnd(GLOBAL_OPTIONS);
            break;
        default:
            fprintf(stderr, "Unknown main menu selection type: %u\n", this->currentSelection);
            break;
        }
        break;
    case Qt::Key_Escape:
        emit this->close();
        break;
    default:
        return;
    }
}