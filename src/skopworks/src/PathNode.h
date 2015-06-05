#ifndef _PATHNODE_H
#define _PATHNODE_H
#include <math.h>

class PathNode
{
public:
	PathNode(float, float);
	bool operator==(const PathNode& other) const;
	bool operator!=(const PathNode& other) const;

	float calculateF(PathNode* end);
	float calculateG(PathNode* parent);
	float calculateH(PathNode* end);

	PathNode* parent;
	float x;
	float y;

	float f;
	float g;
	float h;

	bool isWalkable;
	bool hasPellet;
	bool hasPowerPellet;
	bool hasCherry;

	//Instead of checking through the open and closed list to see if the Node is already there,
    //there is a bool value inOpenList and inClosedList; this saves a lot of computation time.
	bool inOpenList;
	bool inClosedList;

	bool isLowestPath;

};

#endif