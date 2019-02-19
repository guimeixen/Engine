List = {}

function List.new()
	return {first = 0, last = -1}
end

function List.pushRight(queue, value)
	local last = queue.last - 1
	queue.last = last
	queue[last] = value
end

function List.popLeft(queue)
	local first = queue.first
	if (first > queue.last) then
		error('queue is empty')
	end
	local value = queue[first]
	queue[first] = nil					-- To allow gargabe collection
	queue.first = first + 1
	return value
end