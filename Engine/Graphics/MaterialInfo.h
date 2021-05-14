#pragma once

namespace Engine
{
	enum MaterialOptions
	{
		NORMAL_MAP = 1,
		INSTANCING = (1 << 1),
		ANIMATED = (1 << 2),
		ALPHA = (1 << 3),
	};

	enum BlendFactor
	{
		ZERO,
		ONE,
		SRC_ALPHA,
		DST_ALPHA,
		SRC_COLOR,
		DST_COLOR,
		ONE_MINUS_SRC_ALPHA,
		ONE_MINUS_SRC_COLOR
	};

	enum Topology
	{
		TRIANGLES,
		TRIANGLE_STRIP,
		LINES,
		LINE_TRIP
	};
}
