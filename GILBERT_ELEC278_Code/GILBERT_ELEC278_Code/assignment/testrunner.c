#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defs.h"
#include "interface.h"
#include "model.h"
#include "tests.h"
#include "testrunner.h"


static char display[NUM_ROWS][NUM_COLS][CELL_DISPLAY_WIDTH + 1];

int main() {
    memset(display, 0, sizeof(display));
    run_tests();
    return 0;
}

void update_cell_display(ROW row, COL col, const char *text) {
    snprintf(display[row][col], CELL_DISPLAY_WIDTH + 1, "%s", text);
}

void assert_display_text(ROW row, COL col, const char *text) {
    assert(strcmp(text, display[row][col]) == 0);
}

void assert_edit_text(ROW row, COL col, const char *text) {
    char *value = get_textual_value(row, col);
    assert(value != NULL && strcmp(text, value) == 0);
    free(value);
}
