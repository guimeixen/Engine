function vecDot(a, b)
   return (a.x * b.x) + (a.y * b.y) + (a.z * b.z)
end

function vecSum(a,b)
	a.x = a.x + b.x
	a.y = a.y + b.y
	a.z = a.z + b.z
end

function vecScale(a,b)
	a.x = a.x * b
	a.y = a.y * b
	a.z = a.z * b
end

function vecSub(a, b)
	return vec3(a.x - b.x, a.y - b.y, a.z - b.z)
end

function vecSubFast(a, b)
	a.x = a.x - b.x
	a.y = a.y - b.y
	a.z = a.z - b.z
end

function vecLengthSq(a)
	return (a.x * a.x + a.y * a.y + a.z * a.z)
end

function vecLength(a)
	return math.sqrt(vecLengthSq(a))
end

function vecNormalize(a)
	local length = math.sqrt(vecLengthSq(a))
	local multiplier
	if (length > 0) then
		multiplier=1 / length
	else
		multiplier=0.0001
	end
	a.x = a.x * multiplier
	a.y = a.y * multiplier
	a.z = a.z * multiplier
end

function lerp(a, b, t)
	return (1.0 - t) * a + t * b
end

function angleFromPoint(p)
	local angle = math.deg(math.atan(p.x, p.z))
	angle = angle + 360.0
	return math.fmod(angle, 360.0)
end

function randomFloat(low, high)
	return low + math.random()  * (high - low)
end