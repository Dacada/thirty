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
from mathutils import Vector, Matrix, Euler
from math import pi

# useful imports from Python's stdlib
import struct
import array
import os.path  # remove extension from file name


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


def clamp(value, min, max):
    if isinstance(value, Vector):
        return Vector(clamp(n, min, max) for n in value)
    else:
        return min if value < min else max if value > max else value


class FormatSpecifier:
    _specifiers = {
        'float': 'f',
        'u8': 'B',
        'u32': 'I',
        'char': 'c',
    }

    def __init__(self):
        self._format = '<'

    def __getattr__(self, name):
        specifier = self._get_specifier(name)

        def f(amount=1):
            self._insert(specifier, amount)
            return self
        return f

    def _insert(self, what, amount):
        if amount > 1:
            self._format += str(amount)
        self._format += what

    @classmethod
    def _get_specifier(cls, name):
        try:
            return cls._specifiers[name]
        except KeyError:
            raise AttributeError

    def format(self):
        return self._format

    @classmethod
    def array(cls):
        return ArraySpecifier()


class ArraySpecifier(FormatSpecifier):
    def __getattr__(self, name):
        def f():
            return self._get_specifier(name)
        return f


class BOGLEConversionError(Exception):
    pass


class BOGLEConfig:
    def __init__(self, other):
        self.apply_modifiers = other.apply_modifiers
        self.only_selected_objects = other.only_selected_objects
        self.convert_coordinates = other.convert_coordinates
        self.winding_order = other.winding_order
        self.export_materials = other.export_materials


class BOGLEBaseObject:
    def __init__(self, config):
        self.config = config

    def convert(self, object):
        raise NotImplementedError

    def export(self, f):
        raise NotImplementedError

    def get_name(self, object):
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
            return vec.copy()


class BOGLECamera(BOGLEBaseObject):
    """Blender camera export to BOGLE file"""

    def __init__(self, config):
        super().__init__(config)
        self.width = None
        self.height = None
        self.near = None
        self.far = None
        self.fov = None

    def convert(self, object, render):
        camera = object.data
        self.width = render.resolution_x
        self.height = render.resolution_y
        self.near = camera.clip_start
        self.far = camera.clip_end
        self.fov = camera.angle

    def export(self, f):
        fmt = FormatSpecifier().u32().u32().float().float().float().format()
        camera = struct.pack(fmt, self.width, self.height,
                             self.near, self.far, self.fov)
        f.write(camera)


class BOGLEAmbientLight(BOGLEBaseObject):
    def __init__(self, config):
        super().__init__(config)
        self.color = None

    def convert(self, color):
        self.color = color

    def export(self, f):
        fmt = FormatSpecifier().float(4).format()
        light = struct.pack(fmt, *self.color)
        f.write(light)


class BOGLEVertex(BOGLEBaseObject):
    """Represents vertex data for OpenGL"""

    def __init__(self, config):
        super().__init__(config)
        self.vert = None
        self.uv = None
        self.norm = None
        self.tang = None
        self.binorm = None

    def convert(self, vert, tex, norm, tang, binorm):
        self.vert = vert
        self.uv = tex
        self.norm = norm
        self.tang = tang
        self.binorm = binorm

    def export(self, f):
        """Write vertex data to file"""
        vertex_fmt = FormatSpecifier().float(3).float(2).\
            float(3).float(3).float(3).format()
        vertex_data = (
            *self.vert,
            *clamp(self.uv, 0.0, 1.0),
            *self.norm,
            *self.tang,
            *self.binorm,
        )
        vertex = struct.pack(vertex_fmt, *vertex_data)
        f.write(vertex)


class BOGLEGeometry(BOGLEBaseObject):
    """Blender geometry export to BOGLE file"""

    def __init__(self, config):
        super().__init__(config)

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
        try:
            self._object = object
            self._obtain_mesh(object)
            print(f"Exporting mesh with {len(self._mesh.vertices)} vertices")
            self._index_mesh()
            self._remove_indexed_mesh_repeats()
            self._triangulate_indexed_mesh()
            self._map_indices()
            self._get_indices()
            self._get_vertices()
            print(f"Converted mesh to {len(self.vertices)} vertices")
        finally:
            self._cleanup()

    def get_name(self, object):
        try:
            self._obtain_mesh(object)
            return self._mesh.name
        finally:
            self._cleanup()

    def export(self, f):
        """Export the converted data"""
        print(f"Writing {len(self.vertices)} vertices and "
              f"{len(self.indices)} indices to file.")
        self._export_header(f)
        self._export_vertices(f)
        self._export_indices(f)

    def _export_header(self, f):
        header_fmt = FormatSpecifier().u32().u32().format()
        header_data = (
            len(self.vertices),
            len(self.indices)
        )
        header = struct.pack(header_fmt, *header_data)
        f.write(header)

    def _export_vertices(self, f):
        for vertex in self.vertices:
            vertex.export(f)

    def _export_indices(self, f):
        indices_fmt = FormatSpecifier.array().u32()
        indices = array.array(indices_fmt, self.indices).tobytes()
        f.write(indices)

    def _obtain_mesh(self, object):
        if self.config.apply_modifiers:
            self._mesh = object.to_mesh()
        else:
            self._mesh = object.original.to_mesh()
        self._mesh.calc_tangents()
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
                    "Mesh is not made entirely out of triangles! "
                    f"({self.name})")
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
            normal = self._mesh.loops[loop_index].normal
            tangent = self._mesh.loops[loop_index].tangent
            binormal = self._mesh.loops[loop_index].bitangent
            gl_vertex = BOGLEVertex(self.config)
            gl_vertex.convert(
                self._convert_vec(vertex),
                uv.copy(),
                self._convert_vec(normal),
                self._convert_vec(tangent),
                self._convert_vec(binormal))
            self.vertices.append(gl_vertex)

    def _cleanup(self):
        if self._mesh is not None:
            self._mesh.free_tangents()

        if self._object is not None:
            self._object.to_mesh_clear()
            self._object.original.to_mesh_clear()

        self._object = None
        self._mesh = None
        self._indexed_mesh = []
        self._triangle_vertices = {}
        self._index_map = {}


class BOGLEMaterial(BOGLEBaseObject):
    """Blender material export to BOGLE file"""

    def __init__(self, config):
        super().__init__(config)
        self.type = None
        self.shader = None

        self.ambient_color = None
        self.emissive_color = None
        self.diffuse_color = None
        self.specular_color = None
        self.reflectance = None

        self.opacity = None
        self.specular_power = None
        self.index_of_refraction = None
        self.bump_intensity = None
        self.specular_scale = None
        self.alpha_threshold = None

        self.texture_ambient = None
        self.texture_emission = None
        self.texture_diffuse = None
        self.texture_specular = None
        self.texture_specular_power = None
        self.texture_normal = None
        self.texture_bump = None
        self.texture_opacity = None

    def convert(self, object):
        if not self.config.export_materials:
            self._set_default_values()
            return

        material = self._get_material(object)

        try:
            out_node = material.node_tree.get_output_node('ALL')
        except Exception as e:
            raise BOGLEConversionError(
                "Failed to get material output node: " + str(e))

        if out_node.name != 'Material Output':
            raise BOGLEConversionError(
                "Unexpected material out node: " + str(out_node.name))

        if len(out_node.outputs) != 0:
            raise BOGLEConversionError(
                "Unexpected number of outputs in material out node")
        if len(out_node.inputs) != 3:
            raise BOGLEConversionError(
                "Unexpected number of inputs in material out node")

        try:
            if not out_node.inputs['Surface'].is_linked:
                raise BOGLEConversionError(
                    "Surface value from shader nodes is not linked")
            if out_node.inputs['Displacement'].is_linked:
                print("Ignoring displacement value from shader nodes")
            if out_node.inputs['Volume'].is_linked:
                print("Ignoring volume value from shader nodes")
        except KeyError as e:
            raise BOGLEConversionError(
                "Shader did not have expected input: " + str(e))

        links = out_node.inputs['Surface'].links
        if len(links) > 1:
            raise BOGLEConversionError(
                "Shader output is connected to more than one node")

        uber_node = links[0].from_node
        if len(uber_node.inputs) != 31 or len(uber_node.outputs) != 1:
            raise BOGLEConversionError(
                "Unexpected node connected to shader output")

        self.type = 0
        self.shader = 0

        try:
            inputs = uber_node.inputs

            use_geometry_normals = self._get_float_value(
                inputs, 'Use Geometry Normals')
            use_bumpmap_normals = self._get_float_value(
                inputs, 'Use BumpMap Normals')

            if use_geometry_normals != 0 and use_geometry_normals != 1:
                raise BOGLEConversionError(
                    "Use Geometry Normals should be 1 or 0")
            if use_bumpmap_normals != 0 and use_bumpmap_normals != 1:
                raise BOGLEConversionError(
                    "Use BumpMap Normals should be 1 or 0")

            self.ambient_color = self._get_color_value(inputs, 'Ambient')
            self.emissive_color = self._get_color_value(inputs, 'Emissive')
            self.diffuse_color = self._get_color_value(inputs, 'Diffuse')
            self.specular_color = self._get_color_value(inputs, 'Specular')
            self.reflectance = Vector((0.0, 0.0, 0.0, 0.0))
            self.opacity = self._get_float_value(inputs, 'Opacity')
            self.specular_power = self._get_float_value(
                inputs, 'Specular Power')
            self.index_of_refraction = 0.0
            self.bump_intensity = self._get_float_value(
                inputs, 'Bump Intensity')
            self.specular_scale = self._get_float_value(
                inputs, 'Specular Scale')
            self.alpha_threshold = 0.0

            self.texture_ambient = self._get_texture_name(inputs, 'Ambient')
            self.texture_emission = self._get_texture_name(inputs, 'Emissive')
            self.texture_diffuse = self._get_texture_name(inputs, 'Diffuse')
            self.texture_specular = self._get_texture_name(inputs, 'Specular')
            self.texture_specular_power = self._get_texture_name(
                inputs, 'Specular Power')
            self.texture_opacity = self._get_texture_name(inputs, 'Opacity')

            if use_geometry_normals == 0:
                if use_bumpmap_normals == 1:
                    self.texture_normal = ""
                    self.texture_bump = self._get_texture_name(inputs, 'Bump')
                    if self.texture_bump == "":
                        raise BOGLEConversionError(
                            "Bumpmap normals are enabled but no texture is "
                            "linked")
                else:
                    self.texture_normal = self._get_texture_name(
                        inputs, 'Normal')
                    self.texture_bump = ""
                    if self.texture_normal == "":
                        raise BOGLEConversionError(
                            "Normalmap normals are enabled but no texture "
                            "is linked")
            else:
                self.texture_normal = ""
                self.texture_bump = ""
        except KeyError as e:
            raise BOGLEConversionError(
                "Unexpected node connected to shader output, missing "
                "input: " + str(e))

    def get_name(self, object):
        if self.config.export_materials:
            return self._get_material(object).name
        else:
            return 'default'

    def _get_material(self, object):
        return object.material_slots[0].material

    def _get_color_value(self, inputs, base_name):
        socket_color = inputs['Base ' + base_name]
        socket_alpha = inputs['Base ' + base_name + ' Alpha']

        if not isinstance(socket_color, bpy.types.NodeSocketColor):
            raise BOGLEConversionError(
                "Unexpected socket type: " + str(type(socket_color)) +
                " for " + str(socket_color.name))
        if not isinstance(socket_alpha, bpy.types.NodeSocketFloatFactor):
            raise BOGLEConversionError(
                "Unexpected socket type: " + str(type(socket_alpha)) +
                " for " + str(socket_alpha.name))

        color = self._get_vec3_from_socket(socket_color)
        alpha = self._get_float_from_socket(socket_alpha)
        return Vector((*color, alpha))

    def _get_texture_name(self, inputs, base_name):
        socket = inputs['Texture ' + base_name]
        if not socket.is_linked:
            return ''
        links = socket.links
        if len(links) > 1:
            raise BOGLEConversionError("Too many links")
        node = links[0].from_node
        return node.image.name

    def _get_float_value(self, inputs, name):
        socket = inputs[name]
        return self._get_float_from_socket(socket)

    def _get_float_from_socket(self, socket):
        if socket.is_linked:
            raise BOGLEConversionError(
                "Socket should not be linked: " + socket.name)
        else:
            return socket.default_value

    def _get_vec3_from_socket(self, socket):
        if socket.is_linked:
            raise BOGLEConversionError(
                "Socket should not be linked: " + socket.name)
        else:
            return Vector(socket.default_value).xyz

    def _set_default_values(self):
        self.type = 0
        self.shader = 0

        self.ambient_color = Vector((0.02, 0.02, 0.02, 1.0))
        self.emissive_color = Vector((0.0, 0.0, 0.0, 1.0))
        self.diffuse_color = Vector((0.8, 0.8, 0.8, 1.0))
        self.specular_color = Vector((0.2, 0.2, 0.2, 0.2))
        self.reflectance = Vector((0.0, 0.0, 0.0, 1.0))

        self.opacity = 1.0
        self.specular_power = 50.0
        self.index_of_refraction = 0.0
        self.bump_intensity = 0.0
        self.specular_scale = 0.0
        self.alpha_threshold = 0.0

        self.texture_ambient = ''
        self.texture_emission = ''
        self.texture_diffuse = ''
        self.texture_specular = ''
        self.texture_specular_power = ''
        self.texture_normal = ''
        self.texture_bump = ''
        self.texture_opacity = ''

    def export(self, f):
        fmt = FormatSpecifier().u32().u32().\
            float(4).float(4).float(4).float(4).float(4).\
            float().float().float().float().float().float().format()
        data = (
            self.type, self.shader,
            *self.ambient_color,
            *self.emissive_color,
            *self.diffuse_color,
            *self.specular_color,
            *self.reflectance,
            self.opacity, self.specular_power, self.index_of_refraction,
            self.bump_intensity, self.specular_scale, self.alpha_threshold
        )
        material = struct.pack(fmt, *data)
        f.write(material)

        self._export_texture(self.texture_ambient, f)
        self._export_texture(self.texture_emission, f)
        self._export_texture(self.texture_diffuse, f)
        self._export_texture(self.texture_specular, f)
        self._export_texture(self.texture_specular_power, f)
        self._export_texture(self.texture_normal, f)
        self._export_texture(self.texture_bump, f)
        self._export_texture(self.texture_opacity, f)

    def _export_texture(self, name, f):
        if name:
            name, ext = os.path.splitext(name)
            if ext != '.png':
                print("WARNING: Texture file name doesn't have a png "
                      "extension")
        f.write(struct.pack(FormatSpecifier().u32().format(), len(name)))
        f.write(array.array(FormatSpecifier.array().u8(),
                            name.encode('ascii')).tobytes())


class BOGLELight(BOGLEBaseObject):
    """Blender light export to BOGLE file"""

    def __init__(self, config):
        super().__init__(config)
        self.color = None
        self.range = None
        self.intensity = None
        self.type = None
        self.angle = None

    def convert(self, object):
        light = self._get_light(object)

        self.color = Vector((*light.color, 1.0))
        self.range = light.cutoff_distance
        self.intensity = light.energy

        if light.type == 'SPOT':
            self.type = 0
            self.intensity *= 0.005
        elif light.type == 'SUN':
            self.type = 1
        elif light.type == 'POINT':
            self.type = 2
            self.intensity *= 0.005
        else:
            raise BOGLEConversionError(f"Unsupported light type: {light.type}")

        if light.type == 'SPOT':
            self.angle = light.spot_size
        else:
            self.angle = 0

    def get_name(self, object):
        return self._get_light(object).name

    def export(self, f):
        fmt = FormatSpecifier().float(4).float().float().u8().float().format()
        light = struct.pack(fmt, *self.color, self.range,
                            self.intensity, self.type, self.angle)
        f.write(light)

    def _get_light(self, object):
        return object.data


class BOGLEObject(BOGLEBaseObject):
    """Blender object export to BOGLE file"""

    def __init__(self, config):
        super().__init__(config)

        self.name = None
        self.parent_name = None

        self.camera_idx = None
        self.geometry_idx = None
        self.material_idx = None
        self.light_idx = None

        self.translation = None
        self.rotation_axis = None
        self.rotation_angle = None
        self.scale = None

    def convert(self, object, camera_idx, geometry_idx,
                material_idx, light_idx):
        self.name = object.original.name
        if object.parent is not None:
            self.parent_name = object.parent.name

        self.camera_idx = self._idx(camera_idx)
        self.geometry_idx = self._idx(geometry_idx)
        self.material_idx = self._idx(material_idx)
        self.light_idx = self._idx(light_idx)

        trans, rotq, scale = object.matrix_local.decompose()
        if camera_idx is not None:
            # Blender's default camera orientation is different than ours
            rotq.rotate(Euler((0, -pi/2, 0)))
        axis, self.rotation_angle = rotq.to_axis_angle()
        self.translation = self._convert_vec(trans)
        self.rotation_axis = self._convert_vec(axis)
        self.scale = self._convert_vec(scale, negate_x=False)

    def export(self, f, material_keys=None):
        fmt = FormatSpecifier().u8().u32().u32().u32().\
            float(3).float(3).float().float(3).format()
        data = (
            self.camera_idx, self.geometry_idx,
            self.material_idx, self.light_idx,
            *self.translation,
            *self.rotation_axis, self.rotation_angle,
            *self.scale
        )
        obj = struct.pack(fmt, *data)
        f.write(obj)

    def _idx(self, idx):
        if idx is None:
            return 0
        else:
            return idx + 1


class BOGLExporter(BOGLEBaseObject):
    """Blender export to BOGLE file"""

    def __init__(self, config):
        super().__init__(config)

        self.camera = None
        self.globalAmbientLight = None

        self.geometries = []
        self.geometry_indices = {}
        self.materials = []
        self.material_indices = {}
        self.lights = []
        self.light_indices = {}

        self.objects = []
        self.object_tree = None

    def convert(self, context):
        """Convert the objects that should be converted"""
        depsgraph = context.evaluated_depsgraph_get()

        self.globalAmbientLight = BOGLEAmbientLight(self.config)
        self.globalAmbientLight.convert(self._get_world_color(depsgraph))

        for object in self._iterate_objects(depsgraph):
            type = object.original.type

            if type == 'MESH':
                geo = self._convert_geometry(object)
                mat = self._convert_material(object)
                self._convert_object(object, None, geo, mat, None)
            elif type == 'EMPTY':
                self._convert_object(object, None, None, None, None)
            elif type == 'CAMERA':
                cam = self._convert_camera(object, depsgraph.scene.render)
                self._convert_object(object, cam, None, None, None)
            elif type == 'LIGHT':
                light = self._convert_light(object)
                self._convert_object(object, None, None, None, light)
            else:
                print(f"Not converting object type: {type}")
                continue

        self._convert_object_tree()

    def export(self, filepath):
        """Export converted data to file"""
        with open(filepath, 'wb') as f:
            self._export_header(f)

            self.camera.export(f)
            self.globalAmbientLight.export(f)

            for geometry in self.geometries:
                geometry.export(f)

            for material in self.materials:
                material.export(f)

            for light in self.lights:
                light.export(f)

            for object in self.objects:
                object.export(f)
            self._export_object_tree(f)

    def _iterate_objects(self, depsgraph):
        for object_instance in depsgraph.object_instances:
            object = object_instance.object
            if not self.config.only_selected_objects or \
               self._is_object_selected(object):
                yield object

    def _is_object_selected(self, object):
        selected_parent = object.parent is None or \
            object.parent.original.select_get()
        return selected_parent and object.original.select_get()

    def _get_world_color(self, depsgraph):
        return Vector(depsgraph.scene.world.node_tree.nodes['Background'].
                      inputs['Color'].default_value)

    def _convert_camera(self, object, render):
        if self.camera is not None:
            raise BOGLEConversionError("More than one camera defined")

        cam = BOGLECamera(self.config)
        cam.convert(object, render)
        self.camera = cam
        return 0

    def _convert_geometry(self, object):
        return self._convert_indexed(object, BOGLEGeometry,
                                     self.geometry_indices, self.geometries)

    def _convert_material(self, object):
        return self._convert_indexed(object, BOGLEMaterial,
                                     self.material_indices, self.materials)

    def _convert_light(self, object):
        return self._convert_indexed(object, BOGLELight,
                                     self.light_indices, self.lights)

    def _convert_indexed(self, object, cls, indices, objects):
        obj = cls(self.config)
        name = obj.get_name(object)
        if name in indices:
            return indices[name]

        obj.convert(object)
        objects.append(obj)

        idx = len(objects) - 1
        indices[name] = idx
        return idx

    def _convert_object(self, object, camera_idx, geometry_idx,
                        material_idx, light_idx):
        obj = BOGLEObject(self.config)
        obj.convert(object, camera_idx, geometry_idx, material_idx, light_idx)
        self.objects.append(obj)

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
        fmt = FormatSpecifier.array().u8()
        exported_tree = self.object_tree + b'\0'
        tree = array.array(fmt, exported_tree).tobytes()
        f.write(tree)

    def _export_header(self, f):
        header_fmt = FormatSpecifier().char(5).u8().\
            u32().u32().u32().u32().format()
        header_data = (
            b'B', b'O', b'G', b'L', b'E',  # Magic
            0,  # Version
            len(self.geometries),  # Number of geometries
            len(self.materials),  # Number of materials
            len(self.lights),  # Number of lights
            len(self.objects),  # Number of objects
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

    export_materials: BoolProperty(
        name="Export textures",
        description="If not checked, instead of transforming specially crafted shader node trees into BOGLE textures, use the same default texture for every object",
        default=True,
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
