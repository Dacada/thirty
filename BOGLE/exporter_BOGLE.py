# <pep8-80 compliant>

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

# useful math imports
from mathutils import Vector, Matrix
from math import pi

# useful imports from Python's stdlib
import struct
import array


bl_info = {
    'name': "BOGLE exporter (dev)",
    'description': "Export blender data to a custom binary file format BOGLE",
    'author': "David Carrera",
    'version': (0, 0),
    'blender': (2, 82, 0),
    'location': "File > Export",
    'warning': "",
    'category': 'Import-Export',
}


class BOGLEConversionError(Exception):
    pass


class BOGLEConfig:
    def __init__(self, other):
        self.apply_modifiers = other.apply_modifiers
        self.only_selected_objects = other.only_selected_objects
        self.convert_coordinates = other.convert_coordinates
        self.winding_order = other.winding_order


class BOGLEBaseObject:
    def __init__(self, config):
        self.config = config

    def convert(self, object):
        raise NotImplementedError

    def export(self, f):
        raise NotImplementedError

    def _convert_vec(self, vec, negate_x=True):
        if negate_x:
            # (x,y,z) => (-x, z, y)
            _change_basis_matrix = Matrix((
                (-1, 0, 0, 0),
                (0, 0, 1, 0),
                (0, 1, 0, 0),
                (0, 0, 0, 1)
            ))
        else:
            # (x,y,z) => (x, z, y)
            _change_basis_matrix = Matrix((
                (1, 0, 0, 0),
                (0, 0, 1, 0),
                (0, 1, 0, 0),
                (0, 0, 0, 1)
            ))

        if self.config.convert_coordinates:
            return vec @ _change_basis_matrix
        else:
            return Vector(vec)


class BOGLEVertex(BOGLEBaseObject):
    """Represents vertex data for OpenGL: vertex coordinates (3D) and texture
coordinates (2D)."""

    def __init__(self, config, vx, vy, vz, tx, ty, nx, ny, nz):
        super().__init__(config)
        self.vert = Vector((vx, vy, vz))
        self.uv = Vector((tx, ty))
        self.norm = Vector((nx, ny, nz))

    def export(self, f):
        """Write vertex data to file"""
        vertex_fmt = '<8f'
        vertex_data = (
            self.vert.x,
            self.vert.y,
            self.vert.z,
            self.uv.x,
            self.uv.y,
            self.norm.x,
            self.norm.y,
            self.norm.z
        )
        vertex = struct.pack(vertex_fmt, *vertex_data)
        f.write(vertex)


class BOGLEObject(BOGLEBaseObject):
    """Blender object export to BOGLE file"""

    def __init__(self, config):
        super().__init__(config)

        self.name = ""
        self.parent_name = None

        self.vertices = []
        self.indices = []

        self._object = None
        self._mesh = None
        self._indexed_mesh = []
        self._triangle_vertices = {}
        self._index_map = {}
        self._transforms = None

    def convert(self, object, empty=False):
        """This is where the fun starts, the rest is boilerplate. Check each of the
        functions called here to see every step of the process.

        """
        try:
            self._object = object

            self.name = object.original.name.encode('ascii')
            if object.parent is not None:
                self.parent_name = object.parent.name.encode('ascii')

            if len(self.name) > 31:
                raise BOGLEConversionError(
                    f"Name {repr(self.name)} too long, supported names of only up to 31 characters")
            elif b'\0' in self.name:
                raise BOGLEConversionError(
                    f"Invalid name {repr(self.name)} can't have the null character.")

            if empty:
                print("Exporting object without a mesh.")
            else:
                self._obtain_mesh()
                print(
                    f"Exporting mesh with {len(self._mesh.vertices)} vertices")
                self._index_mesh()
                self._remove_indexed_mesh_repeats()
                self._triangulate_indexed_mesh()
                self._map_indices()
                self._get_indices()
                self._get_vertices()
                print(f"Converted mesh to {len(self.vertices)} vertices")

            self._get_transforms()
        finally:
            self._cleanup()

    def export(self, f):
        """Export the converted data"""
        print(f"Writing {len(self.vertices)} vertices and "
              f"{len(self.indices)} indices to file.")
        self._export_header(f)
        self._export_name(f)
        self._export_vertices(f)
        self._export_indices(f)
        self._export_transforms(f)

    def _export_header(self, f):
        header_fmt = "<LL"
        header_data = (
            len(self.vertices),
            len(self.indices)
        )
        header = struct.pack(header_fmt, *header_data)
        f.write(header)

    def _export_name(self, f):
        name_fmt = 'B'
        exported_name = self.name + b'\0' + b' '*(31-len(self.name))
        name = array.array(name_fmt, exported_name).tobytes()
        f.write(name)

    def _export_vertices(self, f):
        for vertex in self.vertices:
            vertex.export(f)

    def _export_indices(self, f):
        indices_fmt = 'I'
        indices = array.array(indices_fmt, self.indices).tobytes()
        f.write(indices)

    def _export_transforms(self, f):
        matrix_fmt = '<3f3f1f3f'
        trans, (rot, a), scale = self._transforms
        transforms = (
            trans.x, trans.y, trans.z,
            rot.x, rot.y, rot.z, a,
            scale.x, scale.y, scale.z
        )
        matrix = struct.pack(matrix_fmt, *transforms)
        f.write(matrix)

    def _obtain_mesh(self):
        if self.config.apply_modifiers:
            self._mesh = self._object.to_mesh()
        else:
            self._mesh = self._object.original.to_mesh()
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
                raise BOGLEConversionError(
                    "Mesh is not made entirely out of triangles!")
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

        indices = [
            self._index_map[vertex]
            for triangle, vertices in self._triangle_vertices.items()
            for vertex in self._sort_winding_tri(triangle, vertices)
        ]
        self.indices = indices

    def _sort_winding_tri(self, triangle_index, vertex_indices):
        # Determine correct winding order for OpenGL face culling (assume CCW):
        # We have three vertices. We'll start from the first one, p1, and
        # decide which the other two, p2 or p3, to continue with. Then we'll
        # end with the other. We calculate the vector that goes from p1 to p2
        # and the one that goes from p1 to p3 (v1=p2-p1 and v2=p3-p1
        # respectively). Then we calculate their cross prodcut: v1.cross(v2)
        # and v2.cross(v1). Whichever is more similar (should be equal) to the
        # normal blender has for that triangle (mesh.polygons[triangle_index])
        # marks the correct winding order.

        if self.config.winding_order == 'NONE':
            return vertex_indices

        ip1, ip2, ip3 = vertex_indices

        p1 = self._mesh.vertices[ip1[0]].co
        p2 = self._mesh.vertices[ip2[0]].co
        p3 = self._mesh.vertices[ip3[0]].co

        v1 = p2 - p1
        v2 = p3 - p1

        norm1 = v1.cross(v2)
        norm2 = v2.cross(v1)

        Vector.normalize(norm1)
        Vector.normalize(norm2)

        norm = self._mesh.polygons[triangle_index].normal

        diff1 = abs((norm-norm1).magnitude)
        diff2 = abs((norm-norm2).magnitude)

        if diff1 < diff2:
            order = (ip1, ip2, ip3)
        else:
            order = (ip1, ip3, ip2)

        if self.config.winding_order == 'CW':
            return reversed(order)
        else:
            return order

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
            normal = self._mesh.vertices[vertex_index].normal
            gl_vertex = BOGLEVertex(
                self.config,
                *self._convert_vec(vertex),
                *uv,
                *self._convert_vec(normal))
            self.vertices.append(gl_vertex)

    def _get_transforms(self):
        trans, rotq, scale = self._object.matrix_local.decompose()
        rot, a = rotq.to_axis_angle()
        convert_trans = self._convert_vec(trans)
        convert_rot = self._convert_vec(rot)
        convert_rota = (convert_rot, a)
        convert_scale = self._convert_vec(scale, negate_x=False)
        self._transforms = (convert_trans, convert_rota, convert_scale)

    def _cleanup(self):
        self._object.to_mesh_clear()
        self._object.original.to_mesh_clear()
        self._object = None
        self._mesh = None
        self._indexed_mesh = []
        self._triangle_vertices = {}
        self._index_map = {}


class BOGLECamera(BOGLEBaseObject):
    """Blender camera export to BOGLE file"""

    def __init__(self, config):
        super().__init__(config)
        self.position = None
        self.yaw = None
        self.pitch = None

    def convert(self, object):
        position, rotation, scale = object.matrix_local.decompose()

        self.position = self._convert_vec(position)

        # Adjust rotation angles between Blender's and our systems
        x, y, z = rotation.to_euler()
        self.pitch = x - pi/2
        self.yaw = z + pi/2

    def export(self, f):
        fmt = '<fffff'
        camera = struct.pack(fmt, *self.position, self.yaw, self.pitch)
        f.write(camera)


class BOGLELight(BOGLEBaseObject):
    """Blender light export to BOGLE file"""

    def __init__(self, config):
        super().__init__(config)
        self.position = None
        self.ambient = None
        self.diffuse = None
        self.specular = None
        self.ambient_power = None
        self.diffuse_power = None
        self.specular_power = None

    def convert(self, object):
        position, _, _ = object.matrix_local.decompose()
        light = object.data

        self.position = self._convert_vec(position)

        self.ambient = self.diffuse = self.specular = Vector(
            tuple(light.color) + (1,))
        self.ambient_power = light.energy * 5
        self.diffuse_power = self.specular_power = self.ambient_power
        self.ambient_power *= 0.003

    def export(self, f):
        fmt = '<ffffffffffffffffff'
        light = struct.pack(fmt, *self.position, *self.ambient,
                            *self.diffuse, *self.specular, self.ambient_power,
                            self.diffuse_power, self.specular_power)
        f.write(light)


class BOGLExporter(BOGLEBaseObject):
    """Blender export to BOGLE file"""

    def __init__(self, config):
        super().__init__(config)
        self.objects = []
        self.cameras = []
        self.lights = []
        self.object_tree = None

    def convert(self, context):
        """Convert the objects that should be converted"""
        for object in self._iterate_objects(context):
            type = object.original.type

            if type == 'MESH':
                list = self.objects
                bogle_entity = self._convert(BOGLEObject, object, empty=False)
            elif type == 'EMPTY':
                list = self.objects
                bogle_entity = self._convert(BOGLEObject, object, empty=True)
            elif type == 'CAMERA':
                list = self.cameras
                bogle_entity = self._convert(BOGLECamera, object)
            elif type == 'LIGHT':
                list = self.lights
                bogle_entity = self._convert(BOGLELight, object)
            else:
                print(f"Not converting object type: {type}")
                continue

            list.append(bogle_entity)

        self._convert_object_tree()

    def export(self, filepath):
        """Export converted data to file"""
        with open(filepath, 'wb') as f:
            self._export_header(f)
            for object in self.objects:
                object.export(f)
            self._export_object_tree(f)
            for camera in self.cameras:
                camera.export(f)
            for light in self.lights:
                light.export(f)

    def _iterate_objects(self, context):
        depsgraph = context.evaluated_depsgraph_get()
        for object_instance in depsgraph.object_instances:
            object = object_instance.object
            if not self.config.only_selected_objects or \
               self.is_object_selected(object):
                yield object

    def is_object_selected(self, object):
        selected_parent = object.parent is None or \
            object.parent.original.select_get()
        return selected_parent and object.original.select_get()

    def _convert(self, cls, object, *args, **kwargs):
        bogle = cls(self.config)
        bogle.convert(object, *args, **kwargs)
        return bogle

    def _convert_object_tree(self):
        parent_children = {-1: []}
        objs = {obj.name: i for i, obj in enumerate(self.objects)}

        for obj in self.objects:
            me = objs[obj.name]
            if obj.parent_name is None:
                parent = -1
            else:
                parent = objs[obj.parent_name]

            children = parent_children.setdefault(parent, [])
            children.append(me)

        def convert_children(i):
            s = ''
            if i in parent_children:
                for child in parent_children[i]:
                    s += str(child) + '{' + convert_children(child) + '}'
            return s

        self.object_tree = convert_children(-1).encode('ascii')

    def _export_object_tree(self, f):
        fmt = 'B'
        exported_tree = self.object_tree + b'\0'
        tree = array.array(fmt, exported_tree).tobytes()
        f.write(tree)

    def _export_header(self, f):
        header_fmt = '<5cBLLL'
        header_data = (
            b'B', b'O', b'G', b'L', b'E',  # Magic
            0,  # Version
            len(self.objects),  # Number of objects
            len(self.cameras),  # Number of cameras
            len(self.lights),  # Number of lights
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

    apply_modifiers: BoolProperty(
        name="Apply Modifiers",
        description="Whether to apply modifiers.",
        default=True,
    )

    only_selected_objects: BoolProperty(
        name="Only selected objects",
        description="Export only the selected objects instead of the "
        "whole scene",
        default=False,
    )

    convert_coordinates: BoolProperty(
        name="Convert coordinates",
        description="Convert Blender's coordinates (x,y,z) to OpenGL "
        "coordinates (-x,z,y)",
        default=True,
    )

    winding_order: EnumProperty(
        name="Winding order",
        description="Winding order of triangles, for OpenGL's face culling",
        items=(
            ('NONE', "Don't care", "Do not care about winding order, whatever "
             "happens happens"),
            ('CCW', "Counter-Clockwise", "Default for OpenGL"),
            ('CW', "Clockwise", ""),
        ),
        default='CCW',
    )

    def execute(self, context):
        print("running BOGLE export...")
        config = BOGLEConfig(self)
        try:
            # self.use_setting, self.type, etc
            exporter = BOGLExporter(config)
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
