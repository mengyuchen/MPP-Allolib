#ifndef INCLUDE_MESHES_HPP
#define INCLUDE_MESHES_HPP

#include "al/core/app/al_App.hpp"
#include "al/core/graphics/al_Mesh.hpp"

using namespace al;

struct Line : Mesh{
    Line(){
        primitive(Mesh::LINE_STRIP);
        vertex(0,0,0);
        //color(0,1,1);

        vertex(0,0,1);
        //color(0,1,1);
    }
};

struct Factory_Building : Mesh {

    Factory_Building(){
       
    }
};
struct Poem : Mesh{
    Poem(){

    }

};


#endif
