#ifndef CONSTANTS_H
#define CONSTANTS_H

const float WORLD_SIZE = 150.0f;
const float WALL_HEIGHT = 5.0f;
const float WALL_THICKNESS = 1.0f;

const float CAR_START_X = 0.0f;
const float CAR_START_Z = 0.0f;
const float CAR_START_ANGLE_DEG = 0.0f;
const float CAR_DEFAULT_SPEED = 0.0f;
const float CAR_SPEED_INC = 0.1f;
const float CAR_TURN_INC = 2.0f; // degrees

// Camera settings
const float CAMERA_FOV = 45.0f;
const float CAMERA_NEAR_PLANE = 0.1f;
const float CAMERA_FAR_PLANE = 1000.0f;

const float CAMERA_SKY_HEIGHT = 200.0f;
const float CAMERA_GROUND_HEIGHT = 4.0f; // from the building

const float CAMERA_HELICOPTER_HEIGHT = 15.0f;
const float CAMERA_HELICOPTER_DISTANCE = 25.0f;
const float PAN_SPEED = 1.5f;                // speed of motion ground view camera
const float GROUND_CAMERA_MAX_ANGLE = 30.0f; // degrees

// Track settings
const float TRACK_RADIUS_X = 40.0f; // outer horizontal radius
const float TRACK_RADIUS_Z = 30.0f; // outer vertical radius
const float TRACK_WIDTH = 15.0f;
const int TRACK_SEGMENTS = 64;      // smoothness of curve

// Building settings
const int BUILDING_SIDES = 2;
const int BUILDINGS_PER_SIDE = 2;
const int BUILDING_COUNT = BUILDING_SIDES * BUILDINGS_PER_SIDE;
const int BUILDING_MODEL_COUNT = 4;

const float BUILDING_TARGET_FOOTPRINT = 6.0f;
const float BUILDING_SIDE_CLEARANCE = 2.0f;
const float BUILDING_SIDE_Z_SPACING = 24.0f;
const float BUILDING_COLOR_R = 1.0f;
const float BUILDING_COLOR_G = 1.0f;
const float BUILDING_COLOR_B = 1.0f;
const float BUILDING_ROOF_ATTACHMENT_EDGE_MARGIN = 0.2f;

const float CAMERA_GROUND_FRONT_OFFSET = 0.5f;

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
const float CAR_CABIN_OFFSET_Y = 7.0f;
const float CAR_CABIN_OFFSET_Z = 0.0f;

const float CAR_WINDOW_THICKNESS = 0.60f;
const float CAR_WINDOW_HEIGHT = 2.25f;

const float CAR_WHEEL_X_RATIO = 0.625f; // how far the wheels are pushed toward the front bumper
const float CAR_WHEEL_Z_PROTRUSION = 0.5f; // how far the wheels pop out from the sides of the car

const int MAX_CAR_HEADLIGHTS = 2;
const float CAR_HEADLIGHT_LOCAL_X = (CAR_FRAME_LENGTH * 0.5f) + 0.2f;
const float CAR_HEADLIGHT_LOCAL_Y = CAR_FRAME_HEIGHT;
const float CAR_HEADLIGHT_LOCAL_Z_OFFSET = CAR_FRAME_WIDTH * 0.28f;
const float CAR_HEADLIGHT_DIRECTION_Y = -0.09f;
const float CAR_HEADLIGHT_VISUAL_WIDTH = 0.7f;
const float CAR_HEADLIGHT_VISUAL_HEIGHT = 0.42f;
const float CAR_HEADLIGHT_VISUAL_DEPTH = 0.25f;

const float CAR_HEADLIGHT_INNER_CUTOFF_DEG = 13.5f;
const float CAR_HEADLIGHT_OUTER_CUTOFF_DEG = 18.5f;
const float CAR_HEADLIGHT_ATTENUATION_CONSTANT = 1.0f;
const float CAR_HEADLIGHT_ATTENUATION_LINEAR = 0.028f;
const float CAR_HEADLIGHT_ATTENUATION_QUADRATIC = 0.0038f;
const float CAR_HEADLIGHT_AMBIENT_STRENGTH = 0.022f;
const float CAR_HEADLIGHT_COLOR_R = 1.0f;
const float CAR_HEADLIGHT_COLOR_G = 1.0f;
const float CAR_HEADLIGHT_COLOR_B = 1.0f;

// Collision settings
const float CAR_HITBOX_LENGTH = CAR_FRAME_LENGTH * CAR_SCALE;
const float CAR_HITBOX_WIDTH = (CAR_FRAME_WIDTH + (2.0f * CAR_WHEEL_Z_PROTRUSION)) * CAR_SCALE;
const float CAR_HITBOX_HEIGHT = (CAR_CABIN_OFFSET_Y + (0.5f * CAR_CABIN_HEIGHT)) * CAR_SCALE;

const float COLLISION_SAT_EPSILON = 0.01f;
const float HITBOX_VISUAL_PADDING = 0.08f;
const float HITBOX_LINE_WIDTH = 2.0f;

// Windmill settings
const float WINDMILL_RADIUS = 2.5f;
const float WINDMILL_DEPTH = 0.5f;
const float WINDMILL_BLADE_WIDTH = 0.6f;
const float WINDMILL_COLOR_R = 0.8f;
const float WINDMILL_COLOR_G = 0.8f;
const float WINDMILL_COLOR_B = 0.8f;

const float WINDMILL_DEFAULT_SPEED = 1.5f; // radians per second
const float WINDMILL_SPEED_INC = 2.0f;

// Lighting and shading settings
const int MAX_BUILDING_LIGHTS = 10;
const int BUILDING_LIGHT_COLOR_COUNT = 6;
const float BUILDING_LIGHT_COLORS[BUILDING_LIGHT_COLOR_COUNT][3] = {
	{1.0f, 0.2f, 0.2f},
	{0.2f, 1.0f, 0.2f},
	{0.2f, 0.2f, 1.0f},
	{1.0f, 1.0f, 0.2f},
	{1.0f, 0.2f, 1.0f},
	{0.2f, 1.0f, 1.0f},
};

const float BUILDING_LIGHT_HEIGHT_OFFSET = 0.0f;
const float BUILDING_LIGHT_BASE_DIRECTION_Y = -0.6f;
const float BUILDING_LIGHT_SWING_BASE_SPEED = 0.35f;
const float BUILDING_LIGHT_SWING_SPEED_STEP = 0.05f;
const float BUILDING_LIGHT_SWING_MAX_ANGLE_DEG = 30.0f;

const float BUILDING_LIGHT_ROAD_FACING_YAW_POSITIVE_X_DEG = 180.0f;
const float BUILDING_LIGHT_ROAD_FACING_YAW_NEGATIVE_X_DEG = 0.0f;

const float WINDMILL_LIGHT_OCCLUSION_PARALLEL_EPSILON = 1e-4f;
const float WINDMILL_LIGHT_OCCLUSION_SOFT_EDGE = 0.12f;
const float WINDMILL_LIGHT_HUB_RADIUS_FACTOR = 0.5f;
const float WINDMILL_LIGHT_HUB_MIN_TRANSMISSION = 0.45f;
const float WINDMILL_LIGHT_BLADE_MIN_TRANSMISSION = 0.55f;

const float BUILDING_LIGHT_GIMBAL_BASE_WIDTH = 1.2f;
const float BUILDING_LIGHT_GIMBAL_BASE_HEIGHT = 0.35f;
const float BUILDING_LIGHT_GIMBAL_BASE_DEPTH = 1.2f;
const float BUILDING_LIGHT_GIMBAL_YOKE_WIDTH = 1.0f;
const float BUILDING_LIGHT_GIMBAL_YOKE_HEIGHT = 0.95f;
const float BUILDING_LIGHT_GIMBAL_YOKE_BAR_THICKNESS = 0.12f;
const float BUILDING_LIGHT_GIMBAL_PIVOT_HEIGHT = 0.62f;
const float BUILDING_LIGHT_LAMP_WIDTH = 0.65f;
const float BUILDING_LIGHT_LAMP_HEIGHT = 0.38f;
const float BUILDING_LIGHT_LAMP_DEPTH = 0.75f;
const float BUILDING_LIGHT_LAMP_CENTER_X_FACTOR = 0.42f;
const float BUILDING_LIGHT_LAMP_CENTER_Y = -0.10f;
const float BUILDING_LIGHT_LAMP_NOZZLE_X_FACTOR = 0.92f;
const float BUILDING_LIGHT_LAMP_NOZZLE_DEPTH_FACTOR = 0.15f;
const float BUILDING_LIGHT_LAMP_NOZZLE_HEIGHT_FACTOR = 0.70f;
const float BUILDING_LIGHT_LAMP_NOZZLE_WIDTH_FACTOR = 0.70f;

const float DEFAULT_LIGHTSOURCE_VIEW_HEIGHT = 20.0f;
const float DEFAULT_LIGHTSOURCE_VIEW_DIR_X = 0.0f;
const float DEFAULT_LIGHTSOURCE_VIEW_DIR_Y = -1.0f;
const float DEFAULT_LIGHTSOURCE_VIEW_DIR_Z = 0.0f;

const float BUILDING_LIGHT_INNER_CUTOFF_DEG = 12.5f;
const float BUILDING_LIGHT_OUTER_CUTOFF_DEG = 17.5f;
const float BUILDING_LIGHT_ATTENUATION_CONSTANT = 1.0f;
const float BUILDING_LIGHT_ATTENUATION_LINEAR = 0.014f;
const float BUILDING_LIGHT_ATTENUATION_QUADRATIC = 0.0007f;

const float GLOBAL_AMBIENT_STRENGTH = 0.1f;
const float SPOTLIGHT_AMBIENT_STRENGTH = 0.05f;

// Day-night cycle and sun settings
const bool DAY_NIGHT_CYCLE_ENABLED = true;
const float DAY_NIGHT_CYCLE_DURATION_SECONDS = 180.0f;
const float DAY_NIGHT_SUN_PHASE_OFFSET_DEG = 90.0f;
const float DAY_NIGHT_SUN_ORBIT_Z_BIAS = 0.22f;

const float DAY_NIGHT_DAYLIGHT_START_HEIGHT = -0.12f;
const float DAY_NIGHT_DAYLIGHT_END_HEIGHT = 0.18f;
const float DAY_NIGHT_HORIZON_BAND_MAX_ABS_HEIGHT = 0.42f;
const float DAY_NIGHT_HORIZON_BAND_MIN_ABS_HEIGHT = 0.02f;
const float DAY_NIGHT_HORIZON_TINT_STRENGTH = 0.40f;

const float GLOBAL_AMBIENT_DAY = GLOBAL_AMBIENT_STRENGTH;
const float GLOBAL_AMBIENT_NIGHT = 0.055f;
const float SPOTLIGHT_AMBIENT_DAY = SPOTLIGHT_AMBIENT_STRENGTH * 0.35f;
const float SPOTLIGHT_AMBIENT_NIGHT = SPOTLIGHT_AMBIENT_STRENGTH;
const float HEADLIGHT_AMBIENT_DAY = CAR_HEADLIGHT_AMBIENT_STRENGTH * 0.45f;
const float HEADLIGHT_AMBIENT_NIGHT = CAR_HEADLIGHT_AMBIENT_STRENGTH;

const float DAY_NIGHT_BUILDING_LIGHT_COLOR_SCALE_DAY = 0.35f;
const float DAY_NIGHT_BUILDING_LIGHT_COLOR_SCALE_NIGHT = 1.0f;
const float DAY_NIGHT_HEADLIGHT_COLOR_SCALE_DAY = 0.5f;
const float DAY_NIGHT_HEADLIGHT_COLOR_SCALE_NIGHT = 1.0f;

const float SUN_DIFFUSE_STRENGTH_DAY = 0.95f;
const float SUN_DIFFUSE_STRENGTH_NIGHT = 0.12f;
const float SUN_SPECULAR_STRENGTH_DAY = 0.75f;
const float SUN_SPECULAR_STRENGTH_NIGHT = 0.05f;
const float SUN_AMBIENT_STRENGTH_DAY = 0.07f;
const float SUN_AMBIENT_STRENGTH_NIGHT = 0.025f;

const float SKY_DAY_R = 0.53f;
const float SKY_DAY_G = 0.81f;
const float SKY_DAY_B = 0.92f;
const float SKY_NIGHT_R = 0.08f;
const float SKY_NIGHT_G = 0.10f;
const float SKY_NIGHT_B = 0.18f;
const float SKY_HORIZON_R = 0.95f;
const float SKY_HORIZON_G = 0.47f;
const float SKY_HORIZON_B = 0.22f;

const float SUN_COLOR_DAY_R = 1.0f;
const float SUN_COLOR_DAY_G = 0.97f;
const float SUN_COLOR_DAY_B = 0.90f;
const float SUN_COLOR_NIGHT_R = 0.38f;
const float SUN_COLOR_NIGHT_G = 0.44f;
const float SUN_COLOR_NIGHT_B = 0.58f;
const float SUN_COLOR_HORIZON_R = 1.0f;
const float SUN_COLOR_HORIZON_G = 0.58f;
const float SUN_COLOR_HORIZON_B = 0.30f;

const float METAL_SHININESS = 128.0f;
const float METAL_SPECULAR_STRENGTH = 1.5f;
const float DIFFUSE_SHININESS = 1.0f;
const float DIFFUSE_SPECULAR_STRENGTH = 0.0f;

#endif