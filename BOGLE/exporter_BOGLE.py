import bpy
import struct
import array
    
def vecEq(v1, v2):
    return abs((v1-v2).magnitude) < 0.00001

def _mesh_to_indices(mesh):
    """
    Given a mesh made entirely of triangular polygons, return a list of 3-tuples. The first element is
    an index of a vertex. The second is an index of a loop (to be used for finding one of the vertex's
    corresponding uv coordinates) and the third is a list of only one element, the index of the polygon
    (triangle) this vertex belongs to.
    """
    triangles_count = 0
    idx_verts = []
    for triangle_idx, triangle in enumerate(mesh.polygons):
        if triangle.loop_total != 3:
            raise Exception("Non triangle found")
        triangles_count += 1
        for loop_index in triangle.loop_indices:
            vertex_index = mesh.loops[loop_index].vertex_index
            idx_verts.append((vertex_index, loop_index, [triangle_idx]))
    return idx_verts, triangles_count

def _unrepeat_indices(idx_verts, mesh, uv_layer):
    """
    Given a mesh object and the list of indices produced by mesh_to_indices, remove all repetitions while
    keeping information of the triangles. A repeat is removed if it has the same vertex and the same uv
    (either by id or by its value, with a certain tolerance). When an index is removed, the sole triangle
    index it had is added to the list of triangle indices of the unique 3-tuple that is left.
    """
    idx_verts_norepeats = []
    for idx_vert_this in idx_verts:
        is_repeated = False
        for idx_vert_other in idx_verts_norepeats:
            this_vertex_idx, this_loop_idx, this_triangle_idxs = idx_vert_this
            other_vertex_idx, other_loop_idx, other_triangle_idxs = idx_vert_other
            
            this_vertex = mesh.vertices[this_vertex_idx].co
            other_vertex = mesh.vertices[other_vertex_idx].co
            
            this_uv = uv_layer.data[this_loop_idx].uv
            other_uv = uv_layer.data[other_loop_idx].uv
            
            if this_vertex_idx == other_vertex_idx or vecEq(this_vertex, other_vertex):
                if this_loop_idx == other_loop_idx or vecEq(this_uv, other_uv):
                    other_triangle_idxs.append(this_triangle_idxs[0])
                    is_repeated = True
                    break
        if not is_repeated:
            idx_verts_norepeats.append(idx_vert_this)
    return idx_verts_norepeats

def _indices_to_triangles(idx_verts):
    """
    Transform the 3-tuple list from unrepeat_indices into a dictionary where each key is the index of a
    triangle (a polygon) from the original mesh, and the value is a list with the three 2-tuples representing
    the vertex index and loop index corresponding to the vertex coordinate and uv coordinate at one of the
    vertices of the triangle.
    """
    triangle_indices = {}
    for idx_vert in idx_verts:
        vertex_idx, loop_idx, triangle_idxs = idx_vert
        for triangle_idx in triangle_idxs:
            vertices_loops_list = triangle_indices.setdefault(triangle_idx, [])
            vertices_loops_list.append((vertex_idx, loop_idx))
    return triangle_indices

def _get_indices_mapping(triangle_indices):
    """
    Each 2-tuple in triangle_indices is a "vertex" as we understand them in OpenGl (or it will be once we find the
    actual data instead of just the indices). So now we map each of these tuples to an "index" as we understand
    them in OpenGl (for index drawing).
    """
    tuples = [tple for tuples in triangle_indices.values() for tple in tuples]
    unique_tuples = set(tuples)
    indices = {tple: i for i, tple in enumerate(unique_tuples)}
    return indices

def _make_indices(triangles_count, triangle_indices, indices):
    """
    Create the final OpenGL datastructure, a list of indices ordered in such a way that each group of three forms a
    triangle. TODO: ENSURE THE WINDING ORDER IS CORRECT!!! WE WILL NEED NORMAL DATA FOR THIS I GUESS???
    """
    opengl_indices = []
    for triangle_index in range(triangles_count):
        opengl_indices.extend(map(indices.get, triangle_indices[triangle_index]))
    return opengl_indices

def _make_vertices(mesh, uv_layer, indices):
    """
    Create the final OpenGL datastructure, a list of coordinates. Three vertex coordinates followed by two UV
    (texture) coordinates. Repeating this pattern until we've done them all. Following the ordering provided by
    indices, go through each 2-tuple and convert it into the actual set of coordinates (3 + 2 floats) which are
    added flatly to the final list.
    """
    reverse_indices = {v:k for k,v in indices.items()}
    opengl_vertices = []
    for triangle_index in sorted(reverse_indices):
        vertex_idx, loop_idx = reverse_indices[triangle_index]
        vertex = mesh.vertices[vertex_idx].co
        uv = uv_layer.data[loop_idx].uv
        coordinates = (vertex.x, vertex.y, vertex.z, uv.x, uv.y)
        opengl_vertices.extend(coordinates)
    return opengl_vertices
    
def object_to_opengl(object):
    mesh = object.data # TODO: Apply modifiers
    uv_layer = mesh.uv_layers.active
    
    idx_verts, triangles_count = _mesh_to_indices(mesh)
    idx_verts_norepeats = _unrepeat_indices(idx_verts, mesh, uv_layer)
    del idx_verts # some lists have been updated in place, this is no longer reliable
    
    triangle_indices = _indices_to_triangles(idx_verts_norepeats)
    indices = _get_indices_mapping(triangle_indices)
    
    opengl_indices = _make_indices(triangles_count, triangle_indices, indices)
    opengl_vertices = _make_vertices(mesh, uv_layer, indices)
    
    return opengl_vertices, opengl_indices

def export(file):
    objects = []
    for scene in bpy.data.scenes:
        seen_objects = set()
        for object in scene.objects:
            if object.type == 'MESH':
                if object.name not in seen_objects:
                    seen_objects.add(object.name)
                    verts, idxs = object_to_opengl(object)
                    objects.append((object.name, verts, idxs))
    
    header_fmt = "<cccccBL"
    header_data = [
        b'B', b'O', b'G', b'L', b'E', # Magic
        0, # Version
        len(objects) # Number of objects
    ]
    
    header = struct.pack(header_fmt, *header_data)
    file.write(header)
    
    for object in objects:
        object_header_fmt = "<BLL"
        object_header_data = [
            len(object[0]), # Length of object name
            len(object[1])//5, # Number of vertices
            len(object[2]) # Number of indices
        ]
        object_header = struct.pack(object_header_fmt, *object_header_data)
        
        name = array.array('B', object[0].encode('ascii')).tobytes()
        vertices = array.array('f', object[1]).tobytes()
        indices = array.array('I', object[2]).tobytes()
        data = name + vertices + indices
        file.write(object_header + data)


def write_some_data(context, filepath):
    print("running BOGLE export...")
    with open(filepath, 'wb') as f:
        export(f)
    return {'FINISHED'}


# ExportHelper is a helper class, defines filename and
# invoke() function which calls the file selector.
from bpy_extras.io_utils import ExportHelper
from bpy.props import StringProperty, BoolProperty, EnumProperty
from bpy.types import Operator


class ExportSomeData(Operator, ExportHelper):
    """Export all objects in all scene to my BOGLE file format for OpenGl."""
    bl_idname = "bogle_export.all_objects"  # important since its how bpy.ops.import_test.some_data is constructed
    bl_label = "Export All Objects to BOGLE"

    # ExportHelper mixin class uses this
    filename_ext = ".bgl"

    filter_glob: StringProperty(
        default="*.bgl",
        options={'HIDDEN'},
        maxlen=255,  # Max internal buffer length, longer would be clamped.
    )

    # List of operator properties, the attributes will be assigned
    # to the class instance from the operator settings before calling.
    # use_setting: BoolProperty(
    #     name="Example Boolean",
    #     description="Example Tooltip",
    #     default=True,
    # )

    # type: EnumProperty(
    #     name="Example Enum",
    #     description="Choose between two items",
    #     items=(
    #         ('OPT_A', "First Option", "Description one"),
    #         ('OPT_B', "Second Option", "Description two"),
    #     ),
    #     default='OPT_A',
    # )

    def execute(self, context):
        return write_some_data(context, self.filepath) #self.use_setting


# Only needed if you want to add into a dynamic menu
def menu_func_export(self, context):
    self.layout.operator(ExportSomeData.bl_idname, text="Export to BOGLE (in-test trash)")


def register():
    bpy.utils.register_class(ExportSomeData)
    bpy.types.TOPBAR_MT_file_export.append(menu_func_export)


def unregister():
    bpy.utils.unregister_class(ExportSomeData)
    bpy.types.TOPBAR_MT_file_export.remove(menu_func_export)


if __name__ == "__main__":
    register()
