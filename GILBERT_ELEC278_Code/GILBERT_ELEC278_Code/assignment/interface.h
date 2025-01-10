#ifndef ASSIGNMENT_INTERFACE_H
#define ASSIGNMENT_INTERFACE_H

#include "defs.h"

#define CELL_DISPLAY_WIDTH 11

// Updates the text which is displayed in a cell.
//
// The contents of 'text' are only accessed during the function call, and the
// function does not modify or deallocate the string. Only the first
// CELL_DISPLAY_WIDTH characters will be used.
void update_cell_display(ROW row, COL col, const char *text);

#endif //ASSIGNMENT_INTERFACE_H
