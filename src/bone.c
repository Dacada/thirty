#include <thirty/bone.h>
#include <thirty/util.h>

void bone_initFromFile(struct bone *const bone, FILE *const f) {
        sfread(bone->positionRelative.raw, sizeof(float), 3, f);
        sfread(bone->rotationRelative.raw, sizeof(float), 4, f);

        uint32_t parent;
        sfread(&parent, sizeof(uint32_t), 1, f);
        bone->parent = parent;
}
