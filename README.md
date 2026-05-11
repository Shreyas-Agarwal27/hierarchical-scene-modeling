# Computer Graphics Assignment 3

[![Review Assignment Due Date](https://classroom.github.com/assets/deadline-readme-button-22041afd0340ce965d47ae6ef1cefeee28c7c493a6346c4f15d667ab976d596c.svg)](https://classroom.github.com/a/t8noNzs3)

An interactive OpenGL city scene with a drivable car, windmills, dynamic building spotlights, car headlights, collision detection, and a day-night lighting cycle.

## Features

- Multiple camera modes (sky, car, ground, light source, helicopter)
- Drivable car with steering and collision detection
- Building-mounted moving spotlights and car headlights
- Dynamic sunlight and sky color from a day-night cycle
- Windmill-light interaction that attenuates spotlight beams
- Optional hitbox visualization for collision debugging
- Live FPS shown in the window title

## Build And Run

### Prerequisites (Linux)

- C++ compiler with C++17 support (g++)
- C compiler (gcc)
- Make
- OpenGL runtime and development libraries
- GLFW development package

Example package install on Debian/Ubuntu:

```bash
sudo apt update
sudo apt install -y build-essential libglfw3-dev libgl1-mesa-dev
```

### Commands

From the project root:

```bash
make
make run
```

Clean build artifacts:

```bash
make clean
```

## Controls

### Camera Modes

- `1`: Sky view
- `2`: Car view
- `3`: Ground view
- `4`: Light source view
- `5`: Helicopter view

### Scene Interaction

- `W`: Increase car speed
- `S`: Decrease car speed
- `A`: Turn car left (when moving)
- `D`: Turn car right (when moving)
- `H`: Toggle car headlights
- `Up Arrow`: Increase windmill speed
- `Down Arrow`: Decrease windmill speed
- `Left Arrow`: Pan ground camera left (ground view)
- `Right Arrow`: Pan ground camera right (ground view)
- `B`: Toggle hitbox rendering
- `Space`: Reset world state
- `Esc`: Exit

## Project Structure

- `src/`: Application, rendering, camera, lighting, collision, and mesh generation logic
- `include/`: Headers, constants, GLM, GLAD, and support utilities
- `shaders/`: Vertex and fragment shaders
- `textures/`: Texture assets
- `models/`, `obj/`: Model data/resources
- `Makefile`: Build and run targets

## Technical Notes

- OpenGL 3.3 core profile with GLFW window/input handling and GLAD loader
- Lighting supports:
	- Global ambient term
	- Dynamic sun (ambient, diffuse, specular)
	- Building spotlights
	- Car headlights
- Collision uses SAT checks between a car OBB and static obstacle AABBs
