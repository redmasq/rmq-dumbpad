#include <stdio.h>

int test_text_encoding(void);
int test_file_meta(void);
int test_line_endings(void);

int
main(void)
{
    if (test_text_encoding() != 0) {
        return 1;
    }
    if (test_file_meta() != 0) {
        return 1;
    }
    if (test_line_endings() != 0) {
        return 1;
    }

    puts("All tests passed.");
    return 0;
}
