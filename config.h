#include <X11/XF86keysym.h> // For Multimedia control keys

#define COLOR(hex)    { ((hex >> 24) & 0xFF) / 255.0f, \
                        ((hex >> 16) & 0xFF) / 255.0f, \
                        ((hex >> 8) & 0xFF) / 255.0f, \
                        (hex & 0xFF) / 255.0f }
/* appearance */
static const int sloppyfocus        = 1;  /* focus follows mouse */
static const int bypass_surface_visibility = 0;  /* 1 means idle inhibitors will disable idle tracking even if it's surface isn't visible  */
static const int smartgaps                 = 0;  /* 1 means no outer gap when there is only one window */
static const int monoclegaps               = 0;  /* 1 means outer gaps in monocle layout */
static const unsigned int borderpx  = 2;  /* border pixel of windows */
static const unsigned int gappih           = 6; /* horiz inner gap between windows */
static const unsigned int gappiv           = 6; /* vert inner gap between windows */
static const unsigned int gappoh           = 6; /* horiz outer gap between windows and screen edge */
static const unsigned int gappov           = 6; /* vert outer gap between windows and screen edge */
static const float rootcolor[]    = COLOR(0x000000ff);
static const float bordercolor[]    = COLOR(0x727169ff);
static const float focuscolor[]     = COLOR(0x6a9589ff);
static const float urgentcolor[]      = {1.0, 0.3, 0.3, 1.0};
/* To conform the xdg-protocol, set the alpha to zero to restore the old behavior */
static const float fullscreen_bg[]  = {0.157, 0.157, 0.157, 1.0};

/* Keyboard layout tracking */
static const char *kbd_status_path = "/home/arnor/.cache/.dwl_kbd_layout";
static const char *kbd_status_callback[] = { "/bin/sh", "-c", "kill -39 $(pidof status-text)", NULL };

/* Autostart */
static const char *const autostart[] = {
        "sh", "-c", "/home/arnor/.dwl/autostart.sh", NULL,
        NULL /* terminate */
};

/* tagging */
static const char *tags[] = { "", "", "", "", "", "", "󰙯", "", "" };

/* pointer constraints */
static const int allow_constrain      = 1;

/* tagging - TAGCOUNT must be no greater than 31 */
#define TAGCOUNT (9)

/* logging */
static int log_level = WLR_ERROR;

static const Rule rules[] = {
    /* app_id     title       tags mask     isfloating   monitor */
    /* examples:
    { "Gimp",     NULL,       0,            1,           -1 },
    */
    { "Gimp",     NULL,       0,                        1,           -1 },
    { "firefox",  NULL,       1 << 8,                   0,           -1 },
    { "brave-browser", NULL,       1 << 8,                   0,           -1 },
    { "Code",     NULL,       1 << 1,                   0,           -1 },
    { "discord",  NULL,       1 << 6,                   0,            1 },
};

/* layout(s) */
static const Layout layouts[] = {
    /* symbol     arrange function */
    { "[]=",      tile },
    { "><>",      NULL },    /* no layout function means floating behavior */
    { "[M]",      monocle },
};

/* monitors */
static const MonitorRule monrules[] = {
    /* name       mfact nmaster scale layout       rotate/reflect x y tagset*/
    /* example of a HiDPI laptop monitor:
    { "eDP-1",    0.5,  1,      2,    &layouts[0], WL_OUTPUT_TRANSFORM_NORMAL },
    */
    /* defaults */
    { "DP-2",     0.55, 1,      1.25,   &layouts[0], WL_OUTPUT_TRANSFORM_NORMAL, 0, 0, 1u },
    { "HDMI-A-1", 0.55, 1,      1.25,   &layouts[0], WL_OUTPUT_TRANSFORM_NORMAL, 2048, 0, 1u << 6 },
    { NULL,       0.55, 1,       1.5,   &layouts[0], WL_OUTPUT_TRANSFORM_NORMAL, 0, 0, 1u },
};

/* keyboard */
static const struct xkb_rule_names xkb_rules = {
    /* can specify fields: rules, model, layout, variant, options */
    /* example:
    .options = "ctrl:nocaps",
    */
    .layout = "us,ru,fi",
    .options = "grp:win_space_toggle",
};

static const int repeat_rate = 25;
static const int repeat_delay = 600;

/* Trackpad */
static const int tap_to_click = 1;
static const int tap_and_drag = 1;
static const int drag_lock = 1;
static const int natural_scrolling = 0;
static const int disable_while_typing = 1;
static const int left_handed = 0;
static const int middle_button_emulation = 0;
/* You can choose between:
LIBINPUT_CONFIG_SCROLL_NO_SCROLL
LIBINPUT_CONFIG_SCROLL_2FG
LIBINPUT_CONFIG_SCROLL_EDGE
LIBINPUT_CONFIG_SCROLL_ON_BUTTON_DOWN
*/
static const enum libinput_config_scroll_method scroll_method = LIBINPUT_CONFIG_SCROLL_2FG;

/* You can choose between:
LIBINPUT_CONFIG_CLICK_METHOD_NONE       
LIBINPUT_CONFIG_CLICK_METHOD_BUTTON_AREAS       
LIBINPUT_CONFIG_CLICK_METHOD_CLICKFINGER 
*/
static const enum libinput_config_click_method click_method = LIBINPUT_CONFIG_CLICK_METHOD_BUTTON_AREAS;

/* You can choose between:
LIBINPUT_CONFIG_SEND_EVENTS_ENABLED
LIBINPUT_CONFIG_SEND_EVENTS_DISABLED
LIBINPUT_CONFIG_SEND_EVENTS_DISABLED_ON_EXTERNAL_MOUSE
*/
static const uint32_t send_events_mode = LIBINPUT_CONFIG_SEND_EVENTS_ENABLED;

/* You can choose between:
LIBINPUT_CONFIG_ACCEL_PROFILE_FLAT
LIBINPUT_CONFIG_ACCEL_PROFILE_ADAPTIVE
*/
static const enum libinput_config_accel_profile accel_profile = LIBINPUT_CONFIG_ACCEL_PROFILE_FLAT;
static const double accel_speed = -0.5;
/* You can choose between:
LIBINPUT_CONFIG_TAP_MAP_LRM -- 1/2/3 finger tap maps to left/right/middle
LIBINPUT_CONFIG_TAP_MAP_LMR -- 1/2/3 finger tap maps to left/middle/right
*/
static const enum libinput_config_tap_button_map button_map = LIBINPUT_CONFIG_TAP_MAP_LRM;

/* If you want to use the windows key for MODKEY, use WLR_MODIFIER_LOGO */
#define MODKEY WLR_MODIFIER_ALT

#define TAGKEYS(KEY,SKEY,TAG) \
    { MODKEY,                    KEY,            view,            {.ui = 1 << TAG} }, \
    { MODKEY|WLR_MODIFIER_CTRL,  KEY,            toggleview,      {.ui = 1 << TAG} }, \
    { MODKEY|WLR_MODIFIER_SHIFT, SKEY,           tag,             {.ui = 1 << TAG} }, \
    { MODKEY|WLR_MODIFIER_CTRL|WLR_MODIFIER_SHIFT,SKEY,toggletag, {.ui = 1 << TAG} }

/* helper for spawning shell commands in the pre dwm-5.0 fashion */
#define SHCMD(cmd) { .v = (const char*[]){ "/bin/sh", "-c", cmd, NULL } }

/* commands */
static const char *termcmd[] = { "footclient", NULL };
/* static const char *menucmd[] = { "dmenu_run", NULL }; */

static const Key keys[] = {
    /* Note that Shift changes certain key codes: c -> C, 2 -> at, etc. */
    /* modifier                  key                 function        argument */
	{ 0,                         XF86XK_AudioLowerVolume,    spawn,  SHCMD("pulsemixer --change-volume -5 && kill -37 $(pidof status-text)") },
	{ 0,                         XF86XK_AudioRaiseVolume,    spawn,  SHCMD("pulsemixer --change-volume +5 && kill -37 $(pidof status-text)") },
	{ 0,                         XF86XK_AudioMute,   spawn,          SHCMD("pulsemixer --toggle-mute && kill -37 $(pidof status-text)") },
    { MODKEY,                    XKB_KEY_p,          spawn,          SHCMD("tofi-run | xargs /bin/sh -c") },
    { MODKEY|WLR_MODIFIER_SHIFT, XKB_KEY_Return,     spawn,          {.v = termcmd} },
    { MODKEY,                    XKB_KEY_j,          focusstack,     {.i = +1} },
    { MODKEY,                    XKB_KEY_k,          focusstack,     {.i = -1} },
    { MODKEY,                    XKB_KEY_i,          incnmaster,     {.i = +1} },
    { MODKEY,                    XKB_KEY_d,          incnmaster,     {.i = -1} },
    { MODKEY,                    XKB_KEY_h,          setmfact,       {.f = -0.05} },
    { MODKEY,                    XKB_KEY_l,          setmfact,       {.f = +0.05} },
    { MODKEY|WLR_MODIFIER_LOGO,  XKB_KEY_h,          incgaps,       {.i = +1 } },
    { MODKEY|WLR_MODIFIER_LOGO,  XKB_KEY_l,          incgaps,       {.i = -1 } },
    { MODKEY|WLR_MODIFIER_LOGO|WLR_MODIFIER_SHIFT,   XKB_KEY_H,      incogaps,      {.i = +1 } },
    { MODKEY|WLR_MODIFIER_LOGO|WLR_MODIFIER_SHIFT,   XKB_KEY_L,      incogaps,      {.i = -1 } },
    { MODKEY|WLR_MODIFIER_LOGO|WLR_MODIFIER_CTRL,    XKB_KEY_h,      incigaps,      {.i = +1 } },
    { MODKEY|WLR_MODIFIER_LOGO|WLR_MODIFIER_CTRL,    XKB_KEY_l,      incigaps,      {.i = -1 } },
    { MODKEY|WLR_MODIFIER_LOGO,  XKB_KEY_0,          togglegaps,     {0} },
    { MODKEY|WLR_MODIFIER_LOGO|WLR_MODIFIER_SHIFT,   XKB_KEY_parenright,defaultgaps,    {0} },
    { MODKEY,                    XKB_KEY_y,          incihgaps,     {.i = +1 } },
    { MODKEY,                    XKB_KEY_o,          incihgaps,     {.i = -1 } },
    { MODKEY|WLR_MODIFIER_CTRL,  XKB_KEY_y,          incivgaps,     {.i = +1 } },
    { MODKEY|WLR_MODIFIER_CTRL,  XKB_KEY_o,          incivgaps,     {.i = -1 } },
    { MODKEY|WLR_MODIFIER_LOGO,  XKB_KEY_y,          incohgaps,     {.i = +1 } },
    { MODKEY|WLR_MODIFIER_LOGO,  XKB_KEY_o,          incohgaps,     {.i = -1 } },
    { MODKEY|WLR_MODIFIER_SHIFT, XKB_KEY_Y,          incovgaps,     {.i = +1 } },
    { MODKEY|WLR_MODIFIER_SHIFT, XKB_KEY_O,          incovgaps,     {.i = -1 } },
    { MODKEY,                    XKB_KEY_Return,     zoom,           {0} },
    { MODKEY,                    XKB_KEY_Tab,        view,           {0} },
    { MODKEY|WLR_MODIFIER_SHIFT, XKB_KEY_C,          killclient,     {0} },
    { MODKEY,                    XKB_KEY_t,          setlayout,      {.v = &layouts[0]} },
    { MODKEY,                    XKB_KEY_f,          setlayout,      {.v = &layouts[1]} },
    { MODKEY,                    XKB_KEY_m,          setlayout,      {.v = &layouts[2]} },
    { MODKEY,                    XKB_KEY_space,      setlayout,      {0} },
    { MODKEY|WLR_MODIFIER_SHIFT, XKB_KEY_space,      togglefloating, {0} },
    { MODKEY,                    XKB_KEY_e,         togglefullscreen, {0} },
    { MODKEY,                    XKB_KEY_0,          view,           {.ui = ~0} },
    { MODKEY|WLR_MODIFIER_SHIFT, XKB_KEY_parenright, tag,            {.ui = ~0} },
    { MODKEY,                    XKB_KEY_comma,      focusmon,       {.i = WLR_DIRECTION_LEFT} },
    { MODKEY,                    XKB_KEY_period,     focusmon,       {.i = WLR_DIRECTION_RIGHT} },
    { MODKEY|WLR_MODIFIER_SHIFT, XKB_KEY_less,       tagmon,         {.i = WLR_DIRECTION_LEFT} },
    { MODKEY|WLR_MODIFIER_SHIFT, XKB_KEY_greater,    tagmon,         {.i = WLR_DIRECTION_RIGHT} },
    TAGKEYS(          XKB_KEY_1, XKB_KEY_exclam,                     0),
    TAGKEYS(          XKB_KEY_2, XKB_KEY_at,                         1),
    TAGKEYS(          XKB_KEY_3, XKB_KEY_numbersign,                 2),
    TAGKEYS(          XKB_KEY_4, XKB_KEY_dollar,                     3),
    TAGKEYS(          XKB_KEY_5, XKB_KEY_percent,                    4),
    TAGKEYS(          XKB_KEY_6, XKB_KEY_asciicircum,                5),
    TAGKEYS(          XKB_KEY_7, XKB_KEY_ampersand,                  6),
    TAGKEYS(          XKB_KEY_8, XKB_KEY_asterisk,                   7),
    TAGKEYS(          XKB_KEY_9, XKB_KEY_parenleft,                  8),
    { MODKEY|WLR_MODIFIER_SHIFT, XKB_KEY_Q,          quit,           {0} },

    /* Ctrl-Alt-Backspace and Ctrl-Alt-Fx used to be handled by X server */
    { WLR_MODIFIER_CTRL|WLR_MODIFIER_ALT,XKB_KEY_Terminate_Server, quit, {0} },
#define CHVT(n) { WLR_MODIFIER_CTRL|WLR_MODIFIER_ALT,XKB_KEY_XF86Switch_VT_##n, chvt, {.ui = (n)} }
    CHVT(1), CHVT(2), CHVT(3), CHVT(4), CHVT(5), CHVT(6),
    CHVT(7), CHVT(8), CHVT(9), CHVT(10), CHVT(11), CHVT(12),
};

static const Button buttons[] = {
    { MODKEY, BTN_LEFT,   moveresize,     {.ui = CurMove} },
    { MODKEY, BTN_MIDDLE, togglefloating, {0} },
    { MODKEY, BTN_RIGHT,  moveresize,     {.ui = CurResize} },
};
