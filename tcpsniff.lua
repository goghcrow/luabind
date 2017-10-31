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

function tcpsniff_onPacket(pkthdr, payload)
	print(pkthdr.caplen)
	print(pkthdr.len)
	print(pkthdr.ts)
	print(pkthdr.tbl)
	print(pkthdr.tbl.caplen)
	print("+++++++++++++++++++++++++++++")
	print(payload)
	-- C.tcpsniff_stop()
end



-- function string:explode(self, delimiter)
-- 	local result = { }
-- 	local from  = 1
-- 	local delim_from, delim_to = string.find( self, delimiter, from  )
-- 	while delim_from do
-- 		table.insert( result, string.sub( self, from , delim_from-1 ) )
-- 		from  = delim_to + 1
-- 		delim_from, delim_to = string.find( self, delimiter, from  )
-- 	end
-- 	table.insert( result, string.sub( self, from  ) )
-- 	return result
-- end