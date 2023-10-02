import csv
import os
import xml.etree.ElementTree as ET
from io import StringIO

map_in = "overworld.tmx"
map_name = os.path.splitext(map_in)[0]
map_out = map_name + ".txt"
xml_tree = ET.parse(map_in)
tilemap = xml_tree.getroot()

# map
#   (layer|objectgroup)*

# layer
#

assert(tilemap.tag == "map")
assert(tilemap.get("version") == "1.10")
assert(tilemap.get("orientation") == "orthogonal")
assert(tilemap.get("renderorder") == "right-down")
width = int(tilemap.get("width"))
height = int(tilemap.get("height"))
tile_width = int(tilemap.get("tilewidth"))
tile_height = int(tilemap.get("tileheight"))

tilesets = tilemap.findall("./tileset")
layers = tilemap.findall("./layer")
object_groups = tilemap.findall("./objectgroup")

print("# map version name")
print(f"map 8 map_{map_name}")
print("")

print("# tilesets")
for tileset in tilesets:
    assert(tileset.tag == "tileset")
    first_gid = tileset.get("firstgid")
    source = tileset.get("source")
    print(f"{first_gid} {source}")
print("")

print("# layers")
for layer in layers:
    assert(layer.tag == "layer")
    id = layer.get("id")
    name = layer.get("name")
    width = int(layer.get("width"))
    height = int(layer.get("height"))
    visible = layer.get("visible") != "false"
    if visible:
        data = layer.find("data")
        assert(data.get("encoding") == "csv")
        assert(data.get("compression") == None)

        csv_io = StringIO(data.text.replace('\n', ''))
        csv_reader = csv.reader(csv_io, delimiter=',')
        tile_data = next(csv_reader)
        tile_data = list(map(int, tile_data))
        print("# tiles width height [indices]")
        print(f"tiles {width} {height}")
        for y in range(0, height):
            for x in range(0, width):
                print(f"{str(tile_data[y * width + x]).ljust(2)} ", end="")
            print("")
print("")

print("# object_groups")
for object_group in object_groups:
    assert(object_group.tag == "objectgroup")
    id = object_group.get("id")
    name = object_group.get("name")
    print(f"# [{id}] \"{name}\"")

    objects = object_group.findall("./object")

    print("# objects count [id name x y width height]")
    print(f"objects {len(objects)}")
    for object in objects:
        assert(object.tag == "object")
        id = object.get("id")
        name = '"' + object.get("name") + '"'
        x = object.get("x")
        y = object.get("y")
        width = object.get("width")
        height = object.get("height")
        print(f"{id} {name.ljust(32)} {str(x).ljust(6)} {str(y).ljust(6)} {str(width).ljust(6)} {str(height).ljust(6)}")
