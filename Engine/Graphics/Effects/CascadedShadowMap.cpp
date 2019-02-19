#include "CascadedShadowMap.h"

#include "Graphics\Camera\Camera.h"

#include "include\glm\gtc\matrix_transform.hpp"

namespace Engine
{
	namespace CascadedShadowMap
	{
		void Update(CSMInfo &csmInfo, Camera &camera, const glm::vec3 &lightDir)
		{
			glm::vec3 frustumCorners[8];

			float near = camera.GetNearPlane();
			float far = camera.GetFarPlane();

			float tang = glm::tan(glm::radians(camera.GetFov() * 0.5f));
			float nh = near * tang;
			float nw = nh * camera.GetAspectRatio();
			float fh = far  * tang;
			float fw = fh * camera.GetAspectRatio();

			const glm::vec3 &Z = camera.GetFront();
			const glm::vec3 &X = camera.GetRight();
			const glm::vec3 &Y = camera.GetUp();

			glm::vec3 nc = camera.GetPosition() + Z * near;
			glm::vec3 fc = camera.GetPosition() + Z * far;

			for (size_t i = 0; i < CASCADE_COUNT; i++)
			{
				float prevSplitDist = i == 0 ? 0.0f : csmInfo.cascadeSplitEnd[i - 1];
				float splitDist = csmInfo.cascadeSplitEnd[i];

				frustumCorners[0] = nc + Y * nh - X * nw;		// near top left
				frustumCorners[1] = nc + Y * nh + X * nw;		// near top right
				frustumCorners[2] = nc - Y * nh - X * nw;		// near bottom left
				frustumCorners[3] = nc - Y * nh + X * nw;		// near bottom right

				frustumCorners[4] = fc + Y * fh - X * fw;		// far top left
				frustumCorners[5] = fc + Y * fh + X * fw;		// far top right
				frustumCorners[6] = fc - Y * fh - X * fw;		// far bottom left
				frustumCorners[7] = fc - Y * fh + X * fw;		// far bottom right

				// Get the corners of the current cascade
				for (int j = 0; j < 4; j++)
				{
					// Describe this better
					// Ray from near corner to the far corner
					const glm::vec3 cornerRay = frustumCorners[j + 4] - frustumCorners[j];
					const glm::vec3 nearCornerRay = cornerRay * prevSplitDist;
					const glm::vec3 farCornerRay = cornerRay * splitDist;
					frustumCorners[j + 4] = frustumCorners[j] + farCornerRay;
					frustumCorners[j] = frustumCorners[j] + nearCornerRay;
				}

				// Calculate the center of the current cascade
				glm::vec3 frustumCenter = glm::vec3(0.0f);
				for (int j = 0; j < 8; j++)
				{
					frustumCenter += frustumCorners[j];
				}

				frustumCenter /= 8.0f;

				// This needs to be constant for it to be stable
				const glm::vec3 upDir = glm::vec3(0.0f, 1.0f, 0.0f);

				// Calculating a tight bounding box would cause shadows artifacts (like flicker) when the camera rotates or moves so we calculate a bounding sphere
				// Calculate the radius of a bounding sphere surrounding the frustum corners
				float sphereRadius = 0.0f;
				for (int j = 0; j < 8; j++)
				{
					float dist = glm::length(frustumCorners[j] - frustumCenter);
					sphereRadius = glm::max(sphereRadius, dist);
				}

				sphereRadius = glm::ceil(sphereRadius * 16.0f) / 16.0f;

				glm::vec3 maxExtents = glm::vec3(sphereRadius);
				glm::vec3 minExtents = -maxExtents;

				glm::vec3 cascadeExtents = maxExtents - minExtents;

				// Calculate the position of the shadow camera
				glm::vec3 shadowCameraPos = frustumCenter + lightDir * -minExtents.z;

				glm::mat4 shadowProj = glm::ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, -cascadeExtents.z, cascadeExtents.z);
				glm::mat4 shadowView = glm::lookAt(shadowCameraPos, frustumCenter, upDir);

				csmInfo.csmAABB[i].min = glm::vec3(minExtents.x + shadowCameraPos.x, minExtents.y + shadowCameraPos.y, -cascadeExtents.z + shadowCameraPos.z);
				csmInfo.csmAABB[i].max = glm::vec3(maxExtents.x + shadowCameraPos.x, maxExtents.y + shadowCameraPos.y, cascadeExtents.z + shadowCameraPos.z);

				// To stop shadow artifacts by moving or rotating the camera we move in texel sized increments
				// Create the rounding matrix, by projecting the world-space origin and determining the fractional offset in texel space
				glm::mat4 temp = shadowProj * shadowView;
				glm::vec4 shadowOrigin = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
				shadowOrigin = temp * shadowOrigin;

				// Find nearest shadow map texel. The 0.5f is because x,y are in the 
				// range [-1,1] and we need them in the range [0,1]
				shadowOrigin = shadowOrigin * (SHADOW_MAP_RES * 0.5f);

				// Round to the nearest 'whole' texel	
				glm::vec4 roundedOrigin = glm::round(shadowOrigin);

				// Fractional offset. We just want to affect the x  (left, right) and y (top, bottom) so set z (near, far) and w to 0 because we don't want to add offset to those. Also w is always 1 and
				// doesn't matter for orthographic projections

				// The difference between the rounded and actual tex coordinate is the 
				// amount by which we need to translate the shadow matrix in order to
				// cancel sub-texel movement
				glm::vec4 roundOffset = roundedOrigin - shadowOrigin;

				// Transform the offset back to homogenous light space [-1,1]
				roundOffset = roundOffset * (2.0f / SHADOW_MAP_RES);
				roundOffset.z = 0.0f;
				roundOffset.w = 0.0f;

				// Finally add the offset to the 3rd column of the ortho matrix
				shadowProj[3] += roundOffset;

				csmInfo.viewProjLightSpace[i] = shadowProj * shadowView;

				csmInfo.cameras[i].SetProjectionMatrix(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, -cascadeExtents.z, cascadeExtents.z);		// Store frustum related variables
				csmInfo.cameras[i].SetProjectionMatrix(shadowProj);																							// Set the correct projection matrix
				csmInfo.cameras[i].SetViewMatrix(shadowView);
				csmInfo.cameras[i].UpdateFrustum(shadowCameraPos, frustumCenter, upDir);
			}
		}
	}
}
