#version 450
#extension GL_GOOGLE_include_directive : enable
#include "include/ubos.glsl"

#define EDGE_THRESHOLD_MIN 0.0312
#define EDGE_THRESHOLD_MAX 0.125
#define ITERATIONS 12
#define SUBPIXEL_QUALITY 0.75

layout(location = 0) out vec4 color;

layout(location = 0) in vec2 uv;

tex_bind2D_user(0) postProcessedTexture;
//layout(set = 1, binding = 0) uniform sampler2D postProcessedTexture;

const float QUALITY[] = { 1.0, 1.0, 1.0, 1.0, 1.0, 1.5, 2.0, 2.0, 2.0, 2.0, 4.0, 8.0 };

float rgb2luma(vec3 rgb)
{
	return sqrt(dot(rgb, vec3(0.299, 0.587, 0.114)));
}

void main()
{
	color.a = 1.0;
	
	vec3 colorCenter = texture(postProcessedTexture, uv).rgb;
	
	// Luma at the current fragment
	float lumaCenter = rgb2luma(colorCenter);
	
	// Luma at the 4 direct neighbours
	float lumaDown = rgb2luma(textureOffset(postProcessedTexture, uv, ivec2(0, -1)).rgb);
	float lumaUp = rgb2luma(textureOffset(postProcessedTexture, uv, ivec2(0, 1)).rgb);
	float lumaLeft = rgb2luma(textureOffset(postProcessedTexture, uv, ivec2(-1, 0)).rgb);
	float lumaRight = rgb2luma(textureOffset(postProcessedTexture, uv, ivec2(1, 0)).rgb);

	// Find the maximum and mininum luma around the current fragment
	float lumaMin = min(lumaCenter, min(min(lumaDown, lumaUp), min(lumaLeft, lumaRight)));
	float lumaMax = max(lumaCenter, max(max(lumaDown, lumaUp), max(lumaLeft, lumaRight)));
	
	// The difference between the two gives us a local contrast value
	float lumaRange = lumaMax - lumaMin;
	
	// If the contrast is lower than a threshold proportional to the max luma or we are in a really dark area, we are not on an edge, so don't perform AA
	if (lumaRange < max(EDGE_THRESHOLD_MIN, lumaMax * EDGE_THRESHOLD_MAX))
	{
		color.rgb = colorCenter;
		return;
	}
	
	// Luma at the 4 remaining corners
	float lumaDownLeft = rgb2luma(textureOffset(postProcessedTexture, uv, ivec2(-1, -1)).rgb);
	float lumaDownRight = rgb2luma(textureOffset(postProcessedTexture, uv, ivec2(1, -1)).rgb);
	float lumaUpLeft = rgb2luma(textureOffset(postProcessedTexture, uv, ivec2(-1, 1)).rgb);
	float lumaUpRight = rgb2luma(textureOffset(postProcessedTexture, uv, ivec2(1, 1)).rgb);
	
	// Combine the 4 edge lumas
	float lumaDownUp = lumaDown + lumaUp;
	float lumaLeftRight = lumaLeft + lumaRight;
	
	// Do the same for the corners
	float lumaLeftCorners = lumaDownLeft + lumaUpLeft;
	float lumaDownCorners = lumaDownLeft + lumaDownRight;
	float lumaRightCorners = lumaDownRight + lumaUpRight;
	float lumaUpCorners = lumaUpLeft + lumaUpRight;
	
	// Compute an estimation of the gradient along the horizontal and vertical axis
	float edgeHorizontal = abs(-2.0 * lumaLeft + lumaLeftCorners) + abs(-2.0 * lumaCenter + lumaDownUp) * 2.0 + abs(-2.0 * lumaRight + lumaRightCorners);
	float edgeVertical = abs(-2.0 * lumaUp + lumaUpCorners) + abs(-2.0 * lumaCenter + lumaLeftRight) * 2.0 + abs(-2.0 * lumaDown + lumaDownCorners);
	
	bool isHorizontal = edgeHorizontal >= edgeVertical;
	
	// The current pixel is not necessarily exactly on the edge. We need to determine in which orientation, orthogonal to the edge direction, is the real edge border
	// Select the two neighbouring texels lumas in the opposite direction to the local edge
	float luma1 = isHorizontal ? lumaDown : lumaLeft;
	float luma2 = isHorizontal ? lumaUp : lumaRight;
	
	// Compute the gradients in this direction
	float gradient1 = luma1 - lumaCenter;
	float gradient2 = luma2 - lumaCenter;
	
	// Which direction is the steepest
	bool is1Steepest = abs(gradient1) >= abs(gradient2);
	
	// Gradient in the corresponding direction, normalized
	float gradientScaled = 0.25 * max(abs(gradient1), abs(gradient2));
	
	// Move by half a pixel in this direction, and compute the average luma at this point
	// Choose the step size(one pixel) according to the edge direction
	float stepLength = isHorizontal ? invScreenRes.y : invScreenRes.x;
	
	// Avg luma in the correct direction
	float lumaLocalAvg = 0.0;
	
	if (is1Steepest)
	{
		stepLength = -stepLength;
		lumaLocalAvg = 0.5 * (luma1 + lumaCenter);
	}
	else
	{
		lumaLocalAvg = 0.5 * (luma2 + lumaCenter);
	}
	
	// Shift the uv in the correct direction by half a pixel
	vec2 currentUV = uv;
	
	if (isHorizontal)
		currentUV.y += stepLength * 0.5;
	else
		currentUV.x += stepLength * 0.5;
		
	// Compute offset (for each iteration step) in the right direction
	vec2 offset = isHorizontal ? vec2(invScreenRes.x, 0.0) : vec2(0.0, invScreenRes.y);
	
	// Compute UVs to explore on each side of the edge, orthogonally.
	vec2 uv1 = currentUV - offset;
	vec2 uv2 = currentUV + offset;
	
	// Read the lumas at both extremities of the exploration segment, and compute the delta with respect to the local avg luma
	float lumaEnd1 = rgb2luma(texture(postProcessedTexture, uv1).rgb);
	float lumaEnd2 = rgb2luma(texture(postProcessedTexture, uv2).rgb);
	lumaEnd1 -= lumaLocalAvg;
	lumaEnd2 -= lumaLocalAvg;
	
	// If the luma deltas at the current extremities are larger than the local gradient, we have reached the side of the edge
	bool reached1 = abs(lumaEnd1) >= gradientScaled;
	bool reached2 = abs(lumaEnd2) >= gradientScaled;
	bool reachedBoth = reached1 && reached2;
	
	// If the side is not reached, we continue to explore in this direction
	if (!reached1)
		uv1 -= offset;
	
	if (!reached2)
		uv2 += offset;
	
	// If both sides have not been reached continue to explore
	if (!reachedBoth)
	{
		for (int i = 2; i < ITERATIONS; i++)
		{
			// If needed, read the luma in the 1st direction, compute delta
			if (!reached1)
			{
				lumaEnd1 = rgb2luma(texture(postProcessedTexture, uv1).rgb);
				lumaEnd1 -= lumaLocalAvg;
			}
			
			// If needed, read luma in opposite direction, compute delta
			if (!reached2)
			{
				lumaEnd2 = rgb2luma(texture(postProcessedTexture, uv2).rgb);
				lumaEnd2 -= lumaLocalAvg;
			}
			
			// If the luma deltas at the current extremities are larger than the local gradient, we have reached the side of the edge.
			reached1 = abs(lumaEnd1) >= gradientScaled;
			reached2 = abs(lumaEnd2) >= gradientScaled;
			reachedBoth = reached1 && reached2;
			
			// If the side is not reached, we continue to explore in this direction, with a variable quality
			if (!reached1)
				uv1 -= offset * QUALITY[i];
	
			if (!reached2)
				uv2 += offset * QUALITY[i];
				
			// If both sides have been reached, stop the exploration.
			if (reachedBoth)
				break;
		}
	}
	
	// Compute the distance to each extremity of the edge
	float distance1 = isHorizontal ? (uv.x - uv1.x) : (uv.y - uv1.y);
	float distance2 = isHorizontal ? (uv2.x - uv.x) : (uv2.y - uv.y);
	
	// In which direction is the extremity of the edge closer
	bool isDirection1 = distance1 < distance2;
	float distanceFinal = min(distance1, distance2);
	
	// Length of the edge
	float edgeThickness = distance1 + distance2;
	
	// UV offset: read in the direction of the closest side of the edge
	float pixelOffset = -distanceFinal / edgeThickness + 0.5;
	
	// Is the luma at the center smaller than the local avg
	bool isLumaCenterSmaller = lumaCenter < lumaLocalAvg;
	
	// If the luma at the center is smaller than its neighbour, the delta luma at each end should be positive (same variation)
	// (in the direction of the closer side of the edge)
	bool correctVariation = ((isDirection1 ? lumaEnd1 : lumaEnd2) < 0.0) != isLumaCenterSmaller;
	
	// If the luma variation is incorrect, do not offset
	float finalOffset = correctVariation ? pixelOffset : 0.0;
	
	// Sub-pixel shifting
	// Full weighted avg of the luma over the 3x3 neighbourhood
	float lumaAverage = (1.0 / 12.0) * (2.0 * (lumaDownUp + lumaLeftRight) + lumaLeftCorners + lumaRightCorners);
	// Ratio of the delta between the global avg and the center luma, over the luma range in the 3x3 neighborhood
	float subPixelOffset1 = clamp(abs(lumaAverage - lumaCenter) / lumaRange,0.0,1.0);
	float subPixelOffset2 = (-2.0 * subPixelOffset1 + 3.0) * subPixelOffset1 * subPixelOffset1;
	// Compute a sub-pixel offset based on this delta.
	float subPixelOffsetFinal = subPixelOffset2 * subPixelOffset2 * SUBPIXEL_QUALITY;
	
	// Pick the biggest of the two offsets.
	finalOffset = max(finalOffset, subPixelOffsetFinal);
	
	// Compute the final UV coordinates.
	vec2 finalUv = uv;
	if (isHorizontal)
		finalUv.y += finalOffset * stepLength;
	else
		finalUv.x += finalOffset * stepLength;
	
	color.rgb = texture(postProcessedTexture, finalUv).rgb;
}