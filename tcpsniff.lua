local totable = string.ToTable
local string_sub = string.sub
local string_find = string.find
local string_len = string.len

print "Starting...\n"

function tcpsniff_interface()
    return "any"
end

function tcpsniff_expression()
    return "tcp and dst port 80"
end

function tcpsniff_onPacket(pcap, ip, tcp, tcpopt, data)
	print_r(pcap)
	print()
	
	print_r(ip)
	print()
	
	print_r(tcp)
	print()

	print_r(tcpopt)
	print()

	print(data)
	C.tcpsniff_stop()
end




-- https://blog.codingnow.com/2009/05/print_r.html
-- 其实实现的细节需要注意的就两个地方：一个是考虑循环引用的问题。（一般用 table 模拟树结构都会记录一个 parent 节点的引用，这样就造成了循环引用）
-- 一个是表格线的处理，对最后一个子节点需要特殊处理。
local print = print
local tconcat = table.concat
local tinsert = table.insert
local srep = string.rep
local type = type
local pairs = pairs
local tostring = tostring
local next = next
 
function print_r(root)
	local cache = {  [root] = "." }
	local function _dump(t,space,name)
		local temp = {}
		for k,v in pairs(t) do
			local key = tostring(k)
			if cache[v] then
				tinsert(temp,"+" .. key .. " {" .. cache[v].."}")
			elseif type(v) == "table" then
				local new_key = name .. "." .. key
				cache[v] = new_key
				tinsert(temp,"+" .. key .. _dump(v,space .. (next(t,k) and "|" or " " ).. srep(" ",#key),new_key))
			else
				tinsert(temp,"+" .. key .. " [" .. tostring(v).."]")
			end
		end
		return tconcat(temp,"\n"..space)
	end
	print(_dump(root, "",""))
end
