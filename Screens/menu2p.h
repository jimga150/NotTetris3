#ifndef MENU2P_H
#define MENU2P_H

#include <nt3screen.h>

class Menu2P : public NT3Screen
{
    Q_OBJECT
public:
    explicit Menu2P(QObject *parent = nullptr);
    ~Menu2P() override;
    
    
};

#endif // MENU2P_H
