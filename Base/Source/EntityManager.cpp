#include "EntityManager.h"
#include "EntityBase.h"
#include "Collider/Collider.h"

#include <iostream>
using namespace std;

// Update all entities
void EntityManager::Update(double _dt)
{
	// Update all entities
	std::list<EntityBase*>::iterator it, end;
	end = entityList.end();
	for (it = entityList.begin(); it != end; ++it)
	{
		(*it)->Update(_dt);
	}

	// Clean up entities that are done
	it = entityList.begin();
	while (it != end)
	{
		if ((*it)->IsDone())
		{
			// Delete if done
			delete *it;
			it = entityList.erase(it);
		}
		else
		{
			// Move on otherwise
			++it;
		}
	}
}

// Render all entities
void EntityManager::Render()
{
	// Render all entities
	std::list<EntityBase*>::iterator it, end;
	end = entityList.end();
	for (it = entityList.begin(); it != end; ++it)
	{
		(*it)->Render();
	}
}

// Render the UI entities
void EntityManager::RenderUI()
{
	// Render all entities UI
	std::list<EntityBase*>::iterator it, end;
	end = entityList.end();
	for (it = entityList.begin(); it != end; ++it)
	{
		(*it)->RenderUI();
	}
}

// Add an entity to this EntityManager
void EntityManager::AddEntity(EntityBase* _newEntity)
{
	entityList.push_back(_newEntity);
}

// Remove an entity from this EntityManager
bool EntityManager::RemoveEntity(EntityBase* _existingEntity)
{
	// Find the entity's iterator
	std::list<EntityBase*>::iterator findIter = std::find(entityList.begin(), entityList.end(), _existingEntity);

	// Delete the entity if found
	if (findIter != entityList.end())
	{
		delete *findIter;
		findIter = entityList.erase(findIter);
		return true;	
	}
	// Return false if not found
	return false;
}

// Constructor
EntityManager::EntityManager()
{
}

// Destructor
EntityManager::~EntityManager()
{
}

// Check for overlap
bool EntityManager::CheckOverlap(Vector3 thisMinAABB, Vector3 thisMaxAABB, Vector3 thatMinAABB, Vector3 thatMaxAABB)
{	
	return 
		((thisMinAABB.x <= thatMaxAABB.x && thisMaxAABB.x >= thatMinAABB.x) && (thisMinAABB.y <= thatMaxAABB.y && thisMaxAABB.y >= thatMinAABB.y) && (thisMinAABB.z <= thatMaxAABB.z && thisMaxAABB.z >= thatMinAABB.z));
}

// Check if this entity's bounding sphere collided with that entity's bounding sphere 
bool EntityManager::CheckSphereCollision(EntityBase *ThisEntity, EntityBase *ThatEntity)
{

	return false;
}

// Check if this entity collided with another entity, but both must have collider
bool EntityManager::CheckAABBCollision(CCollider *ThisEntity, CCollider *ThatEntity)
{
	
	return ((ThisEntity->GetMinAABB().x <= ThatEntity->GetMaxAABB().x&& ThisEntity->GetMaxAABB().x >= ThatEntity->GetMinAABB().x)&& (ThisEntity->GetMinAABB().y <= ThatEntity->GetMaxAABB().y&& ThisEntity->GetMaxAABB().y >= ThatEntity->GetMinAABB().y)&& (ThisEntity->GetMinAABB().z <= ThatEntity->GetMaxAABB().z&& ThisEntity->GetMaxAABB().z >= ThatEntity->GetMinAABB().z));
}

// Check if any Collider is colliding with another Collider
bool EntityManager::CheckForCollision(void)
{
	return false;
}
bool EntityManager::pointtoAABB(Vector3 pos, Vector3 forward, GenericEntity *ThatEntity)
{
	//returna true when in bounds
	return (pos.x  < ThatEntity->GetMaxAABB().x && pos.x  > ThatEntity->GetMinAABB().x)
		&& (pos.y < ThatEntity->GetMaxAABB().y && pos.y  > ThatEntity->GetMinAABB().y)
		&& (pos.z  < ThatEntity->GetMaxAABB().z && pos.z > ThatEntity->GetMinAABB().z);
}