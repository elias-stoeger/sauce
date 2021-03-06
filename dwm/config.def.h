#include <X11/XF86keysym.h>
/* appearance */
static const unsigned int borderpx  = 3;        /* border pixel of windows */
//static const Gap default_gap        = {.isgap = 1, .realgap = 6, .gappx = 6};
static const unsigned int snap      = 10;       /* snap pixel, when to snap to the edge */
static const unsigned int gappih    = 45;       /* horiz inner gap between windows */
static const unsigned int gappiv    = 45;       /* vert inner gap between windows */
static const unsigned int gappoh    = 45;       /* horiz outer gap between windows and screen edge */
static const unsigned int gappov    = 45;       /* vert outer gap between windows and screen edge */
static       int smartgaps          = 0;        /* 1 means no outer gap when there is only one window */
static const int showbar            = 0;        /* 0 means no bar */
static const int topbar             = 1;        /* 0 means bottom bar */
static const int vertpad            = 0;       	/* vertical padding of bar */
static const int sidepad            = 45;       /* horizontal padding of bar */
static const char *fonts[]          = { "IBM Plex Mono:size=10" };
static const char dmenufont[]       = "IBM Plex Mono:size=10";
static const char col_gray1[]       = "#222222";
static const char col_gray2[]       = "#444444";
static const char col_gray3[]       = "#bbbbbb";
static const char col_gray4[]       = "#eeeeee";
static const char col_cyan[]        = "#005577";
static const char col_yellow2[]	    = "#feb236";

/* Nord colors */
static const char nord_orange[]	    = "#d08770";
static const char nord_yellow[]	    = "#ebcb8b";
static const char nord_gray[]	    = "#242933";
static const char nord_gray2[]	    = "#3b4252";
static const char nord_gray3[]	    = "#434c5e";
static const char nord_blue[]	    = "#81a1c1";
static const char nord_white[]	    = "#d8dee9";

static const char *colors[][3]      = {
	/*               	     fg         bg         border   */
	[SchemeNorm] 		= { col_gray3, col_gray1, nord_gray },
	[SchemeSel]  		= { col_gray4, col_cyan,  nord_white },
	// Statusbar right {text, background, not used but cannot be empty}
	[SchemeStatus]  	= { nord_white, nord_gray,  "#000000"  },
	// Tagbar left selected {text,background, not used but cannot be empty}
	[SchemeTagsSel]  	= { nord_white, nord_gray3,  "#000000"  },        
	// Tagbar left unselected {text,background, not used but cannot be empty}
	[SchemeTagsNorm]  	= { nord_white, nord_gray,  "#000000"  },
	// infobar middle  selected {text,background, not used but cannot be empty}
	[SchemeInfoSel]		= { nord_white, nord_gray2,  "#000000"  }, 	
	// infobar middle  unselected {text,background, not used but cannot be empty}
	[SchemeInfoNorm]  	= { nord_white, nord_gray,  "#000000"  }, 
};

/* tagging */
static const char *tags[] = { "1", "2", "3", "4", "5" };

static const Rule rules[] = {
	/* xprop(1):
	 *	WM_CLASS(STRING) = instance, class
	 *	WM_NAME(STRING) = title
	 */
	/* class      instance    title       tags mask     isfloating   monitor */
	//{ "Gimp",     NULL,       NULL,       0,            1,           -1 },
	//{ "Firefox",  NULL,       NULL,       1 << 8,       0,           -1 },
	{ "Blueman-manager", 	NULL,	NULL, 	1 << 5,		0,	-1 },
};

/* layout(s) */
static const float mfact     = 0.50; /* factor of master area size [0.05..0.95] */
static const int nmaster     = 1;    /* number of clients in master area */
static const int resizehints = 1;    /* 1 means respect size hints in tiled resizals */
static const int lockfullscreen = 1; /* 1 will force focus on the fullscreen window */

#define FORCE_VSPLIT 1  /* nrowgrid layout: force two clients to always split vertically */
#include "vanitygaps.c"

static const Layout layouts[] = {
	/* symbol     arrange function */
	{ "[]=",      tile },    /* first entry is default */
	{ "[@]",      spiral },
	{ "[M]",      monocle },
	{ "[\\]",     dwindle },
	{ "H[]",      deck },
	{ "TTT",      bstack },
	{ "===",      bstackhoriz },
	{ "HHH",      grid },
	{ "###",      nrowgrid },
	{ "---",      horizgrid },
	{ ":::",      gaplessgrid },
	{ "|M|",      centeredmaster },
	{ ">M>",      centeredfloatingmaster },
	{ "><>",      NULL },    /* no layout function means floating behavior */
	{ NULL,       NULL },
};

/* key definitions */
#define MODKEY Mod4Mask
#define TAGKEYS(KEY,TAG) \
	{ MODKEY,                       KEY,      view,           {.ui = 1 << TAG} }, \
	{ MODKEY|ControlMask,           KEY,      toggleview,     {.ui = 1 << TAG} }, \
	{ MODKEY|ShiftMask,             KEY,      tag,            {.ui = 1 << TAG} }, \
	{ MODKEY|ControlMask|ShiftMask, KEY,      toggletag,      {.ui = 1 << TAG} },

/* helper for spawning shell commands in the pre dwm-5.0 fashion */
#define SHCMD(cmd) { .v = (const char*[]){ "/bin/sh", "-c", cmd, NULL } }

/* commands */
static char dmenumon[2] = "0"; /* component of dmenucmd, manipulated in spawn() */
static const char *dmenucmd[] = { "dmenu_run", "-m", dmenumon, "-fn", dmenufont, 
				  "-hp", "qutebrowser,kdeconnect-app,surf,pavucontrol,blueman-manager,netflix,teams,element,freetube,signal", NULL };
static const char *passmenucmd[] = { "/home/krixec/.local/bin/passmenu2", NULL };
static const char *termcmd[]  = { "st", NULL };
static const char *browsercmd[] = { "qutebrowser", NULL };
static const char *telegram[] = { "telegram-desktop", NULL };
/* since xbacklight was being moody I wrote my own, more relyable script */
static const char *brightness[2][5] = { { "python3", "/home/krixec/Scripts/brightness.py", "inc", "10", NULL },
					{ "python3", "/home/krixec/Scripts/brightness.py", "dec", "10", NULL } };
//static const char *brightness[2][4] = { { "xbacklight", "-inc", "10", NULL },
//					{ "xbacklight", "-dec", "10", NULL } };
static const char *volume[3][5] = { { "pactl", "set-sink-volume", "@DEFAULT_SINK@", "+10%", NULL },
				    { "pactl", "set-sink-volume", "@DEFAULT_SINK@", "-10%", NULL },
				    { "pactl", "set-sink-mute", "@DEFAULT_SINK@", "toggle", NULL } };
static const char *mute[] = { "amixer", "set", "Capture", "toggle", NULL };
static const char *blank[] = { "xset", "dpms", "force", "off", NULL };
static const char *lock[] = { "xautolock", "-locknow", NULL };
static const char *bluetooth[] = { "bluetoothctl", "power", "on", NULL };
static const char *bluetooth_man[] = { "blueman-manager", NULL };
static const char *bluetooth_herb[] = { "herbe", "Bluetooth eingeschalten", NULL };
static const char *bluetooth_off[] = { "bluetoothctl", "power", "off", NULL };
static const char *bluetooth_off_herb[] = { "herbe", "Bluetooth ausgeschalten", NULL };
static const char *kde_connect[] = { "kdeconnect-cli", "--pair", "-d", "91afe99f5186ager", NULL };
static const char *kde_connect_herb[] = { "herbe", "Suche Verbindung mit Handy ...", NULL };
static const char *print[] = { "scrot", "-s", "Desktop/screenshot.png", NULL };
static const char *hdmi_mirror[] = { "xrandr", "--output", "HDMI1", "--mode", "1920x1080", "--same-as", "eDP1", NULL };
static const char *hdmi_left[] = { "xrandr", "--output", "HDMI1", "--mode", "1920x1080", "--left-of", "eDP1", NULL };
static const char *hdmi_mirror_herb[] = { "herbe", "Bildschirm gespiegelt", NULL };
static const char *hdmi_left_herb[] = { "herbe", "Bildschirm links erweitert", NULL };
static const char *hdmi_off[] = { "xrandr", "--output", "HDMI1", "--off", NULL };

#include "movestack.c"
static Key keys[] = {
	/* modifier                     key        function        argument */
	{ MODKEY,                       XK_d,      spawn,          {.v = dmenucmd } },
	{ MODKEY|ShiftMask,		XK_d,	   spawn,	   {.v = passmenucmd } },
	{ MODKEY|ShiftMask,             XK_Return, spawn,          {.v = termcmd } },
	{ MODKEY,                       XK_t,      spawn,          {.v = telegram } },
	{ MODKEY,                       XK_b,      togglebar,      {0} },
	{ MODKEY,			XK_q,      spawn,	   {.v = browsercmd } },
	{ MODKEY,                       XK_j,      focusstack,     {.i = +1 } },
	{ MODKEY,                       XK_k,      focusstack,     {.i = -1 } },
	{ MODKEY,                       XK_i,      incnmaster,     {.i = +1 } },
	{ MODKEY,                       XK_p,      incnmaster,     {.i = -1 } },
	{ MODKEY,                       XK_h,      setmfact,       {.f = -0.05} },
	{ MODKEY,                       XK_l,      setmfact,       {.f = +0.05} },
	{ MODKEY|ShiftMask,             XK_h,      setcfact,       {.f = +0.25} },
	{ MODKEY|ShiftMask,             XK_l,      setcfact,       {.f = -0.25} },
	{ MODKEY|ShiftMask,             XK_o,      setcfact,       {.f =  0.00} },
	{ MODKEY|ShiftMask,		XK_j,	   movestack,	   {.i = +1 } },
	{ MODKEY|ShiftMask,		XK_k,	   movestack,	   {.i = -1 } },
	{ MODKEY,                       XK_Return, zoom,           {0} },
	{ MODKEY|Mod4Mask,              XK_u,      incrgaps,       {.i = +1 } },
	{ MODKEY|Mod4Mask|ShiftMask,    XK_u,      incrgaps,       {.i = -1 } },
	{ MODKEY|Mod4Mask,              XK_i,      incrigaps,      {.i = +1 } },
	{ MODKEY|Mod4Mask|ShiftMask,    XK_i,      incrigaps,      {.i = -1 } },
	{ MODKEY|Mod4Mask,              XK_o,      incrogaps,      {.i = +1 } },
	{ MODKEY|Mod4Mask|ShiftMask,    XK_o,      incrogaps,      {.i = -1 } },
	{ MODKEY|Mod4Mask,              XK_6,      incrihgaps,     {.i = +1 } },
	{ MODKEY|Mod4Mask|ShiftMask,    XK_6,      incrihgaps,     {.i = -1 } },
	{ MODKEY|Mod4Mask,              XK_7,      incrivgaps,     {.i = +1 } },
	{ MODKEY|Mod4Mask|ShiftMask,    XK_7,      incrivgaps,     {.i = -1 } },
	{ MODKEY|Mod4Mask,              XK_8,      incrohgaps,     {.i = +1 } },
	{ MODKEY|Mod4Mask|ShiftMask,    XK_8,      incrohgaps,     {.i = -1 } },
	{ MODKEY|Mod4Mask,              XK_9,      incrovgaps,     {.i = +1 } },
	{ MODKEY|Mod4Mask|ShiftMask,    XK_9,      incrovgaps,     {.i = -1 } },
	{ MODKEY|Mod4Mask,              XK_0,      togglegaps,     {0} },
	{ MODKEY|Mod4Mask|ShiftMask,    XK_0,      defaultgaps,    {0} },
	{ MODKEY,                       XK_Tab,    view,           {0} },
	{ MODKEY|ShiftMask,             XK_q,      killclient,     {0} },
	{ MODKEY,                       XK_t,      setlayout,      {.v = &layouts[0]} },
	{ MODKEY|ShiftMask,             XK_f,      setlayout,      {.v = &layouts[1]} },
	{ MODKEY,                       XK_m,      setlayout,      {.v = &layouts[2]} },
	{ MODKEY,                       XK_space,  setlayout,      {0} },
	{ MODKEY|ShiftMask,             XK_space,  togglefloating, {0} },
	{ MODKEY,	                XK_f,      togglefullscr,  {0} },
	{ MODKEY,                       XK_0,      view,           {.ui = ~0 } },
	{ MODKEY|ShiftMask,             XK_0,      tag,            {.ui = ~0 } },
	{ MODKEY,                       XK_comma,  focusmon,       {.i = -1 } },
	{ MODKEY,                       XK_period, focusmon,       {.i = +1 } },
	{ MODKEY|ShiftMask,             XK_comma,  tagmon,         {.i = -1 } },
	{ MODKEY|ShiftMask,             XK_period, tagmon,         {.i = +1 } },
	TAGKEYS(                        XK_1,                      0)
	TAGKEYS(                        XK_2,                      1)
	TAGKEYS(                        XK_3,                      2)
	TAGKEYS(                        XK_4,                      3)
	TAGKEYS(                        XK_5,                      4)
	TAGKEYS(                        XK_8,                      7)
	TAGKEYS(                        XK_9,                      8)
	{ MODKEY|ShiftMask,             XK_e,      quit,           {0} },
	/* FN Keys */
	{0, 		XF86XK_MonBrightnessUp,	   spawn,	   {.v=brightness[0]} },
	{0, 		XF86XK_MonBrightnessDown,  spawn,	   {.v=brightness[1]} },
	{0, 		XF86XK_AudioRaiseVolume,   spawn,	   {.v=volume[0]} },
	{0, 		XF86XK_AudioLowerVolume,   spawn,	   {.v=volume[1]} },
	{0, 		XF86XK_AudioMute,	   spawn,	   {.v=volume[2]} },
	{0, 		XF86XK_AudioMicMute,	   spawn,	   {.v=mute} },
			/* F9 */
	{0, 		XF86XK_Tools,		   spawn,	   {.v=blank} },
	{0, 		XF86XK_Tools,		   spawn,	   {.v=lock} },
	{0, 		XF86XK_Bluetooth,	   spawn,	   {.v=bluetooth} },
	{0, 		XF86XK_Bluetooth,	   spawn,	   {.v=bluetooth_man} },
	{0, 		XF86XK_Bluetooth,	   spawn,	   {.v=bluetooth_herb} },
	{ShiftMask, 	XF86XK_Bluetooth,	   spawn,	   {.v=bluetooth_off} },
	{ShiftMask, 	XF86XK_Bluetooth,	   spawn,	   {.v=bluetooth_off_herb} },
			/* F8 */
	{0, 		XF86XK_WLAN, 		   spawn,	   {.v=kde_connect} },
	{0, 		XF86XK_WLAN,	 	   spawn,	   {.v=kde_connect_herb} },
	{0, 		XK_Print,	 	   spawn,	   {.v=print} },
			/* F7 */
	{0, 		XF86XK_Display, 	   spawn,	   {.v=hdmi_mirror} },
	{0, 		XF86XK_Display, 	   spawn,	   {.v=hdmi_mirror_herb} },
	{ShiftMask, 	XF86XK_Display, 	   spawn,	   {.v=hdmi_left} },
	{ShiftMask,	XF86XK_Display, 	   spawn,	   {.v=hdmi_left_herb} },
	{ MODKEY|ShiftMask, XF86XK_Display, 	   spawn,	   {.v=hdmi_off} },
};

/* button definitions */
/* click can be ClkTagBar, ClkLtSymbol, ClkStatusText, ClkWinTitle, ClkClientWin, or ClkRootWin */
static Button buttons[] = {
	/* click                event mask      button          function        argument */
	{ ClkLtSymbol,          0,              Button1,        setlayout,      {0} },
	{ ClkLtSymbol,          0,              Button3,        setlayout,      {.v = &layouts[2]} },
	{ ClkWinTitle,          0,              Button2,        zoom,           {0} },
	{ ClkStatusText,        0,              Button2,        spawn,          {.v = termcmd } },
	{ ClkClientWin,         MODKEY,         Button1,        movemouse,      {0} },
	{ ClkClientWin,         MODKEY,         Button2,        togglefloating, {0} },
	{ ClkClientWin,         MODKEY,         Button3,        resizemouse,    {0} },
	{ ClkTagBar,            0,              Button1,        view,           {0} },
	{ ClkTagBar,            0,              Button3,        toggleview,     {0} },
	{ ClkTagBar,            MODKEY,         Button1,        tag,            {0} },
	{ ClkTagBar,            MODKEY,         Button3,        toggletag,      {0} },
};

