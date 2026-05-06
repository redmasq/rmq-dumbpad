#include "test.h"

#include "../src/file_meta.h"

int
test_file_meta(void)
{
    DumbpadFileMetadata a = {123, 456, 1};
    DumbpadFileMetadata b = {123, 456, 1};
    DumbpadFileMetadata c = {124, 456, 1};
    DumbpadFileMetadata d = {123, 789, 1};
    DumbpadFileMetadata unavailable = {0, 0, 0};

    TEST_ASSERT(dumbpad_compare_file_metadata(a, b) == DUMBPAD_FILE_STATE_UNCHANGED);
    TEST_ASSERT(dumbpad_compare_file_metadata(a, c) == DUMBPAD_FILE_STATE_CHANGED);
    TEST_ASSERT(dumbpad_compare_file_metadata(a, d) == DUMBPAD_FILE_STATE_CHANGED);
    TEST_ASSERT(dumbpad_compare_file_metadata(a, unavailable) == DUMBPAD_FILE_STATE_UNAVAILABLE);

    return 0;
}
