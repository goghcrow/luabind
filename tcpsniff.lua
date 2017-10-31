print "Starting...\n"

function tcpsniff_interface()
    return "any"
end

function tcpsniff_expression()
    return "tcp and dst port 80"
end

function tcpsniff_onPacket(tbl)
	print(tbl)
	-- C.tcpsniff_stop()
end