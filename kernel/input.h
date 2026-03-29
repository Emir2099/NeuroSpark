#ifndef INPUT_H
#define INPUT_H

void keyboard_handler(void);
void mouse_handler(void);
void init_input_stack(void);

extern volatile int mouse_x;
extern volatile int mouse_y;
extern volatile int mouse_buttons;

#endif
