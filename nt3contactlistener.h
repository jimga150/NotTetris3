#ifndef NT3CONTACTLISTENER_H
#define NT3CONTACTLISTENER_H

#include <vector>
#include <algorithm>

#include "box2d/b2_world_callbacks.h"
#include "box2d/b2_contact.h"

class NT3ContactListener : public b2ContactListener
{
public:
    explicit NT3ContactListener();

    void BeginContact(b2Contact* contact);
    //void EndContact(b2Contact* contact);
    //void PreSolve(b2Contact* contact, const b2Manifold* oldManifold);
    //void PostSolve(b2Contact* contact, const b2ContactImpulse* impulse);

    bool hasCurrentPieceCollided();

    b2Body* currentPiece = nullptr;
    std::vector<b2Body*> exceptions;

private:
    bool currentPieceCollided = false;

};

#endif // NT3CONTACTLISTENER_H
