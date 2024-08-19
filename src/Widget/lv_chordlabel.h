#pragma once

#include <lvgl.h>
#include "Chord.h"

typedef struct {
    lv_obj_t obj;
    Chord chord;
} lv_chordlabel_t;

extern const lv_obj_class_t lv_chordlabel_class;

/**
 * Create a chordlabel object
 * @param parent pointer to an object, it will be the parent of the new line
 * @return pointer to the created line
 */
lv_obj_t * lv_chordlabel_create(lv_obj_t * parent);

/**
 * Set a chord.
 * @param obj           pointer to a line object
 * @param chord         a chord
 */
void lv_chordlabel_set_chord(lv_obj_t * obj, const Chord &chord);
Chord lv_chordlabel_get_chord(lv_obj_t * obj);
