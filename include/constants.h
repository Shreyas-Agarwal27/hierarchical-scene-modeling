#ifndef CONSTANTS_H
#define CONSTANTS_H

const float WORLD_SIZE = 100.0f;
const float WALL_HEIGHT = 5.0f;

const float CAR_START_X = 0.0f;
const float CAR_START_Z = 0.0f;
const float CAR_SPEED_INC = 0.1f;
const float CAR_TURN_INC = 2.0f; // degrees

// Camera settings
const float CAMERA_FOV = 45.0f;
const float CAMERA_NEAR_PLANE = 0.1f;
const float CAMERA_FAR_PLANE = 1000.0f;

// Track settings
const float TRACK_RADIUS_X = 30.0f; // outer horizontal radius
const float TRACK_RADIUS_Z = 20.0f; // outer vertical radius
const float TRACK_WIDTH = 5.0f;
const int TRACK_SEGMENTS = 64;      // smoothness of curve

#endif