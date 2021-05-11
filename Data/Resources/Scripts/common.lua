function vec3Dot(a, b)
   return (a.x * b.x) + (a.y * b.y) + (a.z * b.z)
end

function vec3Add(a, b)
	return vec3(a.x + b.x, a.y + b.y, a.z + b.z)
end

function vec3AddFast(a,b)
	a.x = a.x + b.x
	a.y = a.y + b.y
	a.z = a.z + b.z
end

function vec3Scale(a,b)
	a.x = a.x * b
	a.y = a.y * b
	a.z = a.z * b
end

function vec3Sub(a, b)
	return vec3(a.x - b.x, a.y - b.y, a.z - b.z)
end

function vec3SubFast(a, b)
	a.x = a.x - b.x
	a.y = a.y - b.y
	a.z = a.z - b.z
end

function vec3LengthSq(a)
	return (a.x * a.x + a.y * a.y + a.z * a.z)
end

function vec3Length(a)
	return math.sqrt(vec3LengthSq(a))
end

function vec3Normalize(a)
	local length = math.sqrt(vec3LengthSq(a))
	local multiplier
	if (length > 0.0) then
		multiplier = 1.0 / length
	else
		multiplier = 0.0001
	end
	a.x = a.x * multiplier
	a.y = a.y * multiplier
	a.z = a.z * multiplier
end

function printVec3(a)
	print('x: ' .. tostring(a.x) .. ' y: ' .. tostring(a.y) .. ' z:' .. tostring(a.z))
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