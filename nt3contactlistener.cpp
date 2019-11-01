#include "nt3contactlistener.h"

NT3ContactListener::NT3ContactListener(QObject *parent) : QObject(parent)
{

}

void NT3ContactListener::BeginContact(b2Contact* contact){
    //printf("Contact!\n");

    if (this->currentPiece == nullptr) return;

    b2Fixture* fixtureA = contact->GetFixtureA();
    b2Fixture* fixtureB = contact->GetFixtureB();

    for (b2Body* b : this->exceptions){
        for (b2Fixture* f = b->GetFixtureList(); f; f = f->GetNext()){
            if (f == fixtureA || f == fixtureB){
                return;
            }
        }
    }

    for (b2Fixture* f = this->currentPiece->GetFixtureList(); f; f = f->GetNext()){
        if (f == fixtureA || f == fixtureB){
            this->currentPieceCollided = true;
            return;
        }
    }
}

void NT3ContactListener::EndContact(b2Contact* contact){

}

void NT3ContactListener::PreSolve(b2Contact* contact, const b2Manifold* oldManifold){

}

void NT3ContactListener::PostSolve(b2Contact* contact, const b2ContactImpulse* impulse){

}

bool NT3ContactListener::hasCurrentPieceCollided(){
    bool ans = this->currentPieceCollided;
    this->currentPieceCollided = false;
    return ans;
}
