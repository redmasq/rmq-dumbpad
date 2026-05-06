#include "file_meta.h"

DumbpadFileChangeState
dumbpad_compare_file_metadata(DumbpadFileMetadata old_meta, DumbpadFileMetadata new_meta)
{
    if (!old_meta.available || !new_meta.available) {
        return DUMBPAD_FILE_STATE_UNAVAILABLE;
    }

    if (old_meta.size_bytes != new_meta.size_bytes ||
        old_meta.modified_time != new_meta.modified_time) {
        return DUMBPAD_FILE_STATE_CHANGED;
    }

    return DUMBPAD_FILE_STATE_UNCHANGED;
}
