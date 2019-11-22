#include "nt3contactlistener.h"

NT3ContactListener::NT3ContactListener(){

}

void NT3ContactListener::BeginContact(b2Contact* contact){
    //printf("Contact!\n");

    if (this->currentPiece == nullptr) return;

    b2Fixture* fixtureA = contact->GetFixtureA();
    b2Body* bodyA = fixtureA->GetBody();
    
    b2Fixture* fixtureB = contact->GetFixtureB();
    b2Body* bodyB = fixtureB->GetBody();

    for (b2Body* b : this->exceptions){
        if (b == bodyA || b == bodyB){
            return;
        }
    }

    if (bodyA == this->currentPiece || bodyB == this->currentPiece){
        this->currentPieceCollided = true;
    }
}

/*void NT3ContactListener::EndContact(b2Contact* contact){

}*/

/*void NT3ContactListener::PreSolve(b2Contact* contact, const b2Manifold* oldManifold){

}*/

/*void NT3ContactListener::PostSolve(b2Contact* contact, const b2ContactImpulse* impulse){

}*/

bool NT3ContactListener::hasCurrentPieceCollided(){
    bool ans = this->currentPieceCollided;
    this->currentPieceCollided = false;
    return ans;
}
