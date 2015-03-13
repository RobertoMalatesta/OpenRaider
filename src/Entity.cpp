/*!
 * \file src/Entity.cpp
 * \brief Things in the World
 *
 * \author xythobuz
 */

#include "global.h"
#include "Camera.h"
#include "Log.h"
#include "World.h"
#include "Entity.h"

#include <glm/gtc/matrix_transform.hpp>

#define CACHE_SPRITE 0
#define CACHE_MESH 1
#define CACHE_MODEL 2

bool Entity::showEntitySprites = true;
bool Entity::showEntityMeshes = true;
bool Entity::showEntityModels = true;

void Entity::display(glm::mat4 VP) {
    if ((cache == -1) || (cacheType == -1)) {
        /*
         * The order in which to look for matching objects with the same ID
         * seems to be very important!
         * If sprites and meshes are searched before models, many objects will
         * be displayed wrong (eg. 'bad guy' becomes 'clothes' in tr2/boat)...
         */

        for (int i = 0; (i < getWorld().sizeSkeletalModel()) && (cache == -1); i++) {
            auto& s = getWorld().getSkeletalModel(i);
            if (s.getID() == id) {
                cacheType = CACHE_MODEL;
                cache = i;
                break;
            }
        }

        for (int i = 0; (i < getWorld().sizeStaticMesh()) && (cache == -1); i++) {
            auto& s = getWorld().getStaticMesh(i);
            if (s.getID() == id) {
                cacheType = CACHE_MESH;
                cache = i;
                break;
            }
        }

        for (int i = 0; i < getWorld().sizeSpriteSequence(); i++) {
            auto& s = getWorld().getSpriteSequence(i);
            if (s.getID() == id) {
                cacheType = CACHE_SPRITE;
                cache = i;
                break;
            }
        }

        assertGreaterThan(cache, -1);
        assertGreaterThan(cacheType, -1);
    }

    glm::mat4 translate = glm::translate(glm::mat4(1.0f), glm::vec3(pos.x, -pos.y, pos.z));
    glm::mat4 rotate;
    if (cacheType == 0) {
        rotate = glm::rotate(glm::mat4(1.0f), Camera::getRotation().x, glm::vec3(0.0f, 1.0f, 0.0f));
    } else {
        rotate = glm::rotate(glm::mat4(1.0f), rot.y, glm::vec3(0.0f, 1.0f, 0.0f));
    }
    glm::mat4 model = translate * rotate;
    glm::mat4 MVP = VP * model;

    if (cacheType == CACHE_SPRITE) {
        if (showEntitySprites)
            getWorld().getSpriteSequence(cache).display(MVP, sprite);
    } else if (cacheType == CACHE_MESH) {
        if (showEntityMeshes)
            getWorld().getStaticMesh(cache).display(MVP);
    } else if (cacheType == CACHE_MODEL) {
        if (showEntityModels)
            getWorld().getSkeletalModel(cache).display(MVP, animation, frame);
    } else {
        assert(false && "This should not happen...");
    }
}

