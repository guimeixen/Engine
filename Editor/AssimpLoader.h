#pragma once

#include "Physics/BoundingVolumes.h"
#include "Graphics/Mesh.h"
#include "Program/Serializer.h"

#include <vector>

struct aiScene;
struct aiMesh;
struct aiMaterial;
struct aiNode;
struct aiAnimation;

namespace Engine
{
	class Game;
	class Renderer;
	class ScriptManager;
	struct MaterialInstance;
	class Model;
	class AnimatedModel;
	struct Bone;
	struct Animation;

	class AssimpLoader
	{
	public:

		// Loads the model with Assimp and saves it in the custom model format
		static void LoadModel(Game *game, const std::string &path, const std::vector<std::string> &matPaths, bool willUseInstancing = false, bool isAnimated = false, bool loadVertexColors = false);
		// Returns a string with the new path of the model in the custom format
		static void LoadAnimatedModel(Game *game, const std::string &path, const std::vector<std::string> &matPaths);
		// Returns a new animation and also saves it in the custom animation format
		static void LoadSeparateAnimation(FileManager *fileManager, const std::string &path, const std::string &newAnimPath);

	private:
		static Mesh ProcessMesh(Renderer *renderer, Serializer &s, Model *model, const aiScene *aiscene, const aiMesh *aimesh, bool willUseInstancing, bool loadVertexColors);
		static Mesh ProcessAnimatedMesh(Renderer *renderer, Serializer &s, AnimatedModel *am, const aiScene *aiscene, const aiMesh *aimesh);
		static MaterialInstance *LoadMaterialFromAssimpMat(Renderer *renderer, ScriptManager *scriptManager, const std::string &modelPath, const Mesh &mesh, const aiMaterial *aimat, bool isAnimated);
		static void BuildBoneTree(aiNode *node, Bone *parent);
		static void StoreAnimation(Animation *a, aiAnimation *anim);
		static void SerializeBoneTree(Serializer &s, Bone *bone);
		static void SerializeAnimation(FileManager *fileManager, Animation *a);
	};
}