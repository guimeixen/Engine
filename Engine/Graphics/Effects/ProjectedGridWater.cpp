#include "ProjectedGridWater.h"

#include "Graphics/MeshDefaults.h"
#include "Graphics/VertexArray.h"
#include "Graphics/Renderer.h"
#include "Graphics/Material.h"

#include "Program/Log.h"

#include "include/glm/gtc\matrix_access.hpp"
#include "include/glm/gtc\matrix_transform.hpp"

#include <iostream>

namespace Engine
{
	ProjectedGridWater::ProjectedGridWater()
	{
		normalMap = nullptr;
		foamTexture = nullptr;
		projectedGridMesh = {};
		normalMapOffset0 = glm::vec2(0.0f);
		normalMapOffset1 = glm::vec2(0.0f);
		waterHeight = 0.0f;
	}

	void ProjectedGridWater::Init(Renderer *renderer, ScriptManager &scriptManager, float waterHeight)
	{
		SetWaterHeight(waterHeight);

		projectedGridMesh = MeshDefaults::CreateScreenSpaceGrid(renderer, 128);

		mat = renderer->CreateMaterialInstanceFromBaseMat(scriptManager, "Data/Resources/Materials/water_disp_mat.lua", projectedGridMesh.vao->GetVertexInputDescs());
		TextureParams params = { TextureWrap::REPEAT, TextureFilter::LINEAR,TextureFormat::RGBA, TextureInternalFormat::RGBA8, TextureDataType::UNSIGNED_BYTE, true, false };
		normalMap = renderer->CreateTexture2D("Data/Resources/Textures/oceanwaves_ddn2.png", params);
		mat->textures[1] = normalMap;	
		foamTexture = renderer->CreateTexture2D("Data/Resources/Textures/foam.png", params);
		mat->textures[4] = foamTexture;

		renderer->UpdateMaterialInstance(mat);
	}

	void ProjectedGridWater::Update(Camera *camera, float deltaTime)
	{
		intersectionPoints.clear();

		/*IntersectFrustumWithWaterPlane(camera);

		if (intersectionPoints.size() > 0)
		{
			glm::vec3 newCamPos = camera->GetPosition();

			float minCamHeight = waterHeight + 1.315f + 10.0f;
			newCamPos.y = std::max(minCamHeight, newCamPos.y);

			glm::vec3 oldCamPos = camera->GetPosition();
			glm::vec3 focus = oldCamPos + camera->GetFront() * 5.0f;

			// Construct view frame
			glm::vec3 viewFrameZ = glm::normalize(focus - newCamPos);
			glm::vec3 viewFrameX = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), viewFrameZ));
			glm::vec3 viewFrameY = glm::normalize(glm::cross(viewFrameZ, viewFrameX));

			viewFrameY = oldCamPos.y > waterHeight ? viewFrameY : -viewFrameY;

			viewFrame[0] = glm::vec4(viewFrameX, 0.0f);
			viewFrame[1] = glm::vec4(viewFrameY, 0.0f);
			viewFrame[2] = glm::vec4(-viewFrameZ, 0.0f);
			viewFrame[3] = glm::vec4(newCamPos, 1.0f);

			// Construct view and projection matrices
			glm::mat4 invViewFrame = glm::inverse(viewFrame);
			glm::mat4 viewProj = camera->GetProjectionMatrix() * invViewFrame;

			// Project the intersection points of the frustum and water plane into ndc-space, as seen from the camera
			std::vector<glm::vec3> ndcPoints(intersectionPoints.size());

			for (size_t i = 0; i < intersectionPoints.size(); i++)
			{
				glm::vec4 v = viewProj * glm::vec4(intersectionPoints[i], 1.0f);
				v.x /= v.w;
				v.y /= v.w;
				v.z /= v.w;
				ndcPoints[i] = glm::vec3(v.x, v.y, v.z);
			}

			// Create a matrix that maps ndc-points that are within the bounding box of the water plane in (ndc-space) to [0,1]
			glm::mat4 rangeMap;

			// Find ndc bounding box
			glm::vec3 min = ndcPoints[0];
			glm::vec3 max = ndcPoints[0];

			for (size_t i = 1; i < ndcPoints.size(); i++)
			{
				min = glm::min(min, ndcPoints[i]);
				max = glm::max(max, ndcPoints[i]);
			}

			glm::vec2 size = max - min;

			rangeMap = glm::row(rangeMap, 0, glm::vec4(1.0f / size.x, 0.0f, 0.0f, -(min.x / size.x)));
			rangeMap = glm::row(rangeMap, 1, glm::vec4(0.0f, 1.0f / size.y, 0.0f, -(min.y / size.y)));
			rangeMap = glm::row(rangeMap, 2, glm::vec4(0.0f, 0.0f, 1.0f, 0.0f));
			rangeMap = glm::row(rangeMap, 3, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));

			// The projector transform is from world-space to remapped ndc-space where only ndc-points inside the bounding box of the water plane lies within [0,1]
			glm::mat4 toProjectorSpace = rangeMap * viewProj;
			glm::mat4 fromProjectorSpace = glm::inverse(toProjectorSpace);

			// Find out where the corners of the bounding box of the water plane intersect the water plane
			glm::vec2 corners[4];
			corners[0] = glm::vec2(0.0f, 0.0f);
			corners[1] = glm::vec2(1.0f, 0.0f);
			corners[2] = glm::vec2(1.0f, 1.0f);
			corners[3] = glm::vec2(0.0f, 1.0f);

			for (size_t i = 0; i < 4; i++)
			{
				glm::vec3 nearPoint = glm::vec3(corners[i].x, corners[i].y, 0.0f);		// -1.0f
				glm::vec3 farPoint = glm::vec3(corners[i].x, corners[i].y, 1.0f);

				// Start and end before division by w
				glm::vec4 s = fromProjectorSpace * glm::vec4(nearPoint, 1.0f);
				glm::vec4 e = fromProjectorSpace * glm::vec4(farPoint, 1.0f);

				// Division by w here
				glm::vec3 start = glm::vec3(s.x / s.w, s.y / s.w, s.z / s.w);
				glm::vec3 end = glm::vec3(e.x / e.w, e.y / e.w, e.z / e.w);

				glm::vec3 dir = glm::normalize(end - start);

				float d = 0.0f;
				waterBottomPlane.IntersectRay(start, dir, d);
				glm::vec3 hit = start + dir * d;

				viewCorners[i] = invViewFrame * glm::vec4(hit, 1.0f);
				viewCorners[i].x /= viewCorners[i].w;
				viewCorners[i].y /= viewCorners[i].w;
				viewCorners[i].z /= viewCorners[i].w;
			}
		}*/


		glm::vec3 camPos = camera->GetPosition();

		float range = std::max(0.0f, 10.0f) + 5.0f;

		if (camPos.y < waterHeight)
			camPos.y = std::min(camPos.y, waterHeight - range);
		else
			camPos.y = std::max(camPos.y, waterHeight + range);

		const float scale = 18.0f;
		glm::vec3 focus = camera->GetPosition() + camera->GetFront() * scale;
		focus.y = waterHeight;

		viewFrame = glm::lookAt(camPos, focus, glm::vec3(0.0f, 1.0f, 0.0f));

		// Construct view and projection matrices
		glm::mat4 projectorViewProj = camera->GetProjectionMatrix() * viewFrame;

		const FrustumCorners &frustumCorners = camera->GetFrustum().GetCorners();
		frustumCornersWorld[0] = frustumCorners.nbl;
		frustumCornersWorld[1] = frustumCorners.ntl;
		frustumCornersWorld[2] = frustumCorners.ntr;
		frustumCornersWorld[3] = frustumCorners.nbr;

		frustumCornersWorld[4] = frustumCorners.fbl;
		frustumCornersWorld[5] = frustumCorners.ftl;
		frustumCornersWorld[6] = frustumCorners.ftr;
		frustumCornersWorld[7] = frustumCorners.fbr;

		range = std::max(1.0f, 10.0f);

		// For each corner if its world space position is  
		// between the wave range then add it to the list.
		for (size_t i = 0; i < 8; i++)
		{
			if (frustumCornersWorld[i].y <= waterHeight + range && frustumCornersWorld[i].y >= waterHeight - range)
			{
				intersectionPoints.push_back(frustumCornersWorld[i]);
			}
		}

		static const int frustumIndices[12][2] = {
			{ 0,1 },
		{ 1,2 },
		{ 2,3 },
		{ 3,0 },
		{ 4,5 },
		{ 5,6 },
		{ 6,7 },
		{ 7,4 },
		{ 0,4 },
		{ 1,5 },
		{ 2,6 },
		{ 3,7 }
		};

		// Now take each segment in the frustum box and check
		// to see if it intersects the ocean plane on both the
		// upper and lower ranges.
		for (size_t i = 0; i < 12; i++)
		{
			glm::vec3 p0 = frustumCornersWorld[frustumIndices[i][0]];
			glm::vec3 p1 = frustumCornersWorld[frustumIndices[i][1]];

			glm::vec3 max, min;
			if (SegmentPlaneIntersection(p0, p1, glm::vec3(0.0f, 1.0f, 0.0f), waterHeight + range, max))
			{
				intersectionPoints.push_back(max);
			}

			if (SegmentPlaneIntersection(p0, p1, glm::vec3(0.0f, 1.0f, 0.0f), waterHeight - range, min))
			{
				intersectionPoints.push_back(min);
			}
		}

		float xmin = std::numeric_limits<float>::max();
		float ymin = std::numeric_limits<float>::max();
		float xmax = std::numeric_limits<float>::min();
		float ymax = std::numeric_limits<float>::min();
		glm::vec4 q = glm::vec4(0.0f);
		glm::vec4 p = glm::vec4(0.0f);

		// Now convert each world space position into
		// projector screen space. The min/max x/y values
		// are then used for the range conversion matrix.
		// Calculate the x and y span of vVisible in projector space
		for (size_t i = 0; i < intersectionPoints.size(); i++)
		{
			// Project the points of intersection between the frustum and the waterTop or waterBottom plane to the waterPlane
			q.x = intersectionPoints[i].x;
			q.y = waterHeight;
			q.z = intersectionPoints[i].z;
			q.w = 1.0f;

			// Now transform the points to projector space
			p = projectorViewProj * q;
			p.x /= p.w;
			p.y /= p.w;

			if (p.x < xmin) xmin = p.x;
			if (p.y < ymin) ymin = p.y;
			if (p.x > xmax) xmax = p.x;
			if (p.y > ymax) ymax = p.y;
		}

		// Create a matrix that transform the [0,1] range to [xmin,xmax] and [ymin,ymax] and leave the z and w intact
		glm::mat4 rangeMap;
		rangeMap = glm::row(rangeMap, 0, glm::vec4(xmax - xmin, 0.0f, 0.0f, xmin));
		rangeMap = glm::row(rangeMap, 1, glm::vec4(0.0f, ymax - ymin, 0.0f, ymin));
		rangeMap = glm::row(rangeMap, 2, glm::vec4(0.0f, 0.0f, 1.0f, 0.0f));
		rangeMap = glm::row(rangeMap, 3, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));

		// Now update the projector matrix with the range conversion matrix
		glm::mat4 projectorToWorld = glm::inverse(projectorViewProj) * rangeMap;

		glm::vec2 ndcCorners[4];
		ndcCorners[0] = glm::vec2(0.0f, 0.0f);
		ndcCorners[1] = glm::vec2(1.0f, 0.0f);
		ndcCorners[2] = glm::vec2(1.0f, 1.0f);
		ndcCorners[3] = glm::vec2(0.0f, 1.0f);

		// Now transform the corners of the 
		for (int i = 0; i < 4; i++)
		{
			glm::vec4 a, b;

			// Transform the ndc corners to world space
			a = projectorToWorld * glm::vec4(ndcCorners[i].x, ndcCorners[i].y, -1.0f, 1.0f);
			b = projectorToWorld * glm::vec4(ndcCorners[i].x, ndcCorners[i].y, 1.0f, 1.0f);

			// And calculate the intersection between the line made by this two points and the water plane
			// in homogeneous space
			// The rest of the grid vertices will then be interpolated in the vertex shader
			float h = waterHeight;

			glm::vec4 ab = b - a;

			float t = (a.w * h - a.y) / (ab.y - ab.w * h);

			viewCorners[i] = a + ab * t;
		}

		float nAngle0 = 42.0f * (3.14159f / 180.0f);
		float nAngle1 = 76.0f * (3.14159f / 180.0f);
		normalMapOffset0 += glm::vec2(glm::cos(nAngle0), glm::sin(nAngle0)) * 0.025f * deltaTime;
		normalMapOffset1 += glm::vec2(glm::cos(nAngle1), glm::sin(nAngle1)) * 0.010f * deltaTime;
	}

	void ProjectedGridWater::Render(Renderer *renderer)
	{
		if (intersectionPoints.size() > 0)
		{
			// Render the water
			RenderItem ri = {};
			ri.mesh = &projectedGridMesh;
			ri.matInstance = mat;
			ri.shaderPass = 0;
			renderer->Submit(ri);
		}
	}

	void ProjectedGridWater::Dispose()
	{
		if (normalMap)
			normalMap->RemoveReference();
		if (foamTexture)
			foamTexture->RemoveReference();

		if (projectedGridMesh.vao)
			delete projectedGridMesh.vao;

		Log::Print(LogLevel::LEVEL_INFO, "Disposing projected grid water\n");
	}

	void ProjectedGridWater::SetWaterHeight(float height)
	{
		this->waterHeight = height;
		waterBottomPlane.SetNormalAndPoint(glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, waterHeight, 0.0f));
		waterTopPlane.SetNormalAndPoint(glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, waterHeight + 1.315f, 0.0f));
	}

	void ProjectedGridWater::IntersectFrustumWithWaterPlane(Camera *camera)
	{
		const FrustumCorners &corners = camera->GetFrustum().GetCorners();

		IntersectFrustumEdgeWaterPlane(corners.nbl, corners.ntl);
		IntersectFrustumEdgeWaterPlane(corners.ntl, corners.ntr);
		IntersectFrustumEdgeWaterPlane(corners.ntr, corners.nbr);
		IntersectFrustumEdgeWaterPlane(corners.nbr, corners.nbl);

		IntersectFrustumEdgeWaterPlane(corners.fbl, corners.ftl);
		IntersectFrustumEdgeWaterPlane(corners.ftl, corners.ftr);
		IntersectFrustumEdgeWaterPlane(corners.ftr, corners.fbr);
		IntersectFrustumEdgeWaterPlane(corners.fbr, corners.fbl);

		IntersectFrustumEdgeWaterPlane(corners.nbl, corners.fbl);
		IntersectFrustumEdgeWaterPlane(corners.ntl, corners.ftl);
		IntersectFrustumEdgeWaterPlane(corners.ntr, corners.ftr);
		IntersectFrustumEdgeWaterPlane(corners.nbr, corners.fbr);
	}

	void ProjectedGridWater::IntersectFrustumEdgeWaterPlane(const glm::vec3 &start, const glm::vec3 &end)
	{
		glm::vec3 delta = end - start;
		glm::vec3 dir = glm::normalize(delta);
		float length = glm::length(delta);

		float distance = 0.0f;

		if (waterTopPlane.IntersectRay(start, dir, distance))
		{
			if (distance <= length)
			{
				glm::vec3 hitPos = start + dir * distance;

				intersectionPoints.push_back(glm::vec3(hitPos.x, waterHeight, hitPos.z));
			}
		}

		if (waterBottomPlane.IntersectRay(start, dir, distance))
		{
			if (distance <= length)
			{
				glm::vec3 hitPos = start + dir * distance;

				intersectionPoints.push_back(glm::vec3(hitPos.x, waterHeight, hitPos.z));
			}
		}
	}

	bool ProjectedGridWater::SegmentPlaneIntersection(const glm::vec3 &a, const glm::vec3 &b, const glm::vec3 &n, float d, glm::vec3 &q)
	{
		glm::vec3 ab = b - a;
		float t = (d - glm::dot(n, a)) / glm::dot(n, ab);

		if (t > -0.0f && t <= 1.0f)
		{
			q = a + t * ab;
			return true;
		}

		return false;
	}
}
