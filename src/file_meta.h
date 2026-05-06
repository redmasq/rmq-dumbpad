#ifndef DUMBPAD_FILE_META_H
#define DUMBPAD_FILE_META_H

#include <stdint.h>

typedef struct DumbpadFileMetadata {
    uint64_t size_bytes;
    uint64_t modified_time;
    int available;
} DumbpadFileMetadata;

typedef enum DumbpadFileChangeState {
    DUMBPAD_FILE_STATE_UNAVAILABLE = 0,
    DUMBPAD_FILE_STATE_UNCHANGED = 1,
    DUMBPAD_FILE_STATE_CHANGED = 2
} DumbpadFileChangeState;

DumbpadFileChangeState dumbpad_compare_file_metadata(
    DumbpadFileMetadata old_meta,
    DumbpadFileMetadata new_meta);

#endif
