#ifndef INCLUDE_LOCATION_BASE_HPP
#define INCLUDE_LOCATION_BASE_HPP

#include "al/core/app/al_App.hpp"
#include "al/core/graphics/al_VAOMesh.hpp"

struct Location{
    Vec3f position;
    float scaleFactor;
    Color c;
    Mesh mesh;
    Mesh mesh_wire;
};

#endif
