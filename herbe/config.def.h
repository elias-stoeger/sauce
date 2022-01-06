static const char *background_color = "#2e3440";
static const char *border_color = "#d8dee9";
static const char *font_color = "#d8dee9";
static const char *font_pattern = "Noto Emoji Regular:size=12";
static const unsigned line_spacing = 5;
static const unsigned int padding = 15;

static const unsigned int width = 450;
static const unsigned int border_size = 3;
static const unsigned int pos_x = 30;
static const unsigned int pos_y = 30;

enum corners { TOP_LEFT, TOP_RIGHT, BOTTOM_LEFT, BOTTOM_RIGHT };
enum corners corner = BOTTOM_RIGHT;

static const unsigned int duration = 5; /* in seconds */

#define DISMISS_BUTTON Button1
#define ACTION_BUTTON Button3
