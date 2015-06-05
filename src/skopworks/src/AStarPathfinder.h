#ifndef _ASTAR_PATHFINDER
#define _ASTAR_PATHFINDER

#include "PathNode.h"

#include <tlocCore/tloc_core.h>
#include <tlocGraphics/tloc_graphics.h>
#include <tlocMath/tloc_math.h>
#include <tlocPrefab/tloc_prefab.h>

using namespace tloc;
class AStarPathFinder
{
public:
	//AStarPathFinder();
	AStarPathFinder(core_conts::Array<core_conts::Array<PathNode*>>grid);

	bool findPath(float startX, float startY, float endX, float endY);
	void clearPath();
	void printPath();

	core_conts::Array<core_conts::Array<PathNode*>> pathGrid;

	PathNode* start;
	PathNode* end;
	PathNode* current;

private:

	core_conts::List<PathNode*> openList;
	core_conts::List<PathNode*> closedList;
};

#endif