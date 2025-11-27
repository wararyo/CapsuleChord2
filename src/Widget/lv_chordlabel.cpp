#include "lv_chordlabel.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/*********************
 *      DEFINES
 *********************/
#define MY_CLASS &lv_chordlabel_class

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void lv_chordlabel_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj);
static void lv_chordlabel_event(const lv_obj_class_t *class_p, lv_event_t *e);
static void draw_main(lv_event_t *e);

/**********************
 *  STATIC VARIABLES
 **********************/
const lv_obj_class_t lv_chordlabel_class = {
    .base_class = &lv_obj_class,
    .constructor_cb = lv_chordlabel_constructor,
    .event_cb = lv_chordlabel_event,
    .width_def = LV_SIZE_CONTENT,
    .height_def = LV_SIZE_CONTENT,
    .instance_size = sizeof(lv_chordlabel_t),
};

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t *lv_chordlabel_create(lv_obj_t *parent)
{
    LV_LOG_INFO("begin");
    lv_obj_t *obj = lv_obj_class_create_obj(MY_CLASS, parent);
    lv_obj_class_init_obj(obj);
    return obj;
}

/*=====================
 * Setter functions
 *====================*/

void lv_chordlabel_set_chord(lv_obj_t *obj, const Chord &chord)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_chordlabel_t *cl = (lv_chordlabel_t *)obj;
    cl->chord = Chord(&chord);

    lv_obj_refresh_self_size(obj);

    lv_obj_invalidate(obj);
}

/*=====================
 * Getter functions
 *====================*/

Chord lv_chordlabel_get_chord(lv_obj_t *obj)
{
    LV_ASSERT_OBJ(obj, MY_CLASS);

    lv_chordlabel_t *cl = (lv_chordlabel_t *)obj;

    return cl->chord;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void lv_chordlabel_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj)
{
    LV_UNUSED(class_p);
    LV_TRACE_OBJ_CREATE("begin");

    lv_chordlabel_t *cl = (lv_chordlabel_t *)obj;

    cl->chord = Chord();

    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    
    // デフォルトフォントを変更
    LV_ASSERT_OBJ(obj, MY_CLASS);
    lv_style_t *style = (lv_style_t *)lv_mem_alloc(sizeof(lv_style_t));
    lv_style_init(style);
    lv_style_set_text_font(style, &mnoto_40);
    lv_obj_add_style(obj, style, 0);
    lv_obj_refresh_self_size(obj);
    lv_obj_invalidate(obj);

    LV_TRACE_OBJ_CREATE("finished");
}

static void lv_chordlabel_event(const lv_obj_class_t *class_p, lv_event_t *e)
{
    LV_UNUSED(class_p);

    lv_res_t res;

    /*Call the ancestor's event handler*/
    res = lv_obj_event_base(MY_CLASS, e);
    if(res != LV_RES_OK) return;

    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);

    if (code == LV_EVENT_STYLE_CHANGED)
    {
    }
    else if (code == LV_EVENT_REFR_EXT_DRAW_SIZE)
    {
        const lv_font_t *font = lv_obj_get_style_text_font(obj, LV_PART_MAIN);
        lv_coord_t font_h = lv_font_get_line_height(font);
        lv_event_set_ext_draw_size(e, font_h / 4);
    }
    else if (code == LV_EVENT_SIZE_CHANGED)
    {
    }
    else if (code == LV_EVENT_GET_SELF_SIZE)
    {
        lv_point_t size;
        lv_chordlabel_t *cl = (lv_chordlabel_t *)obj;
        char text[64] = {0};
        snprintf(text, sizeof(text), "%s", cl->chord.toString().c_str());
        const lv_font_t *font = lv_obj_get_style_text_font(obj, LV_PART_MAIN);
        lv_coord_t letter_space = lv_obj_get_style_text_letter_space(obj, LV_PART_MAIN);
        lv_coord_t line_space = lv_obj_get_style_text_line_space(obj, LV_PART_MAIN);
        lv_text_flag_t flag = LV_TEXT_FLAG_NONE;

        lv_coord_t w = lv_obj_get_content_width(obj);
        if (lv_obj_get_style_width(obj, LV_PART_MAIN) == LV_SIZE_CONTENT && !obj->w_layout) w = LV_COORD_MAX;
        else w = lv_obj_get_content_width(obj);

        lv_txt_get_size(&size, text, font, letter_space, line_space, w, flag);

        lv_point_t *self_size = (lv_point_t *)lv_event_get_param(e);
        self_size->x = LV_MAX(self_size->x, size.x);
        self_size->y = LV_MAX(self_size->y, size.y);
    }
    else if (code == LV_EVENT_DRAW_MAIN)
    {
        draw_main(e);
    }
}

// MNotoで正しく表示するためには特殊な表記をする必要があるため、Chord::toString()とは別で実装する
const std::vector<String> rootStrings = {"C","C[","D","D[","E","F","F[","G","G[","A","A[","B"};
static String chord_to_string(const Chord &chord)
{
    String str = rootStrings[chord.root];
    
    // 3度
    if(chord.option & Chord::Sus4) {
        str += "sus$";
    } else if(chord.option & Chord::Sus2) {
        str += "sus\"";
    } else if(chord.option & Chord::Dimish) {
        str += "dim";
    } else if(chord.option & Chord::Aug) {
        str += "aug";
    } else if(chord.option & Chord::Minor) {
        str += "m";
    } // Major is implied
    
    // 7度
    if(chord.option & Chord::MajorSeventh) {
        str += "M7";
    } else if(chord.option & Chord::Seventh) {
        str += "7";
    } else if(chord.option & Chord::Sixth) {
        str += "6";
    }
    
    // 5度
    bool fifthFlat = (chord.option & Chord::FifthFlat) && !(chord.option & Chord::Dimish);
    
    // 括弧の中に含める文字
    bool hasExtensions = false;
    String extensions = "";
    
    // フラットファイブとテンションが同時にある場合、フラットファイブは括弧の中に含める
    if(fifthFlat && (chord.option & 0b1111111000000000)) {
        extensions += "]5";
        hasExtensions = true;
    }
    
    // テンション
    if(chord.option & Chord::Ninth || chord.option & Chord::NinthSharp) {
        if(hasExtensions) extensions += "?";
        if(chord.option & Chord::NinthSharp) {
            extensions += "[9";
        } else {
            extensions += "9";
        }
        hasExtensions = true;
    }
    
    if(chord.option & Chord::Eleventh || chord.option & Chord::EleventhSharp) {
        if(hasExtensions) extensions += "?";
        if(chord.option & Chord::EleventhSharp) {
            extensions += "[11";
        } else {
            extensions += "11";
        }
        hasExtensions = true;
    }
    
    if(chord.option & Chord::Thirteenth || chord.option & Chord::ThirteenthSharp || chord.option & Chord::ThirteenthFlat) {
        if(hasExtensions) extensions += "?";
        if(chord.option & Chord::ThirteenthSharp) {
            extensions += "[13";
        } else if(chord.option & Chord::ThirteenthFlat) {
            extensions += "]13";
        } else {
            extensions += "13";
        }
        hasExtensions = true;
    }
    
    // 算出した文字列の合成
    if(hasExtensions) {
        str += "{" + extensions + "}";
    } else if(fifthFlat) {
        // フラットファイブは括弧の中に含めない
        str += "-5";
    }
    
    // Add slash chord notation if a bass note is specified
    if (chord.bass != Chord::BASS_DEFAULT) {
        str += "/" + rootStrings[chord.bass];
    }
    
    return str;
}

static void draw_main(lv_event_t *e)
{
    lv_obj_t *obj = lv_event_get_target(e);
    lv_chordlabel_t *cl = (lv_chordlabel_t *)obj;
    char text[64] = {0};
    snprintf(text, sizeof(text), "%s", chord_to_string(cl->chord).c_str());
    lv_draw_ctx_t *draw_ctx = lv_event_get_draw_ctx(e);

    lv_area_t txt_coords;
    lv_obj_get_content_coords(obj, &txt_coords);

    lv_text_flag_t flag = LV_TEXT_FLAG_NONE;
    if (lv_obj_get_style_width(obj, LV_PART_MAIN) == LV_SIZE_CONTENT && !obj->w_layout)
        flag |= LV_TEXT_FLAG_FIT;

    lv_draw_label_dsc_t label_draw_dsc;
    lv_draw_label_dsc_init(&label_draw_dsc);

    label_draw_dsc.ofs_x = 0;
    label_draw_dsc.ofs_y = 0;

    label_draw_dsc.flag = flag;
    lv_obj_init_draw_label_dsc(obj, LV_PART_MAIN, &label_draw_dsc);
    lv_bidi_calculate_align(&label_draw_dsc.align, &label_draw_dsc.bidi_dir, text);

    label_draw_dsc.sel_start = lv_label_get_text_selection_start(obj);
    label_draw_dsc.sel_end = lv_label_get_text_selection_end(obj);
    if (label_draw_dsc.sel_start != LV_DRAW_LABEL_NO_TXT_SEL && label_draw_dsc.sel_end != LV_DRAW_LABEL_NO_TXT_SEL)
    {
        label_draw_dsc.sel_color = lv_obj_get_style_text_color_filtered(obj, LV_PART_SELECTED);
        label_draw_dsc.sel_bg_color = lv_obj_get_style_bg_color(obj, LV_PART_SELECTED);
    }

    lv_area_t txt_clip;
    bool is_common = _lv_area_intersect(&txt_clip, &txt_coords, draw_ctx->clip_area);
    if(!is_common) return;

    lv_draw_label(draw_ctx, &label_draw_dsc, &txt_coords, text, NULL);
}
