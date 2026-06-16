#ifndef KEYPAD_H
#define KEYPAD_H

typedef enum key {
    KEY_NONE = -1,
    KEY_0 = 0,
    KEY_1,
    KEY_2,
    KEY_3,
    KEY_4,
    KEY_5,
    KEY_6,
    KEY_7,
    KEY_8,
    KEY_9,
    KEY_ENABLE,
    KEY_OK,
    KEY_CANCEL,
    KEY_BACKSLASH,
    KEY_SIGN,
    KEY_DOT,
} key_t;

// export the keymap so it can be easily accesses from other parts of the code
extern const key_t keymap[4][4];

void
keypad_init(void);

#endif
