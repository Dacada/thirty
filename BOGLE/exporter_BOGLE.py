"""This converter converts the mesh data from a Blender object into a
representation that is more suitable for OpenGL indexed drawing.

Vertices and UVs are extracted from the object, avoiding repetition when
possible, and the final output is an extremely simple binary file containing
data that can almost directly be passed on to OpenGL.

"""

# imports for integration with Blender UI
import bpy
from bpy_extras.io_utils import ExportHelper
from bpy.props import StringProperty, BoolProperty, EnumProperty
from bpy.types import Operator

# useful imports from Blender's API
from mathutils import Vector

# useful imports from Python's stdlib itself
import struct
import array


bl_info = {
    'name': "BOGLE exporter (dev)",
    'description': "Export blender data to a custom binary file format BOGLE",
    'author': "David Carrera",
    'version': (0, 0, 0),
    'blender': (2, 82, 0),
    'location': "File > Export",
    'warning': "",
    'category': 'Import-Export',
}


class BOGLEConversionError(Exception):
    pass


class BOGLEVertex:
    """Represents vertex data for OpenGL: vertex coordinates (3D) and texture
coordinates (2D)."""
    def __init__(self, vx, vy, vz, tx, ty):
        self.vert = Vector((vx, vy, vz))
        self.uv = Vector((tx, ty))

    def export(self, f):
        """Write vertex data to file"""
        vertex_fmt = '<fffff'
        vertex_data = (
            self.vert.x,
            self.vert.y,
            self.vert.z,
            self.uv.x,
            self.uv.y
        )
        vertex = struct.pack(vertex_fmt, *vertex_data)
        f.write(vertex)


class BOGLEObject:
    """Blender object export to BOGLE object file"""

    def __init__(self):
        self.name = ""
        self.vertices = []
        self.indices = []

        self._object = None
        self._mesh = None
        self._indexed_mesh = []
        self._triangle_vertices = {}
        self._index_map = {}

    def convert(self, object):
        """This is where the fun starts, the rest is boilerplate. Check each of the
        functions called here to see every step of the process.

        """
        self._object = object
        self._obtain_name()
        self._obtain_mesh()
        self._index_mesh()
        self._remove_indexed_mesh_repeats()
        self._triangulate_indexed_mesh()
        self._map_indices()
        self._get_indices()
        self._get_vertices()

    def export(self, f):
        """Export the converted data"""
        self._export_header(f)
        self._export_name(f)
        self._export_vertices(f)
        self._export_indices(f)

    def _export_header(self, f):
        header_fmt = "<BLL"
        header_data = (
            len(self.name),
            len(self.vertices),
            len(self.indices)
        )
        header = struct.pack(header_fmt, *header_data)
        f.write(header)

    def _export_name(self, f):
        name_fmt = 'B'
        name = array.array(name_fmt, self.name.encode('ascii')).tobytes()
        f.write(name)

    def _export_vertices(self, f):
        for vertex in self.vertices:
            vertex.export(f)

    def _export_indices(self, f):
        indices_fmt = 'I'
        indices = array.array(indices_fmt, self.indices).tobytes()
        f.write(indices)

    def _obtain_name(self):
        self.name = self._object.name

    def _obtain_mesh(self):
        # TODO: Automatically triangulate, apply modifiers, etc.
        self._mesh = self._object.data
        self._uv_data = self._mesh.uv_layers.active.data

    def _index_mesh(self):
        # self._mesh is a triangular mesh. Here we will convert it to a list of
        # certain indices we're interested in. We care about the vertex_index,
        # the loop_index and the triangle_index. The first two are clearly
        # explained in Blender's docs, the triangle_index is just the index of
        # the corresponding polygon in self._mesh.polygons. Since self._mesh is
        # triangular, every polygon in it should have three loops. If that's
        # not the case we raise an exception.

        indexed_mesh = []

        for triangle_index, triangle in enumerate(self._mesh.polygons):
            if triangle.loop_total != 3:
                raise Exception("Mesh is not made entirely out of triangles!")
            for loop_index in triangle.loop_indices:
                vertex_index = self._mesh.loops[loop_index].vertex_index
                indices = (vertex_index, loop_index, [triangle_index])
                indexed_mesh.append(indices)

        self._indexed_mesh = indexed_mesh

    def _remove_indexed_mesh_repeats(self):
        # At this point self._indexed_mesh is a list of tuples which each
        # contain a vertex index, a loop index and one triangle index (actually
        # a list of length one for now). The problem is that this is
        # repetitive. If we stayed with this every vertex would be triplicated
        # to make sure it can be associated with every possible uv. But this is
        # not necesarily the case, we might need only two or one vertices. In
        # other words, this assumes that vertices map to three different uvs,
        # the absolute maximum number of uvs they could map to. But it could be
        # just one or two. In the end we'll replicate vertices for as many uvs
        # as they map into. So it's important to remove repetitions here. After
        # this process self._indexed_mesh will look exactly the same but the
        # last element will have every triangle that contains the same
        # combination of vertex and uv. In an ideal mesh where no repetition of
        # vertices is needed, every list would have three elements: that is,
        # every vertex is used by three triangles. But if there are repetitions
        # there will be less: Out of the three triangles that use one vertex,
        # another had to use a different vertex to be able to have that UV
        # coordinate.

        def i(f):
            return int(round(f, 5)*10**5)

        indexed_norepeat = {}
        for index in self._indexed_mesh:
            vertex_index, loop_index, triangle_indices = index

            vertex = self._mesh.vertices[vertex_index].co
            uv = self._uv_data[loop_index].uv

            coords = (i(vertex.x), i(vertex.y), i(vertex.z), i(uv.x), i(uv.y))
            triangles = indexed_norepeat.setdefault(
                coords, (vertex_index, loop_index, []))
            triangles[2].append(triangle_indices[0])

        self._indexed_mesh = list(indexed_norepeat.values())

    def _triangulate_indexed_mesh(self):
        # Transform self._indexed_mesh, which is a mapping from OpenGL vertices
        # to the object's triangles, into a mapping from the object's triangles
        # to the OpenGL vertices. That is, transform the list into a dictionary
        # where each key is the index of a triangle in the original mesh and
        # each value is an OpenGL vertex. We're still using indices instead of
        # the actual values. Note how every triangle will have three OpenGL
        # vertices even after the previous transformations of the data.

        triangle_vertices = {}
        for index in self._indexed_mesh:
            vertex_index, loop_index, triangle_indexes = index
            for triangle_index in triangle_indexes:
                triangle = triangle_vertices.setdefault(triangle_index, [])
                triangle.append((vertex_index, loop_index))
        self._triangle_vertices = triangle_vertices

    def _map_indices(self):
        # Associate a sequential integer to each unique tuple found in the
        # values of self._indexed_mesh. This sequential integer is the index
        # used in OpenGL. This mapping is saved in self._index_map

        self._index_map = {
            vertex: gl_index
            for gl_index, vertex in enumerate(set([
                vertex
                for vertices in self._triangle_vertices.values()
                for vertex in vertices
            ]))
        }

    def _get_indices(self):
        # Remember that self._triangle_vertices is a mapping from a triangle to
        # the three vertices it's formed by, and self._index_map is a mapping
        # from a vertex to the index it should end up having. For each
        # triangle, we get the index corresponding to each of its three
        # vertices, and add these three indexes sequentially to the final list.

        # TODO: CORRECT WINDING ORDER TO AVOID FACE CULLING!!!!
        indices = [
            self._index_map[vertex]
            for triangle, vertices in self._triangle_vertices.items()
            for vertex in vertices
        ]
        self.indices = indices

    def _get_vertices(self):
        # Remember that self._index_map is a mapping from a vertex to the index
        # it should end up having. And that these indices are just sequential
        # numbers. So they should go from 0 to len(self._index_map)-1. Knowing
        # this, we simply create the reverse dictionary and iterate its values
        # in the sequential order of the corresponding key. That is, the value
        # of key 0, the value of key 1, etc. Each value is a vertex which we
        # can convert to the correct OpenGL coordinate system.

        reverse_index_map = {v: k for k, v in self._index_map.items()}
        for gl_index in sorted(reverse_index_map):
            vertex_index, loop_index = reverse_index_map[gl_index]
            vertex = self._mesh.vertices[vertex_index].co
            uv = self._uv_data[loop_index].uv
            gl_vertex = BOGLEVertex(-vertex.x, vertex.z, vertex.y, uv.x, uv.y)
            self.vertices.append(gl_vertex)


class BOGLExporter:
    """Blender export to BOGLE file"""

    def __init__(self):
        self.objects = []

    def convert(self, context):
        """Convert the objects that should be converted"""
        for object in self._iterate_objects(context):
            bogle_object = self._convert_object(object)
            self.objects.append(bogle_object)

    def export(self, filepath):
        """Export converted data to file"""
        with open(filepath, 'wb') as f:
            self._export_header(f)
            for object in self.objects:
                object.export(f)

    def _iterate_objects(self, context):
        for object in context.scene.objects:
            if object.type == 'MESH':
                yield object

    def _convert_object(self, object):
        bogle_object = BOGLEObject()
        bogle_object.convert(object)
        return bogle_object

    def _export_header(self, f):
        header_fmt = '<cccccBL'
        header_data = (
            b'B', b'O', b'G', b'L', b'E',  # Magic
            0,  # Version
            len(self.objects)  # Number of objects
        )
        header = struct.pack(header_fmt, *header_data)
        f.write(header)


class BogleExportData(Operator, ExportHelper):
    """Export objects in the current scene to BOGLE"""

    bl_idname = "export.bogle"
    bl_label = "Export Data"

    filename_ext = ".bgl"

    filter_glob: StringProperty(
        default="*.bgl",
        options={'HIDDEN'},
        maxlen=255,
    )

    use_setting: BoolProperty(
        name="Example Boolean",
        description="Example Tooltip",
        default=True,
    )

    type: EnumProperty(
        name="Example Enum",
        description="Choose between two items",
        items=(
            ('OPT_A', "First Option", "Description one"),
            ('OPT_B', "Second Option", "Description two"),
        ),
        default='OPT_A',
    )

    def execute(self, context):
        print("running BOGLE export...")
        try:
            # self.use_setting, self.type, etc
            exporter = BOGLExporter()
            exporter.convert(context)
            exporter.export(self.filepath)
        except BOGLEConversionError as e:
            self.report({'ERROR'}, str(e))
            print(str(e))
            return {'CANCELLED'}
        else:
            print("BOGLE export finished")
            return {'FINISHED'}


def menu_func_export(self, context):
    self.layout.operator(BogleExportData.bl_idname, text="Export to BOGLE")


def register():
    bpy.utils.register_class(BogleExportData)
    bpy.types.TOPBAR_MT_file_export.append(menu_func_export)


def unregister():
    bpy.utils.unregister_class(BogleExportData)
    bpy.types.TOPBAR_MT_file_export.remove(menu_func_export)


if __name__ == "__main__":
    register()
