#include "nt3contactlistener.h"

NT3ContactListener::NT3ContactListener(QObject *parent) : QObject(parent)
{

}

void NT3ContactListener::BeginContact(b2Contact* contact){
    //contact->GetFixtureA();
    printf("Contact!\n");
    //fflush(stdout);
}

void NT3ContactListener::EndContact(b2Contact* contact){

}

void NT3ContactListener::PreSolve(b2Contact* contact, const b2Manifold* oldManifold){

}

void NT3ContactListener::PostSolve(b2Contact* contact, const b2ContactImpulse* impulse){

}
