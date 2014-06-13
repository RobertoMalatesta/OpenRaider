/*!
 * \file include/World.h
 * \brief The game world (model)
 *
 * \author Mongoose
 * \author xythobuz
 */

#ifndef _WORLD_H_
#define _WORLD_H_

#include <list>
#include <vector>

#include "Entity.h"
#include "Room.h"
#include "SkeletalModel.h"
#include "Sprite.h"

#include "WorldData.h"

/*!
 * \brief The game world (model)
 */
class World {
public:

    /*!
     * \brief Deconstructs an object of World
     */
    ~World();

    /*!
     * \brief Clears all data in world
     */
    void destroy();

    void addRoom(Room &room);
    unsigned int sizeRoom();
    Room &getRoom(unsigned int index);

    void addSprite(SpriteSequence &sprite);
    unsigned int sizeSprite();
    SpriteSequence &getSprite(unsigned int index);

    void addEntity(Entity &entity);
    unsigned int sizeEntity();
    Entity &getEntity(unsigned int index);

    void addSkeletalModel(SkeletalModel &model);
    unsigned int sizeSkeletalModel();
    SkeletalModel &getSkeletalModel(unsigned int index);

    /*!
     * \brief Adds mesh to world
     * \param model mesh to add
     */
    void addMesh(model_mesh_t *model);

    model_mesh_t *getMesh(int index);

    /*!
     * \brief Find room a location is in.
     *
     * If it fails to be in a room it gives closest overlapping room.
     * \param index Guessed room index
     * \param x X coordinate
     * \param y Y coordinate
     * \param z Z coordinate
     * \returns correct room index or -1 for unknown
     */
    int getRoomByLocation(int index, float x, float y, float z);

    /*!
     * \brief Find room a location is in.
     *
     * If it fails to be in a room it gives closest overlapping room.
     * \param x X coordinate
     * \param y Y coordinate
     * \param z Z coordinate
     * \returns correct room index or -1 for unknown
     */
    int getRoomByLocation(float x, float y, float z);

private:

    // Old World
    std::vector<model_mesh_t *> mMeshes;     //!< Unanimated meshes

    // New World
    std::vector<Room *> mRooms;
    std::vector<SpriteSequence *> mSprites;
    std::vector<Entity *> mEntities;
    std::vector<SkeletalModel *> mModels;
};

World &getWorld();

#endif
