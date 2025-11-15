## Software Renderer

### Overview
A prototype voxel engine written in C using SDL2,
running entirely on the CPU.

It has first person camera controls.

### Rendering
First, ray intersection with the model bounding box is checked with an AABB.
If the ray intersects the bounding box, it will march through using DDA


