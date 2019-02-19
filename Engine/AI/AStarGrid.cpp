#include "AStarGrid.h"

#include "Game\Game.h"
#include "Graphics\Effects\DebugDrawManager.h"
#include "Program\Log.h"

#include "Program\Utils.h"

#include "include\glm\gtc\matrix_transform.hpp"
#include "include\glm\gtx\norm.hpp"

#include <iostream>
//#include <chrono>

namespace Engine
{
	AStarGrid::AStarGrid()
	{
		grid = nullptr;
		gridSize = glm::vec2(0.0f);
		gridCenter = glm::vec2(0.0f);
	}

	AStarGrid::~AStarGrid()
	{
	}

	void AStarGrid::Init(Game *game, const glm::vec2 &gridCenter, const glm::vec2 &gridSize, float nodeRadius)
	{
		if (isInit)
			return;

		this->game = game;
		this->gridSize = gridSize;
		this->nodeRadius = nodeRadius;
		nodeDiameter = nodeRadius * 2.0f;

		// Calculate how many nodes can fit in the given grid size
		gridSizeXZ.x = (int)glm::round(gridSize.x / nodeDiameter);
		gridSizeXZ.y = (int)glm::round(gridSize.y / nodeDiameter);

		totalGridNodes = static_cast<unsigned int>(gridSizeXZ.x * gridSizeXZ.y);

		if (!grid)
			grid = new AStarNode[totalGridNodes];

		isInit = true;
	}

	void AStarGrid::Update()
	{
		if (!isBuilt)
		{
			RebuildGrid(game, gridCenter);

			if (isBuilt)
				std::cout << "Done building grid\n";
		}
	}

	void AStarGrid::Dispose()
	{
		if (isInit)
		{
			if (grid)
			{
				delete[] grid;
				grid = nullptr;
			}

		}

		isInit = false;
	}

	void AStarGrid::RebuildGrid(Game *game, const glm::vec2 &newGridCenter)
	{
		//std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();

		this->gridCenter = newGridCenter;
		gridCenterI = glm::ivec2(static_cast<int>(gridCenter.x), static_cast<int>(gridCenter.y));
		glm::vec2 gridBottomLeft = gridCenter - glm::vec2(gridSize.x * 0.5f, gridSize.y * 0.5f);

		Terrain *terrain = game->GetTerrain();

		if (rebuildStartIndexX >= gridSizeXZ.x)
		{
			rebuildStartIndexX = 0;
			rebuildStopIndexX = 0;
			rebuildStartIndexZ++;
			rebuildStopIndexZ++;
		}

		if (rebuildStartIndexZ >= gridSizeXZ.y)
		{
			rebuildStartIndexZ = 0;
			isBuilt = true;
		}

		for (int z = rebuildStartIndexZ; z < rebuildStopIndexZ; z++)
		{
			for (int x = rebuildStartIndexX; x < rebuildStopIndexX; x++)
			{
				glm::vec2 worldPos = gridBottomLeft + glm::vec2(x * nodeDiameter + nodeRadius, z * nodeDiameter + nodeRadius);

				float height = 0.0f;
				if (terrain)
					height = terrain->GetHeightAt((int)worldPos.x, (int)worldPos.y);

				bool walkable = !game->GetPhysicsManager().CheckSphere(btVector3(worldPos.x, height, worldPos.y), nodeRadius);		// Returns true if there is anything overlapping the sphere		

				grid[gridIndex].parent = nullptr;
				grid[gridIndex].worldPos = worldPos;
				grid[gridIndex].gridPos = glm::ivec2(x, z);
				grid[gridIndex].hCost = 0;
				grid[gridIndex].gCost = 0;
				grid[gridIndex].walkable = walkable;
				grid[gridIndex].heapIndex = 0;

				gridIndex++;
				rebuildStartIndexX++;
			}
		}

		rebuildStopIndexX += nodesRebuiltPerFrame;

		/*std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();

		auto duration = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
		std::cout << "Grid rebuild time: " << duration << " microseconds\n";*/
	}

	void AStarGrid::RebuildImmediate(Game *game, const glm::vec2 &newGridCenter)
	{
		this->gridCenter = newGridCenter;
		gridCenterI = glm::ivec2(static_cast<int>(gridCenter.x), static_cast<int>(gridCenter.y));
		glm::vec2 gridBottomLeft = gridCenter - glm::vec2(gridSize.x * 0.5f, gridSize.y * 0.5f);

		int i = 0;

		Terrain *terrain = game->GetTerrain();

		for (int z = 0; z < gridSizeXZ.y; z++)
		{
			for (int x = 0; x < gridSizeXZ.x; x++)
			{
				glm::vec2 worldPos = gridBottomLeft + glm::vec2(x * nodeDiameter + nodeRadius, z * nodeDiameter + nodeRadius);

				float height = 0.0f;
				if (terrain)
					height = terrain->GetHeightAt((int)worldPos.x, (int)worldPos.y);

				bool walkable = !game->GetPhysicsManager().CheckSphere(btVector3(worldPos.x, height, worldPos.y), nodeRadius);		// Returns true if there is anything overlapping the sphere		

				AStarNode &node = grid[i];
				node.parent = nullptr;
				node.worldPos = worldPos;
				node.gridPos = glm::ivec2(x, z);
				node.hCost = 0;
				node.gCost = 0;

				// Prevent the node from being overwritten if there's is no physics object there but there is a tree/rock
				// But this will cause problems if we were to move a physics object and rebuild the grid again, the node would not be updated
				if(node.walkable)
					node.walkable = walkable;
				node.heapIndex = 0;

				i++;
			}
		}

		/*const std::vector<Vegetation> &vegetation = terrain->GetVegetation();
		const std::vector<ModelInstanceData> &vegInsData = terrain->GetVegetationInstanceData();

		for (size_t i = 0; i < vegetation.size(); i++)
		{
			const Vegetation &v = vegetation[i];
			if (!v.generateObstacles || v.count == 0)
				continue;

			for (size_t j = v.offset; j < v.offset + v.count; j++)
			{
				const glm::vec3 &pos = vegInsData[j].modelMatrix[3];
				
				AStarNode *n = NodeFromWorldPos(glm::vec2(pos.x, pos.z));
				if (n)
				{
					n->walkable = false;

					float x = std::abs(pos.x - n->worldPos.x);
					float z = std::abs(pos.z - n->worldPos.y);

					if (x > 0.3f || z > 0.3f)
						continue;
				}

				n = NodeFromWorldPos(glm::vec2(pos.x - 1.0f, pos.z));
				if (n)
					n->walkable = false;

				n = NodeFromWorldPos(glm::vec2(pos.x + 1.0f , pos.z));
				if (n)
					n->walkable = false;

				n = NodeFromWorldPos(glm::vec2(pos.x, pos.z - 1.0f));
				if (n)
					n->walkable = false;

				n = NodeFromWorldPos(glm::vec2(pos.x, pos.z + 1.0f));
				if (n)
					n->walkable = false;
			}
		}*/
	}

	void AStarGrid::SetNeedsRebuild(bool needsRebuild)
	{
		if (needsRebuild)
		{
			isBuilt = false;
			gridIndex = 0;
		}
	}

	void AStarGrid::SetGridCenter(const glm::vec2 &center)
	{
		gridCenter = center;
		gridCenterI = glm::ivec2(static_cast<int>(gridCenter.x), static_cast<int>(gridCenter.y));
	}

	void AStarGrid::PrepareDebugDraw()
	{
		glm::vec3 camPos = game->GetMainCamera()->GetPosition();

		glm::mat4 m;
		glm::vec3 pos;

		Terrain *terrain = game->GetTerrain();

		for (size_t i = 0; i < totalGridNodes; i++)
		{
			const AStarNode &node = grid[i];

			float height = 0.0f;
			if (terrain)
				height = game->GetTerrain()->GetHeightAt((int)node.worldPos.x, (int)node.worldPos.y);

			pos.x = node.worldPos.x;
			pos.y = height;
			pos.z = node.worldPos.y;

			if (glm::length2(pos - camPos) > 400.0f)
				continue;

			m = glm::translate(glm::mat4(1.0f), pos);
			m = glm::scale(m, glm::vec3(nodeDiameter * 0.7f));		// Scale it down a bit to be able to distingush the cubes

			/*if (std::find(path.begin(), path.end(), &grid[i]) != path.end())
				game->GetDebugDrawManager()->AddCube(m, glm::vec3(1.0f, 0.0f, 1.0f));
			else */if (grid[i].walkable)
				game->GetDebugDrawManager()->AddCube(m);
			else
				game->GetDebugDrawManager()->AddCube(m, glm::vec3(1.0f, 0.0f, 0.0f));
		}
	}

	bool AStarGrid::FindPath(const glm::vec2 &startPos, const glm::vec2 &targetPos, std::vector<glm::vec2> &nodeWaypoints, int maxSearch)
	{
		AStarNode *startNode = NodeFromWorldPos(startPos);
		AStarNode *targetNode = NodeFromWorldPos(targetPos);
		if (!targetNode || !startNode)										// Check for null because the target (player) could be outside the grid
			return false;

		//AStarNodeHeap openSet(gridSizeXZ.x * gridSizeXZ.y);		// Here we were allocating 171000 nodes per object that requested a path
		// Because the path are relatively short we don't need that many nodes
		AStarNodeHeap openSet(100);		// Possible moves. Only allocate enough nodes for the space around what requested the path. 171000 * 0.0006 = ~102
		std::vector<AStarNode*> closedSet;						// Searched nodes

		openSet.Add(startNode);

		int i = 0;
		while (openSet.Size() > 0)
		{
			// At the beginning we only have one node
			AStarNode *currentNode = openSet.RemoveFirst();
			closedSet.push_back(currentNode);

			// Limit the search for the path. Useful for when were always requesting a path. But be careful to not limit too early otherwise the path might end very different from the real path (like starting to go in the wrong direction)
			if (i >= maxSearch)
			{
				RetracePath(startNode, currentNode, nodeWaypoints);
				//std::cout << "returning early path\n";
				return true;
			}
			
			if (currentNode == targetNode)		// Path found
			{
				RetracePath(startNode, targetNode, nodeWaypoints);
				//std::cout << "returning full path\n";
				return true;
			}

			std::forward_list<AStarNode*> neighbours = GetNeighbours(currentNode);

			// If no path is found, loop through all the node neighbours
			for (AStarNode *neighbour : neighbours)
			{
				// If the neighbour is not walkable or was already searched, skip to the next neighbour
				if (!neighbour->walkable || std::find(closedSet.begin(), closedSet.end(), neighbour) != closedSet.end())
				{
					continue;
				}

				int newMoveCostToNeighbour = currentNode->gCost + GetDistance(currentNode, neighbour);

				bool notInOpenSet = !openSet.Contains(neighbour);

				// If the new path to the neighbour is shorter than the old path or the neighbour is not in the possible moves list
				if (newMoveCostToNeighbour < neighbour->gCost || notInOpenSet)
				{
					neighbour->gCost = newMoveCostToNeighbour;					// Distance from starting node
					neighbour->hCost = GetDistance(neighbour, targetNode);		// Distance to the target node
					neighbour->parent = currentNode;

					// Add the neighbour as a possible move if it isn't the list
					if (notInOpenSet)
						openSet.Add(neighbour);
					else
						openSet.Update(neighbour);
				}
			}
			i++;
		}

		return false;
	}

	void AStarGrid::UpdateNode(const glm::vec2 &worldPos, bool walkable)
	{
		AStarNode *n = NodeFromWorldPos(worldPos);
		if (n)
			n->walkable = walkable;
	}

	AStarNode *AStarGrid::NodeFromWorldPos(const glm::vec2 &pos)
	{
		// 0 on the left, 0.5 on the middle and 1 on the right
		float percentX = (pos.x - gridCenter.x) / gridSize.x + 0.5f;		// Optimization
		float percentZ = (pos.y - gridCenter.y) / gridSize.y + 0.5f;

		if (percentX > 1.0f || percentX < 0.0f || percentZ > 1.0f || percentZ < 0.0f)
			return nullptr;

		percentX = glm::clamp(percentX, 0.0f, 1.0f);
		percentZ = glm::clamp(percentZ, 0.0f, 1.0f);

		int x = (int)glm::round((gridSizeXZ.x - 1) * percentX);
		int z = (int)glm::round((gridSizeXZ.y - 1) * percentZ);

		return &grid[z * gridSizeXZ.x + x];
	}

	std::forward_list<AStarNode*> AStarGrid::GetNeighbours(AStarNode *node)
	{
		std::forward_list<AStarNode*> neighbours;

		for (int z = -1; z <= 1; z++)
		{
			for (int x = -1; x <= 1; x++)
			{
				if (x == 0 && z == 0)			// x=0 y=0 is the center node so skip it
					continue;

				int checkX = node->gridPos.x + x;			// Position is from -gridSize to gridSize so add half gridSize to become 0 to gridSize * 2
				int checkZ = node->gridPos.y + z;
				
				if (checkX >= 0 && checkX < gridSizeXZ.x && checkZ >= 0 && checkZ < gridSizeXZ.y)
				{
					neighbours.push_front(&grid[checkZ * gridSizeXZ.x + checkX]);
				}
			}
		}

		return neighbours;
	}

	int AStarGrid::GetDistance(AStarNode *nodeA, AStarNode *nodeB)
	{
		int distX = glm::abs(nodeA->gridPos.x - nodeB->gridPos.x);
		int distZ = glm::abs(nodeA->gridPos.y - nodeB->gridPos.y);

		if (distX > distZ)
			return 14 * distZ + 10 * (distX - distZ);

		return 14 * distX + 10 * (distZ - distX);
	}

	void AStarGrid::RetracePath(AStarNode *startNode, AStarNode *targetNode, std::vector<glm::vec2> &nodeWaypoints)
	{
		path.clear();

		AStarNode *currentNode = targetNode;

		while (currentNode != startNode)		// Because the nodes are pointers we can just compare the address
		{
			path.push_back(currentNode);
			currentNode = currentNode->parent;
		}

		SimplifyPath(nodeWaypoints);
	}

	void AStarGrid::SimplifyPath(std::vector<glm::vec2> &nodeWaypoints)
	{
		nodeWaypoints.clear();

		glm::vec2 oldDir = glm::vec2();

		for (size_t i = 1; i < path.size(); i++)
		{
			glm::vec2 newDir = glm::vec2(path[i - 1]->gridPos.x - path[i]->gridPos.x, path[i - 1]->gridPos.y - path[i]->gridPos.y);

			if (newDir != oldDir)
			{
				nodeWaypoints.push_back(path[i]->worldPos);
				//Log::Vec2(path[i]->worldPos);
			}

			oldDir = newDir;
		}
	}

	void AStarGrid::SaveGridToFile()
	{
		Serializer s;
		s.OpenForWriting();

		s.Write(totalGridNodes);

		for (size_t i = 0; i < totalGridNodes; i++)
		{
			AStarNode &node = grid[i];
			s.Write(node.worldPos);
			s.Write(node.walkable);
		}

		s.Save(game->GetProjectDir() + "aigrid.data");
		s.Close();

		std::cout << "Saved ai grid file\n";
	}

	void AStarGrid::LoadGridFromFile()
	{
		Serializer s;
		s.OpenForReading(game->GetProjectDir() + "aigrid.data");

		if (s.IsOpen())
		{
			unsigned int gridSize = 0;
			s.Read(gridSize);
			
			if (totalGridNodes == gridSize)		// Only load if the saved grid has the same size as the one that is resized on Init()
			{
				int i = 0;
				for (int z = 0; z < gridSizeXZ.y; z++)
				{
					for (int x = 0; x < gridSizeXZ.x; x++)
					{
						AStarNode &node = grid[i];
						s.Read(node.worldPos);
						s.Read(node.walkable);

						node.gCost = 0;
						node.hCost = 0;
						node.heapIndex = 0;
						node.parent = nullptr;
						node.gridPos = glm::ivec2(x, z);

						i++;
					}
				}
			}
			else
			{
				LoadDefaultGrid();
			}

			std::cout << "Loaded ai grid file\n";
		}
		else
		{
			LoadDefaultGrid();
		}

		s.Close();
	}

	void AStarGrid::LoadDefaultGrid()
	{
		int i = 0;
		for (int z = 0; z < gridSizeXZ.y; z++)
		{
			for (int x = 0; x < gridSizeXZ.x; x++)
			{
				AStarNode &node = grid[i];
				node.worldPos = glm::vec2(static_cast<float>(x), static_cast<float>(z));
				node.walkable = true;
				node.gCost = 0;
				node.hCost = 0;
				node.heapIndex = 0;
				node.parent = nullptr;
				node.gridPos = glm::ivec2(x, z);

				i++;
			}
		}
	}
}
