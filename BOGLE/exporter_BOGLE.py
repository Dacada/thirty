# <pep8-80 compliant>

"""This converter converts the mesh data from a Blender scene into a format
used by this game engine thing: BOGLE.

The resulting file is an extremely simple binary format where everything is
just laid out as is.

Currently, every visible object on the scene is exported. Linked duplicates are
supported. Meaning that two objects with the same mesh in Blender will have the
same geometry component in BOGLE and it will only be defined once.

Cameras, meshes, materials, lights, animations and objects are currently
exported.

"""

import bpy
from bpy_extras.io_utils import ExportHelper
from bpy.props import StringProperty, BoolProperty, EnumProperty
from bpy.types import Operator

from mathutils import Vector, Quaternion, Euler

import struct
import array
import os.path
import re


bl_info = {
    'name': "BOGLE exporter (dev)",
    'description': "Export blender data to a custom binary file format BOGLE",
    'author': "dacada",
    'version': (0, 0),
    'blender': (2, 83, 0),
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
        self.winding_order = other.winding_order
        self.export_materials = other.export_materials


class BOGLEBaseObject:
    def __init__(self, config):
        self.type = None
        self.name = None
        self.config = config

    def convert(self, object):
        self.type = 0
        self.name = self.get_name(object)

    def export_type(self, f):
        fmt = FormatSpecifier().u8().format()
        obj = struct.pack(fmt, self.type)
        f.write(obj)

    def export_name(self, f):
        fmt = FormatSpecifier().u32().format()
        obj = struct.pack(fmt, len(self.name))
        f.write(obj)

        fmt = FormatSpecifier.array().u8()
        name = array.array(fmt, self.name.encode('ascii')).tobytes()
        f.write(name)

    def export(self, f):
        self.export_type(f)
        self.export_name(f)

    def get_name(self, object):
        return object.data.name


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


class BOGLECamera(BOGLEBaseObject):
    """Blender camera export to BOGLE file"""

    def __init__(self, config):
        super().__init__(config)
        self.width = None
        self.height = None
        self.near = None
        self.far = None
        self.fov = None
        self.ismain = None

    def convert(self, object, scene):
        super().convert(object)
        self.type = 0

        camera = object.data

        self.width = scene.render.resolution_x
        self.height = scene.render.resolution_y
        self.near = camera.clip_start
        self.far = camera.clip_end
        self.fov = camera.angle
        self.ismain = object.name == scene.camera.name

    def export(self, f):
        super().export(f)
        fmt = FormatSpecifier().u32(2).float(3).u8().format()
        camera = struct.pack(fmt, self.width, self.height,
                             self.near, self.far, self.fov, self.ismain)
        f.write(camera)


class BOGLEVertex(BOGLEBaseObject):
    """Represents vertex data for OpenGL"""

    def __init__(self, config):
        super().__init__(config)
        self.vert = None
        self.uv = None
        self.norm = None
        self.tang = None
        self.binorm = None
        self.bidxs = None
        self.bwghts = None

    def convert(self, vert, tex, norm, tang, binorm, bone_idxs, bone_weights):
        self.vert = vert
        self.uv = tex
        self.norm = norm
        self.tang = tang
        self.binorm = binorm
        self.bidxs = bone_idxs
        self.bwghts = bone_weights

    def export(self, f):
        """Write vertex data to file"""
        vertex_fmt = FormatSpecifier().float(20).format()
        vertex_data = (
            *self.vert,
            *clamp(self.uv, 0.0, 1.0),
            *self.norm,
            *self.tang,
            *self.binorm,
            *self.bidxs,
            *self.bwghts
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
        super().convert(object)

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
        super().export(f)
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
                    f"({self._object.name})")
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
        # coordinate. HOWEVER! a vertex isn't just the vertex coordinate, it's
        # also the normals (and tangent, and binormal) coordinates. It would be
        # incorrect to consider equal two vertices just because they have the
        # same vertex and uv coordinates, the normals are also important

        def i(f):
            return int(round(f, 5)*10**5)

        indexed_norepeat = {}
        for index in self._indexed_mesh:
            vertex_index, loop_index, triangle_indices = index

            vertex = self._mesh.vertices[vertex_index].co
            uv = self._uv_data[loop_index].uv
            normal = self._mesh.loops[loop_index].normal
            tangent = self._mesh.loops[loop_index].tangent
            binormal = self._mesh.loops[loop_index].bitangent

            coords = (*map(i, vertex), *map(i, uv), *map(i, normal),
                      *map(i, tangent), *map(i, binormal))
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
            bone_idxs, bone_weights = self._get_bone_data(vertex_index)
            gl_vertex = BOGLEVertex(self.config)
            gl_vertex.convert(
                vertex.copy(),
                uv.copy(),
                normal.copy(),
                tangent.copy(),
                binormal.copy(),
                bone_idxs,
                bone_weights)
            self.vertices.append(gl_vertex)

    def _get_bone_data(self, idx):
        bones = {}
        total = 0
        for vg in self._object.vertex_groups:
            try:
                bones[vg.name] = vg.weight(idx)
                total += 1
            except RuntimeError:  # vertex is not part of group
                bones[vg.name] = 0

        if total == 0:
            return Vector((0, 0, 0)), Vector((0.0, 0.0, 0.0))

        if total > 3:
            print(
                f"WARNING! Vertex {idx} is affected by more than 3 bones! "
                "Only the 3 with the most weight will be exported!"
            )

        bones = [i for i in bones.items()]
        bones.sort()  # sorted alphabetically by name
        bones = [(w, i) for i, (n, w) in enumerate(bones)]
        bones.sort(reverse=True)  # sorted by weight ascending
        bones = bones[:3]
        bones.sort(key=lambda e: e[1])  # sorted by index because it's nice

        weights, indices = zip(*bones)
        return Vector(indices), Vector(weights)

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
        self.shader = None

        self.ambient_color = None
        self.emissive_color = None
        self.diffuse_color = None
        self.specular_color = None

        self.opacity = None
        self.specular_power = None
        self.reflectance = None
        self.refraction = None
        self.index_of_refraction = None
        self.bump_intensity = None
        self.specular_scale = None
        self.alpha_threshold = None

        self.alpha_blending_mode = None

        self.texture_ambient = None
        self.texture_emission = None
        self.texture_diffuse = None
        self.texture_specular = None
        self.texture_specular_power = None
        self.texture_normal = None
        self.texture_bump = None
        self.texture_opacity = None

    def convert(self, object):
        super().convert(object)

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
        if len(uber_node.inputs) != 35 or len(uber_node.outputs) != 1:
            raise BOGLEConversionError(
                "Unexpected node connected to shader output")

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
            self.opacity = self._get_float_value(inputs, 'Opacity')
            self.specular_power = self._get_float_value(
                inputs, 'Specular Power')

            self.reflectance = self._get_float_value(inputs, 'Reflectance')
            self.refraction = self._get_float_value(inputs, 'Refraction')
            self.index_of_refraction = self._get_float_value(
                inputs, 'Index of Refraction')

            self.bump_intensity = self._get_float_value(
                inputs, 'Bump Intensity')
            self.specular_scale = self._get_float_value(
                inputs, 'Specular Scale')
            self.alpha_threshold = self._get_float_value(
                inputs, 'Alpha Threshold')
            self.alpha_blending_mode = self._get_float_value(
                inputs, 'Alpha Blending Mode') > 0.001

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
                "input: " + str(e) + "\nFor object: " + object.name)

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
        self.shader = 0

        self.ambient_color = Vector((0.02, 0.02, 0.02, 1.0))
        self.emissive_color = Vector((0.0, 0.0, 0.0, 1.0))
        self.diffuse_color = Vector((0.8, 0.8, 0.8, 1.0))
        self.specular_color = Vector((0.2, 0.2, 0.2, 0.2))

        self.opacity = 1.0
        self.specular_power = 50.0
        self.reflectance = 0.0
        self.index_of_refraction = 0.0
        self.bump_intensity = 0.0
        self.specular_scale = 0.0
        self.alpha_threshold = 0.0
        self.alpha_blending_mode = False

        self.texture_ambient = ''
        self.texture_emission = ''
        self.texture_diffuse = ''
        self.texture_specular = ''
        self.texture_specular_power = ''
        self.texture_normal = ''
        self.texture_bump = ''
        self.texture_opacity = ''

    def export(self, f):
        self.export_type(f)
        f.write(struct.pack(FormatSpecifier().u8().format(), self.shader))
        self.export_name(f)

        fmt = FormatSpecifier().float(24).u8().format()
        data = (
            *self.ambient_color,
            *self.emissive_color,
            *self.diffuse_color,
            *self.specular_color,
            self.opacity, self.specular_power,
            self.reflectance, self.refraction, self.index_of_refraction,
            self.bump_intensity, self.specular_scale, self.alpha_threshold,
            self.alpha_blending_mode
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
        self.attenuation_constant = None
        self.attenuation_linear = None
        self.attenuation_quadratic = None
        self.intensity = None
        self.angle = None

    def convert(self, object):
        super().convert(object)

        light = object.data

        self.color = Vector((*light.color, 1.0))

        self.attenuation_constant = 0
        self.attenuation_quadratic = 1
        d = light.cutoff_distance
        self.attenuation_linear = 1000.0 / d - d

        if light.type == 'SPOT':
            self.type = 0
        elif light.type == 'SUN':
            self.type = 1
        elif light.type == 'POINT':
            self.type = 2
        else:
            raise BOGLEConversionError(f"Unsupported light type: {light.type}")

        # formula derived taking values by eye and applying some curve fitting
        if light.type == 'SPOT' or light.type == 'POINT':
            self.intensity = 250.0+(-0.5-250.0)/(1.0+(light.energy/15.0)**0.7)
        else:
            self.intensity = light.energy

        if light.type == 'SPOT':
            self.angle = light.spot_size
        else:
            self.angle = 0

    def export(self, f):
        super().export(f)
        fmt = FormatSpecifier().float(9).format()
        light = struct.pack(fmt, *self.color,
                            self.attenuation_constant,
                            self.attenuation_linear,
                            self.attenuation_quadratic,
                            self.intensity, self.angle)
        f.write(light)


class BOGLEKeyframe(BOGLEBaseObject):
    def __init__(self, config):
        super().__init__(config)
        self.timestamp = None
        self.rootOffset = None
        self.boneRotations = None

    def convert(self, frame, fcurves, boneNames, fps):
        self.timestamp = frame/fps

        offset = Vector()
        rotations = {name: Quaternion() for name in boneNames}
        for fcurve in fcurves:
            path = self._split_data_path(fcurve.data_path)

            if path[0] != 'pose':
                raise BOGLEConversionError(
                    f"Can only convert pose animations not {path[0]}")

            m = re.match(r'bones\["(.*)"\]', path[1])
            if m is None:
                raise BOGLEConversionError(
                    f"Can only convert bone animations not {path[1]}")
            name = m.group(1)

            if path[2] == 'rotation_quaternion':
                q = rotations[name]
                if type(q) is not Quaternion:
                    q = q.to_quaternion()
                q[fcurve.array_index] = fcurve.evaluate(frame)
                rotations[name] = q
            elif path[2] == 'rotation_euler':
                q = rotations[name]
                if type(q) is not Euler:
                    q = q.to_euler()
                q[fcurve.array_index] = fcurve.evaluate(frame)
                rotations[name] = q
            elif path[2] == 'location':
                offset[fcurve.array_index] = fcurve.evaluate(frame)
            else:
                raise BOGLEConversionError(
                    f"Can only convert rotation or locatio not {path[2]}")

        self.rootOffset = offset
        self.boneRotations = [rot.to_quaternion()
                              if type(rot) is not Quaternion
                              else
                              rot
                              for _, rot in sorted(rotations.items())]

    def _split_data_path(self, data_path):
        splt = []
        curr = ''
        splitting = True

        for c in data_path:
            if c == '.' and splitting:
                splt.append(curr)
                curr = ''
            else:
                curr += c
                if c == '"':
                    splitting = not splitting

        if curr:
            splt.append(curr)

        return splt

    def export(self, f):
        fmt = FormatSpecifier().float(4).format()
        keyframe = struct.pack(fmt, self.timestamp, *self.rootOffset)
        f.write(keyframe)

        fmt = FormatSpecifier().float(4).format()
        for rot in self.boneRotations:
            rotCglm = (rot.x, rot.y, rot.z, rot.w)
            rotStr = struct.pack(fmt, *rotCglm)
            f.write(rotStr)


class BOGLEAnimation(BOGLEBaseObject):
    def __init__(self, config):
        super().__init__(config)
        self.keyframes = None

    def convert(self, action, boneNames, fps):
        super().convert(action)
        # it's possible that an animation controls two values but those two
        # values have different keyframes, so collect every single used
        # keyframe: the exported animation will have as many keyframes as there
        # exist in total. For example, if one rotation has keyframes at 3, 7
        # and 10 and another at 5, 7 and 9, we will export keyframes 3, 5, 7, 9
        # and 10 for both rotations
        keyframe_points = set(kfp.co.x
                              for fcurve in action.fcurves
                              for kfp in fcurve.keyframe_points)
        keyframe_points = sorted(keyframe_points)

        self.keyframes = []
        for point in keyframe_points:
            keyframe = BOGLEKeyframe(self.config)
            keyframe.convert(point, action.fcurves, boneNames, fps)
            self.keyframes.append(keyframe)

    def export(self, f):
        self.export_name(f)

        fmt = FormatSpecifier().u32().format()
        anim = struct.pack(fmt, len(self.keyframes))
        f.write(anim)

        for keyframe in self.keyframes:
            keyframe.export(f)

    def get_name(self, action):
        return action.name


class BOGLEBone(BOGLEBaseObject):
    def __init__(self, config):
        super().__init__(config)
        self.position = None
        self.rotation = None
        self.parent = None

    def convert(self, position, rotation, parent):
        self.position = position
        self.rotation = rotation
        self.rotation.normalize()

        if parent is None:
            self.parent = 0
        else:
            self.parent = parent + 1

    def export(self, f):
        fmt = FormatSpecifier().float(7).u32().format()
        rotation = (
            self.rotation.x,
            self.rotation.y,
            self.rotation.z,
            self.rotation.w
        )
        bone = struct.pack(fmt, *self.position, *rotation, self.parent)
        f.write(bone)


class BOGLESkeleton(BOGLEBaseObject):
    def __init__(self, config):
        super().__init__(config)
        self.model = None
        self.bones = None

    def convert(self, model, armature):
        self.model = model

        bones = {bone.name: bone for bone in armature.bones}
        boneNames = sorted(bones.keys())

        self.bones = []
        for name in boneNames:
            bone = bones[name]
            if bone.parent is None:
                pos, rot, _ = bone.matrix_local.decompose()
                parentIdx = None
            else:
                rot = bone.matrix.to_quaternion()
                pos = bone.parent.vector + bone.head
                pos.rotate(bone.parent.matrix.inverted())
                pos.rotate(bone.matrix.inverted())
                parentIdx = boneNames.index(bone.parent.name)

            b = BOGLEBone(self.config)
            b.convert(pos, rot, parentIdx)
            self.bones.append(b)

        return boneNames

    def export(self, f):
        fmt = FormatSpecifier().float(16).u32().format()
        cols = (
            *self.model.col[0],
            *self.model.col[1],
            *self.model.col[2],
            *self.model.col[3]
        )
        skel = struct.pack(fmt, *cols, len(self.bones))
        f.write(skel)

        for bone in self.bones:
            bone.export(f)


class BOGLEAnimationCollection(BOGLEBaseObject):
    def __init__(self, config):
        super().__init__(config)
        self.skeleton = None
        self.animations = None

    def convert(self, object, fps):
        armature = object.parent

        super().convert(armature)

        # Need to know where the armature is in relation to the object it's
        # animating
        model = object.matrix_local.inverted()

        self.skeleton = BOGLESkeleton(self.config)
        boneNames = self.skeleton.convert(model, armature.data)

        self.animations = []
        for nla_track in armature.animation_data.nla_tracks:
            for strip in nla_track.strips:
                animation = BOGLEAnimation(self.config)
                animation.convert(strip.action, boneNames, fps)
                self.animations.append(animation)

    def export(self, f):
        super().export(f)

        fmt = FormatSpecifier().u32().format()
        col = struct.pack(fmt, len(self.animations))
        f.write(col)

        self.skeleton.export(f)

        for animation in self.animations:
            animation.export(f)


class BOGLEObject(BOGLEBaseObject):
    """Blender object export to BOGLE file"""

    def __init__(self, config):
        super().__init__(config)

        self.parent_name = None

        self.camera_idx = None
        self.geometry_idx = None
        self.material_idx = None
        self.light_idx = None
        self.animation_idx = None
        self.transform = None

    def convert(self, object, camera_idx, geometry_idx,
                material_idx, light_idx, animation_idx):
        super().convert(object)

        transform = object.matrix_local.copy()

        parent = object.parent
        while parent is not None and parent.type == 'ARMATURE':
            transform = parent.matrix_local @ transform
            parent = parent.parent
        if parent is not None:
            self.parent_name = parent.name

        self.camera_idx = self._idx(camera_idx)
        self.geometry_idx = self._idx(geometry_idx)
        self.material_idx = self._idx(material_idx)
        self.light_idx = self._idx(light_idx)
        self.animation_idx = self._idx(animation_idx)
        self.transform = transform.copy()

    def export(self, f, material_keys=None):
        self.export_name(f)

        fmt = FormatSpecifier().u32(5).float(16).format()
        data = (
            self.camera_idx, self.geometry_idx,
            self.material_idx, self.light_idx,
            self.animation_idx,
            *self.transform.col[0],
            *self.transform.col[1],
            *self.transform.col[2],
            *self.transform.col[3],
        )
        obj = struct.pack(fmt, *data)
        f.write(obj)

    def _idx(self, idx):
        if idx is None:
            return 0
        else:
            return idx + 1

    def get_name(self, object):
        return object.original.name


class BOGLExporter(BOGLEBaseObject):
    """Blender export to BOGLE file"""

    def __init__(self, config):
        super().__init__(config)

        self.globalAmbientLight = None

        self.cameras = []
        self.camera_indices = {}
        self.geometries = []
        self.geometry_indices = {}
        self.materials = []
        self.material_indices = {}
        self.lights = []
        self.light_indices = {}
        self.animation_collections = []
        self.animation_collection_indices = {}

        self.objects = []
        self.object_tree = None

    def convert(self, context):
        """Convert the objects that should be converted"""
        depsgraph = context.evaluated_depsgraph_get()

        self.globalAmbientLight = BOGLEAmbientLight(self.config)
        self.globalAmbientLight.convert(self._get_world_color(depsgraph))

        nlights = 0
        for object in self._iterate_objects(depsgraph):
            type = object.original.type

            if type == 'MESH':
                geo = self._convert_geometry(object)
                mat = self._convert_material(object)
                animation_collection = self._convert_animation_collection(
                    object, depsgraph.scene.render.fps)
                self._convert_object(object, None, geo, mat, None,
                                     animation_collection)
            elif type == 'EMPTY':
                self._convert_object(object, None, None, None, None, None)
            elif type == 'CAMERA':
                cam = self._convert_camera(object, depsgraph.scene)
                self._convert_object(object, cam, None, None, None, None)
            elif type == 'LIGHT':
                light = self._convert_light(object)
                # we have a limit on lights, not just on light instances
                if light is not None:
                    nlights += 1
                self._convert_object(object, None, None, None, light, None)
            elif type == 'ARMATURE':
                pass  # Done when finding the mesh object
            else:
                print(f"Not converting object type: {type}")
                continue

        if nlights > 20:
            raise BOGLEConversionError("Too many lights!")

        self._convert_object_tree()

    def export(self, filepath):
        """Export converted data to file"""
        with open(filepath, 'wb') as f:
            self._export_header(f)

            self.globalAmbientLight.export(f)

            for camera in self.cameras:
                camera.export(f)

            for geometry in self.geometries:
                geometry.export(f)

            for material in self.materials:
                material.export(f)

            for light in self.lights:
                light.export(f)

            for animation_collection in self.animation_collections:
                animation_collection.export(f)

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

    def _convert_camera(self, object, scene):
        return self._convert_indexed(object, BOGLECamera,
                                     self.camera_indices, self.cameras, scene)

    def _convert_geometry(self, object):
        return self._convert_indexed(object, BOGLEGeometry,
                                     self.geometry_indices, self.geometries)

    def _convert_material(self, object):
        return self._convert_indexed(object, BOGLEMaterial,
                                     self.material_indices, self.materials)

    def _convert_light(self, object):
        return self._convert_indexed(object, BOGLELight,
                                     self.light_indices, self.lights)

    def _convert_animation_collection(self, object, fps):
        if object.parent is None or object.parent.type != 'ARMATURE':
            return None
        return self._convert_indexed(object, BOGLEAnimationCollection,
                                     self.animation_collection_indices,
                                     self.animation_collections, fps)

    def _convert_indexed(self, object, cls, indices, objects, *args):
        obj = cls(self.config)
        name = obj.get_name(object)
        if name in indices:
            return indices[name]

        obj.convert(object, *args)
        objects.append(obj)

        idx = len(objects) - 1
        indices[name] = idx
        return idx

    def _convert_object(self, object, camera_idx, geometry_idx,
                        material_idx, light_idx, animation_idx):
        obj = BOGLEObject(self.config)
        obj.convert(object, camera_idx, geometry_idx, material_idx,
                    light_idx, animation_idx)
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
        header_fmt = FormatSpecifier().char(5).u8().u32(6).format()
        header_data = (
            b'B', b'O', b'G', b'L', b'E',  # Magic
            0,  # Version
            len(self.cameras),
            len(self.geometries),
            len(self.materials),
            len(self.lights),
            len(self.animation_collections),
            len(self.objects),
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
