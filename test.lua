print "Hello World"

print(C.double(2))

function hello(str)
	return #str
end

function trans(...)
    return ...
end

function nestedtbl(tbl)
    print()
    print("tbl=", tbl)
    print("tbl.name=", tbl.name)
    print("tbl.id=", tbl.id)
    print("tbl.subtbl=", tbl.subtbl)
    print("tbl.subtbl.name=", tbl.subtbl.name)
    print("tbl.subtbl.id=", tbl.subtbl.id)
    print("tbl.subtbl.subtbl=", tbl.subtbl.subtbl)
    print("tbl.subtbl.subtbl.name=", tbl.subtbl.subtbl.name)
    print("tbl.subtbl.subtbl.id=", tbl.subtbl.subtbl.id)

    return tbl
end