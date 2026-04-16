#ifndef OBJECT_MESEHES_H
#define OBJECT_MESEHES_H

#include "mesh.h"

namespace ObjectMeshes {
Mesh createTrack(unsigned int textureID);
Mesh createGround(unsigned int textureID);

Mesh createCarFrame();
Mesh createCarWindows();
Mesh createCarWheels();

Mesh createWindmill();
Mesh createLightGimbalBase();
Mesh createLightGimbalHead();
Mesh createUnitBox();

Mesh createWalls(unsigned int textureID);
}  // namespace ObjectMeshes

#endif