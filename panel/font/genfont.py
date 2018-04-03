import sys, os, string
import pprint


def print_val(id, prop, val, indent = "    "):
    key = "RUNE_%s" % id
    if prop != "":
        key = key + "_" + prop
    print "%s%-20s = %s," % (indent, key, val)


def transpose(lists):
   if not lists: return []
   return map(lambda *row: list(row), *lists)


def gendata(rune):
    data = []
    for n in xrange(0, rune['height'] / 8):
        slice = rune['def'][n * 8 : n * 8 + 8]
        slice = transpose(slice)
        slice = [reduce(lambda a,b: (a << 1) + b, row) for row in slice]
        data.append(slice)

    return data


def flatten(data):
    return sum(data, [])

def rle(data):
    result = []
    data = flatten(data)
    counted_octet = -1
    count = 0
    for octet in data:
        if count == 0:
            count = 1
            counted_octet = octet
        elif octet == counted_octet:
            count += 1
        else:
            if count == 1 and counted_octet == 0:
                result.append(0)
            else:
                result.append(count)
                result.append(counted_octet)
            count = 1
            counted_octet = octet

    if count > 0:
        if count == 1 and counted_octet == 0:
            result.append(0)
        else:
            result.append(count)
            result.append(counted_octet)

    return result


what    = sys.argv[1]
runes   = { }
id_list = []

gen_inc  = (what == 'inc')
gen_data = (what == 'data')

rune = None

while True:
    line = sys.stdin.readline()
    if line == "":
        break

    line = line[:-1]
    if line == "" or line[:1] == "#":
        continue

    pieces = line.split(' ')
    if pieces[0] == "ID":
        if not rune is None:
            if len(rune['def']) != rune['height']:
                print "ERROR: %s: wrong number of lines" % rune['id']
            runes[rune['id']] = rune
            id_list.append(rune['id'])
        rune = { 'id': pieces[1], 'def' : [] }
    elif pieces[0] == "DIM":
        rune['width'] = (int)(pieces[1])
        rune['height'] = (int)(pieces[2])
        if rune['height'] % 8 != 0:
            print "ERROR: %s: height must be a multiple of 8" % rune['id']
    elif pieces[0] == "END":
        if not rune is None:
            if len(rune['def']) != rune['height']:
                print "ERROR: %s: wrong number of lines" % rune['id']
            runes[rune['id']] = rune
            id_list.append(rune['id'])
        break
    else:
        rune['def'].append(map(lambda v: 1 if v != '-' else 0, pieces[:rune['width']]))

if gen_inc:
    print "#ifndef __RUNES__"
    print "#define __RULES__"
    print "enum Rune {"

n = 0
offset = 0
total_size = 0
for id in id_list:
    rune = runes[id]
    w = rune['width']
    h = rune['height']
    rune['data'] = rle(gendata(rune))
    size = len(rune['data'])
    rune['dim'] = w*h/8
    rune['size'] = size
    rune['offset'] = offset
    runes[id] = rune

    if gen_inc:
        print_val(id, "", n)
        print_val(id, "WIDTH", w)
        print_val(id, "HEIGHT", h)
        print_val(id, "OFFSET", offset)
        print_val(id, "SIZE", size)

    n += 1
    offset += size
    total_size += size

if gen_inc:
    print "    RUNE_NUM             = %d\n};" % n
    print "#endif // __RULES__"

if gen_data:
    num_runes = n

    print "const unsigned short rune_offset[RUNE_NUM] = {"

    last_id = id_list[-1]
    s = "    "
    n = 0
    for id in id_list:
        if n == 4:
            s += "\n    "
            n = 0
        n += 1
        s += "RUNE_%s_OFFSET" % id
        if id != last_id:
            s += ", "

    print s
    print "};\n\n"


    print "const unsigned char rune_size[RUNE_NUM] = {"

    s = "    "
    n = 0
    for id in id_list:
        if n == 4:
            s += "\n    "
            n = 0
        n += 1
        s += "RUNE_%s_SIZE" % id
        if id != last_id:
            s += ", "

    print s
    print "};\n\n"


    print "// %d runes.  Total data size %d bytes." % (num_runes, total_size)
    print "// This is run-length encoded.  One byte count, one byte value.  The exception is"
    print "// a single 0 which represents one zero."

    print "const unsigned char rune_data[] = {"

    for id in id_list:
        rune = runes[id]
        data = rune['data']
        print "    // Rune: %s, offset %d, size %d" % (id, rune['offset'], rune['size'])
        s = "    "
        pos = 0
        for octet in rune['data']:
            s += "0x%02x," % octet
            pos += 1
            if (pos % 16) == 0:
                print s
                s = "    "
                pos = 0
        if s != "    ": 
            if id == last_id and s[-1] == ",":
                print s[:-1]
            else:
                print s
        print

    print "    // End of list\n};"
