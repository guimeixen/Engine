#pragma once

#include "EditorWindow.h"
#include "Program\Serializer.h"
#include "Graphics\Animation\AnimatedModel.h"

#include "imgui\imgui.h"

#include <string>

class EditorLink
{
public:
	EditorLink();

	bool CheckIntersection() const;
	void Render();

	void SetFromNodePos(const ImVec2 &from, unsigned int fromID) { this->from = from; this->fromID = fromID; }
	void SetToNodePos(const ImVec2 &to, unsigned int toID) { this->to = to; this->toID = toID; }
	void SetOffset(float offset) { this->offset = offset; }

	unsigned int GetFromID() const { return fromID; }
	unsigned int GetToID() const { return toID; }

	void Serialize(Engine::Serializer &s);
	void Deserialize(Engine::Serializer &s);

private:
	unsigned int fromID;
	unsigned int toID;
	ImVec2 from;
	ImVec2 to;
	float offset;
	ImVec2 right;
};

class EditorAnimNode
{
public:
	EditorAnimNode();

	void Render(ImU32 col);
	bool ContainsPoint(const ImVec2 &point);

	void AddLink(const EditorLink &link) { editorLinks.push_back(link); }

	void SetPos(const ImVec2 &pos) { this->pos = pos; }
	const ImVec2 &GetPos() const { return pos; }

	const ImVec2 &GetSize() const { return size; }

	void SetName(const std::string &name) { this->name = name; }
	const std::string &GetName() const { return name; }

	std::vector<EditorLink> &GetFromLinks() { return editorLinks; }

	void Serialize(Engine::Serializer &s);
	void Deserialize(Engine::Serializer &s);

private:
	ImVec2 pos;
	ImVec2 size;
	std::string name;
	std::vector<EditorLink> editorLinks;			// Links that go out of this node
};

class AnimationWindow : public EditorWindow
{
public:
	AnimationWindow();

	void Render();

	void OpenAnimationController(const std::string &path);

	unsigned int GetCurrentNodeID() const { return currentNodeID; }
	unsigned int GetCurrentLinkID() const { return currentLinkID; }
	Engine::AnimationController *GetCurrentAnimationController() const { return curAnimController; }

private:
	void HandleNodeCreation();
	void HandleLinkIntersection();
	void HandleLinkPositioning();
	void HandleNodeIntersection();
	void HandleNodeDrag();
	void HandleNodeContextPopup();

	void ResetName();

	void SaveEditorAnimationController(const std::string &path);
	void LoadEditorAnimationController(const std::string &path);
	
private:
	bool isControllerCreated = false;
	bool isControllerLoaded = false;
	std::vector<std::string> files;

	Engine::Entity curEntity;
	Engine::AnimationController *curAnimController;
	std::string curAnimContPath;

	std::vector<EditorAnimNode> editorNodes;
	unsigned int currentNodeID = 0;
	int currentLinkID = -1;
	bool firstClick	= true;
	bool isDragging	= false;
	int nodeAnimIndex = 0;
	bool looped = true;
	bool showingNodeContext = false;

	ImVec2 oldMousePos;
	std::vector<std::string> animNames;
	std::vector<unsigned int> animIDs;
	
	bool creatingLink = false;
	char name[128];
};

