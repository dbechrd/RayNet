import xml.etree.ElementTree as ET
import csv
from io import StringIO

xml_tree = ET.parse("overworld.tmx")
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

print("tilesets:")
for tileset in tilesets:
    assert(tileset.tag == "tileset")
    first_gid = tileset.get("firstgid")
    source = tileset.get("source")
    print(f"  {first_gid} {source}")

print("layers:")
for layer in layers:
    assert(layer.tag == "layer")
    id = layer.get("id")
    name = layer.get("name")
    width = layer.get("width")
    height = layer.get("height")
    visible = layer.get("visible") != "false"
    if visible:
        print(f"  [{id}] \"{name}\" {width} x {height}")

        data = layer.find("data")
        assert(data.get("encoding") == "csv")
        assert(data.get("compression") == None)

        csv_io = StringIO(data.text.replace('\n', ''))
        csv_reader = csv.reader(csv_io, delimiter=',')
        tile_data = next(csv_reader)
        tile_data = list(map(int, tile_data))
        print(f"  {len(tile_data)} tiles {type(tile_data[0])}")

print("object_groups:")
for object_group in object_groups:
    assert(object_group.tag == "objectgroup")
    id = object_group.get("id")
    name = object_group.get("name")
    print(f"  [{id}] \"{name}\"")

    objects = object_group.findall("./object")
    for object in objects:
        assert(object.tag == "object")
        id = object.get("id")
        name = object.get("name")
        x = object.get("x")
        y = object.get("y")
        width = object.get("width")
        height = object.get("height")
        print(f"    [{id}] \"{name}\" {width} x {height} @ {x}, {y}")