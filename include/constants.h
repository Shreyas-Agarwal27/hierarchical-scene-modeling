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
const float TRACK_RADIUS_X = 40.0f; // outer horizontal radius
const float TRACK_RADIUS_Z = 30.0f; // outer vertical radius
const float TRACK_WIDTH = 15.0f;
const int TRACK_SEGMENTS = 64;      // smoothness of curve

// Car settings
const float CAR_SCALE = 0.75;
const float CAR_FRAME_LENGTH = 16.0f;
const float CAR_FRAME_WIDTH = 8.0f;
const float CAR_FRAME_HEIGHT = 4.0f;
const float CAR_WHEEL_RADIUS = 2.0f;

const float CAR_CABIN_LENGTH = 8.2f;
const float CAR_CABIN_HEIGHT = 3.0f;
const float CAR_CABIN_WIDTH = 7.2f;
const float CAR_CABIN_OFFSET_X = -2.0f;
const float CAR_CABIN_OFFSET_Y = 7.7f;
const float CAR_CABIN_OFFSET_Z = 0.0f;

const float CAR_WINDOW_THICKNESS = 0.60f;
const float CAR_WINDOW_HEIGHT = 2.25f;

const float CAR_WHEEL_X_RATIO = 0.625f; // how far the wheels are pushed toward the front bumper
const float CAR_WHEEL_Z_PROTRUSION = 0.5f; // how far the wheels pop out from the sides of the car

#endif