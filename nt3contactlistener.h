#ifndef NT3CONTACTLISTENER_H
#define NT3CONTACTLISTENER_H

#include <QObject>

#include "Box2D/Dynamics/b2WorldCallbacks.h"
#include "Box2D/Dynamics/Contacts/b2Contact.h"

class NT3ContactListener : public QObject, public b2ContactListener
{
    Q_OBJECT
public:
    explicit NT3ContactListener(QObject *parent = nullptr);

    void BeginContact(b2Contact* contact);
    void EndContact(b2Contact* contact);
    void PreSolve(b2Contact* contact, const b2Manifold* oldManifold);
    void PostSolve(b2Contact* contact, const b2ContactImpulse* impulse);

    bool hasCurrentPieceCollided();

    b2Body* currentPiece = nullptr;

private:
    bool currentPieceCollided = false;

signals:

public slots:
};

#endif // NT3CONTACTLISTENER_H
