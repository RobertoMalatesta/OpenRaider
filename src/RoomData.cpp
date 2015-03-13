/*!
 * \file src/RoomData.cpp
 * \brief Auxiliary Room classes
 *
 * \author xythobuz
 */

#include "global.h"
#include "Camera.h"
#include "World.h"
#include "system/Shader.h"
#include "RoomData.h"

#include <glm/gtc/matrix_transform.hpp>

void BoundingBox::display(glm::mat4 VP, glm::vec3 colorLine, glm::vec3 colorDot) {
    std::vector<glm::vec3> verts;
    std::vector<glm::vec3> cols;
    std::vector<unsigned short> inds;

    for (int i = 0; i < 8; i++) {
        verts.push_back(corner[i]);
        cols.push_back(colorLine);
    }

    inds.push_back(0);
    inds.push_back(2);
    inds.push_back(4);
    inds.push_back(1);
    inds.push_back(6);
    inds.push_back(7);
    inds.push_back(5);
    inds.push_back(3);
    inds.push_back(0);
    inds.push_back(1);
    inds.push_back(4);
    inds.push_back(7);
    inds.push_back(6);
    inds.push_back(3);
    inds.push_back(5);
    inds.push_back(2);

    static ShaderBuffer vert, col, ind;
    vert.bufferData(verts);
    col.bufferData(cols);
    ind.bufferData(inds);
    Shader::drawGL(vert, col, ind, VP, GL_LINE_STRIP);

    cols.clear();
    inds.clear();

    for (int i = 0; i < 8; i++) {
        cols.push_back(colorDot);
        inds.push_back(i);
    }

    static ShaderBuffer vert2, col2, ind2;
    vert2.bufferData(verts);
    col2.bufferData(cols);
    ind2.bufferData(inds);
    Shader::drawGL(vert2, col2, ind2, VP, GL_POINTS);
}

// ----------------------------------------------------------------------------

StaticModel::StaticModel(glm::vec3 pos, float angle, int i) : id(i), cache(-1) {
    glm::mat4 translate = glm::translate(glm::mat4(1.0f), glm::vec3(pos.x, -pos.y, pos.z));
    glm::mat4 rotate = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 1.0f, 0.0f));
    model = translate * rotate;
}

void StaticModel::display(glm::mat4 VP) {
    if (cache < 0) {
        for (int i = 0; i < getWorld().sizeStaticMesh(); i++) {
            if (getWorld().getStaticMesh(i).getID() == id) {
                cache = i;
            }
        }
        assertGreaterThanEqual(cache, 0);
    }

    getWorld().getStaticMesh(cache).display(VP * model);
}

// ----------------------------------------------------------------------------

void RoomSprite::display(glm::mat4 VP) {
    glm::mat4 translate = glm::translate(glm::mat4(1.0f), glm::vec3(pos.x, -pos.y, pos.z));
    glm::mat4 rotate = glm::rotate(glm::mat4(1.0f), Camera::getRotation().x, glm::vec3(0.0f, 1.0f,
                                   0.0f));
    glm::mat4 model = translate * rotate;

    getWorld().getSprite(sprite).display(VP * model);
}

// ----------------------------------------------------------------------------

float Sector::getFloor() {
    return floor;
}

float Sector::getCeiling() {
    return ceiling;
}

bool Sector::isWall() {
    return wall;
}

