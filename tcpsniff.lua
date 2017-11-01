local date = os.date
local tointeger = math.tointeger
local sprintf = string.format
local tinsert = table.insert
local tconcat = table.concat

local TH_FIN = 0x01
local TH_SYN = 0x02
local TH_RST = 0x04
local TH_PUSH = 0x08
local TH_ACK = 0x10
local TH_URG = 0x20


print "Sniffing...\n"

local function ip2long(ip)
    local a,b,c,d = ip:match("(%d%d?%d?)%.(%d%d?%d?)%.(%d%d?%d?)%.(%d%d?%d?)")
	return tointeger((a << 24) + (b << 16) + (c << 8) + d)
end

local function long2ip(long)
	return ((long & 0xff000000) >> 24) .. "." .. ((long & 0xff0000) >> 16) .."." .. ((long & 0xff00) >> 8) .."." .. (long & 0xff)
end



function tcpsniff_interface()
    return "en0"
end

function tcpsniff_expression()
    return "tcp and dst port 80"
end

function tcpsniff_onPacket(pcap, ip, tcp, tcpopt, data)
	local time = date("%X", tointeger(pcap.ts))
	local src = long2ip(ip.src) .. ":" .. tcp.sport
	local dst = long2ip(ip.dst) .. ":" .. tcp.dport

	local flagstbl = {}
	local flags = tcp.flags

	if (flags & TH_SYN) ~= 0 then
		tinsert(flagstbl, "SYN")
	end
	if (flags & TH_ACK) ~= 0 then
		tinsert(flagstbl, "ACK")
	end
	if (flags & TH_PUSH) ~= 0 then
		tinsert(flagstbl, "PSH")
	end
	if (flags & TH_FIN) ~= 0 then
		tinsert(flagstbl, "FIN")
	end
	if (flags & TH_RST) ~= 0 then
		tinsert(flagstbl, "RST")
	end

	flags = tconcat(flagstbl, " ")
	local win = tcp.win * (2 ^ tcpopt.snd_wscale)
	local len = #data
	print(sprintf("%s %s -> %s %s seq %d, ack %d, win %u, pktlen %u, len %u\n", time, src, dst, flags, tcp.seq, tcp.ack, win, pcap.len, len))
	if len > 0 then
		print(data)
	end

	-- C.tcpsniff_stop()
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
