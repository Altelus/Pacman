#include "AStarPathfinder.h"

AStarPathFinder::AStarPathFinder( core_conts::Array<core_conts::Array<PathNode*>> grid)
{
	start = end = current = nullptr;
	pathGrid = grid;
}

bool AStarPathFinder::findPath( float startX, float startY, float endX, float endY)
{
	clearPath();

	if (startX - 1 < 0 || startX - 1 > pathGrid.size()-1 ||
		startY - 1 < 0 || startY - 1 > pathGrid.size()-1 ||
		endX - 1 < 0 || endX - 1 > pathGrid.size()-1 ||
		endY - 1 < 0 || endY - 1 > pathGrid.size()-1)
		return false;

	start = pathGrid[(unsigned int)startX - 1][(unsigned int)startY - 1];
	end = pathGrid[(unsigned int)endX - 1][(unsigned int)endY - 1];

	openList.push_back(start);
	
	current = start;

	///REPEAT
	do 
	{
		current = openList.front();
		// set current to node with lowest f value in open list
		for (core_conts::List<PathNode*>::iterator i = openList.begin(); i != openList.end(); ++i)
		{
			if ((*i)->f < current->f)
				current = (*i);
		}

		// move to closed list
		current->inOpenList = false;
		openList.remove(current);

		current->inClosedList = true;
		closedList.push_back(current);

		// end reached, flag parents of node as part of final path
		if (current == end)
		{
			while (current != start)
			{
				current->isLowestPath = true;
				current = current->parent;
			}
			return true;
		}

		// iterate through adjacent nodes
		for (int y = -1; y < 2; ++y)
		{
			for (int x = -1; x < 2; ++x)
			{
				float offsetCol = current->y+y;
				float offsetRow = current->x+x;

				// check if node out of bounds
				if (offsetCol >= 0 && offsetCol < pathGrid.size() &&
					offsetRow >= 0 && offsetRow < pathGrid[(unsigned int)offsetCol].size())
				{
					PathNode* adjNode = pathGrid[(unsigned int)offsetCol][(unsigned int)offsetRow];
					//core_conts::List<PathNode*>::iterator findNode;

					// proceed if adj node in not in closed list and is walkable
					if (adjNode->isWalkable && !adjNode->inClosedList)
						//(std::find(closedList.begin(), closedList.end(), adjNode) == closedList.end() && adjNode != closedList.back()))
					{
						// check if adj node already in open list, if so recalculate and compare G.
						// if new G is shorter, update node in open list
						//if (std::find(openList.begin(), openList.end(), adjNode) != openList.end() || !openList.empty() && adjNode == openList.back())
						if (adjNode->inOpenList)
						{
							if (adjNode->calculateG(current) < adjNode->g)
							{
								adjNode->parent = current;
								adjNode->calculateF(end);
							}
						}
						else
						{
							// not in openlist, add it
							adjNode->parent = current;
							adjNode->calculateF(end);
							adjNode->inOpenList = true;
							openList.push_back(adjNode);
						}
					}
				}
			}
		}
	}while(openList.size());

	return false;
}

void AStarPathFinder::clearPath()
{
	core_conts::List<PathNode*>::iterator findNode;

	for (unsigned int i = 0; i < pathGrid.size(); i++)
	{
		for (unsigned int j = 0; j < pathGrid[i].size(); j++)
		{
			pathGrid[i][j]->inOpenList = false;
			pathGrid[i][j]->inClosedList = false;
			pathGrid[i][j]->isLowestPath = false;
		}
	}

	openList.clear();
	closedList.clear();
}

void AStarPathFinder::printPath()
{
	for (unsigned int i = 0; i < pathGrid.size(); i++)
	{
		for (unsigned int j = 0; j < pathGrid[i].size(); j++)
		{
			if (pathGrid[i][j] == start)
				printf("S");
			else if (pathGrid[i][j] == end)
				printf("E");
			else if (pathGrid[i][j]->isLowestPath)
				printf("x");
			else if (pathGrid[i][j]->isWalkable)
				printf("0");
			else
				printf("1");

		}
		printf("\n");
	}
}
