function print_r(t, name)
	local type = type
	local pairs = pairs
	local tostring = tostring
	local print = print
	local insert_table = table.insert
	local concat_table = table.concat
	local rep_str = string.rep
	
	if "table" ~= type(t) then
		if name then
			print(tostring(name) .. " = [" .. tostring(t) .. "]")
		else
			print(t)
		end
		return
	end
	
	local cache = { [t] = "." } --To filter cycle reference
	local output = {}
	local maxlines = 30
	local space = "   "
	local function insert_output(str)
		insert_table(output, str)
		if maxlines <= #output then --flush output
			print(concat_table(output, "\n"))
			output = {}
		end
	end
	local function _dump(t, depth, name)
		for k, v in pairs(t) do
			local kname = tostring(k)
			local text
			if cache[v] then --Cycle reference
				insert_output(rep_str(space, depth) .. "|- " .. kname .. " {" .. cache[v] .. "}")
			elseif "table" == type(v) then
				local new_table_key = name .. "." .. kname
				cache[v] = new_table_key
				insert_output(rep_str(space, depth) .. "|- " .. kname)
				_dump(v, depth + 1, new_table_key)
			else
				insert_output(rep_str(space, depth) .. "|- " .. kname .. " [" .. tostring(v) .. "]")
			end
		end
	end
	if name then
		print(name)
	end
	_dump(t, 0, "")
	print(concat_table(output, "\n"))
end

function print_m(t, name)
	local tostring = tostring
	local print = print
	
	if "table" ~= type(t) then
		if name then
			print(tostring(name) .. " = [" .. tostring(t) .. "]")
		else
			print(t)
		end
		return
	end
	
	if name then print(name) end
	for k, v in pairs(t) do
		print("|- " .. tostring(k) ..  " [" .. tostring(v) .. "]")
	end
	meta = getmetatable(t)
	if meta then
		for k, v in pairs(meta) do
			print("|- " .. tostring(k) ..  " [" .. tostring(v) .. "]")
		end
	end
end
