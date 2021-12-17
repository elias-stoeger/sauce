// This file is part of Snownews - A lightweight console RSS newsreader
//
// Copyright (c) 2003-2004 Oliver Feiler <kiza@kcore.de>
// Copyright (c) 2021 Mike Sharov <msharov@users.sourceforge.net>
//
// Snownews is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 3
// as published by the Free Software Foundation.
//
// Snownews is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Snownews. If not, see http://www.gnu.org/licenses/.

#include "dialog.h"
#include "main.h"
#include "cat.h"
#include "conv.h"
#include "filters.h"
#include "feedio.h"
#include "uiutil.h"
#include "parse.h"
#include "setup.h"
#include <ncurses.h>

char* UIOneLineEntryField (int x, int y)
{
    // UIStatus switches off attron!
    attron (WA_REVERSE);
    echo();
    curs_set (1);

    move (y, x);
    // Beware of hardcoded textlength size!
    // getnstr size does NOT include \0. This is really stupid and causes
    // 1-byte overflows. Always size=len-1!
    char text [512] = {0};
    getnstr (text, sizeof(text)-1);

    noecho();
    if (!_settings.cursor_always_visible)
	curs_set (0);
    attroff (WA_REVERSE);

    // This memory needs to be freed in the calling function!
    return strdup (text);
}

void UIChangeBrowser (void)
{
    // malloc = 17 (strlen("Current setting: ") + browser
    // We will malloc a bigger junk, because other languages
    // might need longer strings and crash!
    size_t cbrsl = strlen (_("Current setting: %s")) + strlen (_settings.browser) + 1;
    char curbrowserstr[cbrsl];
    snprintf (curbrowserstr, cbrsl, _("Current setting: %s"), _settings.browser);

  repeat:
    // Clear screen area we want to "draw" to.
    attron (WA_REVERSE);
    UISupportDrawBox (3, 5, COLS - 4, 7);
    UIStatus (curbrowserstr, 0, 0);

    char* browserstring = UIOneLineEntryField (5, 6);

    if (strlen (browserstring) == 0) {
	free (browserstring);
	UIStatus (_("Aborted."), 1, 0);
	return;
    }

    const char* fmtstr = strstr (browserstring, "%");
    if (fmtstr && fmtstr[1] != 's') {
	free (browserstring);
	UIStatus (_("Only %s format strings allowed for default browser!"), 3, 1);
	goto repeat;
    }

    if (strstr (browserstring, "'") != NULL)
	UIStatus (_("Unsafe browser string (contains quotes)! See snownews.kcore.de/faq#toc2.4"), 3, 1);

    free (_settings.browser);
    _settings.browser = browserstring;
}

// Dialog to change feedname.
// Return: 0    on success
//         1    on user abort
//         2    original title restored
void UIChangeFeedName (struct feed* cur_ptr)
{
    // Clear screen area we want to "draw" to.
    attron (WA_REVERSE);
    UISupportDrawBox (3, 5, COLS - 4, 7);
    UIStatus (_("Enter new name. Blank line to abort. '-' to reset."), 0, 0);

    char* newname = UIOneLineEntryField (5, 6);

    // If strlen is zero, return 1.
    if (strlen (newname) == 0) {
	free (newname);
	return;
    }
    // If newname contains "|", abort since this is used as a delimiter for the config file.
    if (strstr (newname, "|") != NULL) {
	free (newname);
	UIStatus (_("The new title must not contain a \"|\" character!"), 2, 0);
	return;
    }
    // Restore original title.
    if (newname && cur_ptr->custom_title) {
	if (strcmp (newname, "-") == 0) {
	    if (cur_ptr->title != NULL)
		free (cur_ptr->title);
	    cur_ptr->title = cur_ptr->original;
	    // Set back original to NULL pointer.
	    cur_ptr->original = NULL;
	    free (cur_ptr->custom_title);
	    cur_ptr->custom_title = NULL;
	    free (newname);
	    return;
	}
    }
    // Copy new name into ->custom_title.
    free (cur_ptr->custom_title);
    cur_ptr->custom_title = strdup (newname);

    // Save original.
    free (cur_ptr->original);
    cur_ptr->original = cur_ptr->title;

    // Set new title.
    cur_ptr->title = newname;
    return;
}

// Popup window to add new RSS feed. Passing an URL will
// automatically add it, no questions asked.
int UIAddFeed (char* newurl)
{
    char* url;
    if (newurl == NULL) {
	// Clear screen area we want to "draw" to.
	attron (WA_REVERSE);
	UISupportDrawBox (3, 5, COLS - 4, 7);

	UIStatus (_("Enter URL of the feed you want to add. Blank line to abort."), 0, 0);

	url = UIOneLineEntryField (5, 6);

	// If read stringlength is ZARO (abort of action requested) return 1
	// and confuse the calling function.
	if (strlen (url) == 0) {
	    free (url);
	    return 1;
	}

	CleanupString (url, false);

	// Support that stupid feed:// "protocol"
	if (strncasecmp (url, "feed://", 7) == 0)
	    memcpy (url, "http", 4);

	// If URL does not start with the procotol specification, assume http://
	if (strncasecmp (url, "http://", 7) != 0 && strncasecmp (url, "https://", 8) != 0 && strncasecmp (url, "exec:", 5) != 0) {
	    char* httpurl = malloc (strlen ("http://") + strlen (url) + 1);
	    sprintf (httpurl, "http://%s", url);
	    free (url);
	    url = httpurl;
	}
    } else
	url = strdup (newurl);

    struct feed* new_ptr = newFeedStruct();

    // getnstr does not return newline... says the docs.
    new_ptr->feedurl = malloc (strlen (url) + 1);
    strncpy (new_ptr->feedurl, url, strlen (url) + 1);

    // Attach to feed pointer chain.
    AddFeedToList (new_ptr);

    // Tag execurl.
    if (strncasecmp (url, "exec:", 5) == 0)
	new_ptr->execurl = 1;

    if (strncasecmp (url, "smartfeed:", 10) == 0)
	new_ptr->smartfeed = 1;

    // Don't need url text anymore.
    free (url);

    // Download new feed and DeXMLize it. */
    if (UpdateFeed (new_ptr) != 0) {
	UIStatus (_("The feed could not be parsed. Do you need a filter script for this feed? (y/n)"), 0, 0);
	if (getch() == 'n')
	    return -1;
	else {
	    UIPerFeedFilter (new_ptr);
	    FilterPipeNG (new_ptr);
	    if (DeXML (new_ptr) != 0)
		return -1;
	    else
		new_ptr->problem = false;
	}
    }
    new_ptr->mtime = time (NULL);
    return 0;
}

void FeedInfo (const struct feed* current_feed)
{
    char* url = strdup (current_feed->feedurl);	// feedurl - authinfo.

    // Remove authinfo from URL.
    const char* pauthinfo = strstr (url, "@");
    if (pauthinfo)
	memmove (strstr (url, "://") + 3, pauthinfo + 1, strlen (pauthinfo));

    unsigned boxw = COLS - 6, centerx = COLS / 2u, leftx = centerx - boxw/2 + 4;

    UISupportDrawBox (5, 4, boxw, 13);

    attron (WA_REVERSE);
    mvaddn_utf8 (5, centerx - utf8_length(current_feed->title), current_feed->title, boxw-8);
    mvaddn_utf8 (7, leftx, url, boxw-8);
    move (8, leftx);
    if (current_feed->lastmodified) {
	char timebuf [32] = "";
	ctime_r (&current_feed->lastmodified, timebuf);
	for (unsigned i = 0; i < sizeof(timebuf); ++i)
	    if (timebuf[i] == '\n')
		timebuf[i] = 0;
	timebuf[sizeof(timebuf)-1] = 0;
	add_utf8 (_("Last updated: "));
	addstr (timebuf);
    } else
	add_utf8 (_("No modification date."));
    free (url);
    url = NULL;

    char* hashme = Hashify (current_feed->feedurl);
    char cachefile [PATH_MAX];
    CacheFilePath (hashme, cachefile, sizeof(cachefile));
    free (hashme);

    struct stat cachestat;
    move (9, leftx);
    if (stat (cachefile, &cachestat) < 0)
	add_utf8 (_("Not in disk cache."));
    else {
	char cacheszbuf [64];
	snprintf (cacheszbuf, sizeof(cacheszbuf), _("In disk cache: %jd bytes"), cachestat.st_size);
	add_utf8 (cacheszbuf);
    }

    // Print category info
    mvadd_utf8 (10, leftx, _("Categories:"));
    addstr (" ");
    if (current_feed->feedcategories == NULL)
	addstr (_("none"));
    else {
	char* categories = GetCategoryList (current_feed);
	printw (categories);
	free (categories);
    }

    // Tell user if feed uses auth, but don't display the string.
    move (11, leftx);
    if (current_feed->lasterror) {
	add_utf8 (_("Download failed: "));
	add_utf8 (current_feed->lasterror);
    } else if (pauthinfo)
	add_utf8 (_("Feed uses authentication."));
    else
	add_utf8 (_("Feed does not use authentication."));

    // Display filter script if any.
    if (current_feed->perfeedfilter != NULL) {
	UISupportDrawBox (5, 13, boxw, 14);
	attron (WA_REVERSE);
	const char* filteredmsg = _("Filtered through:");
	mvadd_utf8 (13, leftx, filteredmsg);
	addstr (" ");
	addn_utf8 (current_feed->perfeedfilter, boxw-8-utf8_length(filteredmsg));
    }

    UIStatus (_("Displaying feed information."), 0, 0);

    // Wait for the any key.
    getch();
}

bool UIDeleteFeed (const char* feedname)
{
    UISupportDrawBox (3, 5, COLS - 3, 8);

    attron (WA_REVERSE);
    mvadd_utf8 (6, COLS / 2 - 21, _("Are you sure you want to delete this feed?"));
    mvprintw (7, 5, "%s", feedname);

    UIStatus (_("Type 'y' to delete, any other key to abort."), 0, 0);

    return getch() == 'y';
}

void UIHelpScreen (void)
{
    unsigned centerx = COLS / 2u, centery = LINES / 2u;

    UISupportDrawBox (centerx - 20, centery - 10, centerx + 24, centery + 9);

    attron (WA_REVERSE);
    // Keys
    const int offset = 18, offsetstr = 12;
    mvprintw (centery - 9, centerx - offset, "%c:", _settings.keybindings.addfeed);
    mvprintw (centery - 8, centerx - offset, "%c:", _settings.keybindings.deletefeed);
    mvprintw (centery - 7, centerx - offset, "%c:", _settings.keybindings.changefeedname);
    mvprintw (centery - 6, centerx - offset, "%c:", _settings.keybindings.reloadall);
    mvprintw (centery - 5, centerx - offset, "%c:", _settings.keybindings.reload);
    mvprintw (centery - 4, centerx - offset, "%c:", _settings.keybindings.markallread);
    mvprintw (centery - 3, centerx - offset, "%c:", _settings.keybindings.dfltbrowser);
    mvprintw (centery - 2, centerx - offset, "%c, %c:", _settings.keybindings.moveup, _settings.keybindings.movedown);
    mvprintw (centery - 1, centerx - offset, "%c:", _settings.keybindings.sortfeeds);
    mvprintw (centery, centerx - offset, "%c:", _settings.keybindings.categorize);
    mvprintw (centery + 1, centerx - offset, "%c:", _settings.keybindings.filter);
    mvprintw (centery + 2, centerx - offset, "%c:", _settings.keybindings.filtercurrent);
    mvprintw (centery + 3, centerx - offset, "%c:", _settings.keybindings.nofilter);
    mvprintw (centery + 4, centerx - offset, "%c:", _settings.keybindings.newheadlines);
    mvprintw (centery + 5, centerx - offset, "%c:", _settings.keybindings.perfeedfilter);
    mvadd_utf8 (centery + 6, centerx - offset, _("tab:"));
    mvprintw (centery + 7, centerx - offset, "%c:", _settings.keybindings.about);
    mvprintw (centery + 8, centerx - offset, "%c:", _settings.keybindings.quit);
    // Descriptions
    mvadd_utf8 (centery - 9, centerx - offsetstr, _("Add RSS feed..."));
    mvadd_utf8 (centery - 8, centerx - offsetstr, _("Delete highlighted RSS feed..."));
    mvadd_utf8 (centery - 7, centerx - offsetstr, _("Rename feed..."));
    mvadd_utf8 (centery - 6, centerx - offsetstr, _("Reload all feeds"));
    mvadd_utf8 (centery - 5, centerx - offsetstr, _("Reload this feed"));
    mvadd_utf8 (centery - 4, centerx - offsetstr, _("Mark all read"));
    mvadd_utf8 (centery - 3, centerx - offsetstr, _("Change default browser..."));
    mvadd_utf8 (centery - 2, centerx - offsetstr, _("Move item up, down"));
    mvadd_utf8 (centery - 1, centerx - offsetstr, _("Sort feed list alphabetically"));
    mvadd_utf8 (centery, centerx - offsetstr, _("Categorize feed..."));
    mvadd_utf8 (centery + 1, centerx - offsetstr, _("Apply filter..."));
    mvadd_utf8 (centery + 2, centerx - offsetstr, _("Only current category"));
    mvadd_utf8 (centery + 3, centerx - offsetstr, _("Remove filter"));
    mvadd_utf8 (centery + 4, centerx - offsetstr, _("Show new headlines"));
    mvadd_utf8 (centery + 5, centerx - offsetstr, _("Add conversion filter..."));
    mvadd_utf8 (centery + 6, centerx - offsetstr, _("Type Ahead Find"));
    mvadd_utf8 (centery + 7, centerx - offsetstr, _("About"));
    mvadd_utf8 (centery + 8, centerx - offsetstr, _("Quit program"));
    attroff (WA_REVERSE);

    UIStatus (_("Press the any(tm) key to exit help screen."), 0, 0);
    getch();
}

void UIDisplayFeedHelp (void)
{
    unsigned centerx = COLS / 2u, centery = LINES / 2u;
    UISupportDrawBox (centerx - 20, centery - 6, centerx + 24, centery + 7);

    attron (WA_REVERSE);
    // Keys
    const int offset = 18, offsetstr = 7;
    mvprintw (centery - 5, centerx - offset, "%c", _settings.keybindings.prev);
    add_utf8 (_(", up:"));
    mvprintw (centery - 4, centerx - offset, "%c", _settings.keybindings.next);
    add_utf8 (_(", down:"));
    mvadd_utf8 (centery - 3, centerx - offset, _("enter:"));
    mvprintw (centery - 2, centerx - offset, "%c:", _settings.keybindings.reload);
    mvprintw (centery - 1, centerx - offset, "%c:", _settings.keybindings.forcereload);
    mvprintw (centery, centerx - offset, "%c:", _settings.keybindings.urljump);
    mvprintw (centery + 1, centerx - offset, "%c:", _settings.keybindings.urljump2);
    mvprintw (centery + 2, centerx - offset, "%c:", _settings.keybindings.markread);
    mvprintw (centery + 3, centerx - offset, "%c:", _settings.keybindings.markunread);
    mvprintw (centery + 4, centerx - offset, "%c:", _settings.keybindings.feedinfo);
    mvadd_utf8 (centery + 5, centerx - offset, _("tab:"));
    mvprintw (centery + 6, centerx - offset, "%c:", _settings.keybindings.prevmenu);
    // Descriptions
    mvadd_utf8 (centery - 5, centerx - offsetstr, _("Previous item"));
    mvadd_utf8 (centery - 4, centerx - offsetstr, _("Next item"));
    mvadd_utf8 (centery - 3, centerx - offsetstr, _("View item"));
    mvadd_utf8 (centery - 2, centerx - offsetstr, _("Reload this feed"));
    mvadd_utf8 (centery - 1, centerx - offsetstr, _("Force reload this feed"));
    mvadd_utf8 (centery, centerx - offsetstr, _("Open homepage"));
    mvadd_utf8 (centery + 1, centerx - offsetstr, _("Open link"));
    mvadd_utf8 (centery + 2, centerx - offsetstr, _("Mark all read"));
    mvadd_utf8 (centery + 3, centerx - offsetstr, _("Toggle item read status"));
    mvadd_utf8 (centery + 4, centerx - offsetstr, _("Show feed info..."));
    mvadd_utf8 (centery + 5, centerx - offsetstr, _("Type Ahead Find"));
    mvadd_utf8 (centery + 6, centerx - offsetstr, _("Return to main menu"));
    attroff (WA_REVERSE);

    UIStatus (_("Press the any(tm) key to exit help screen."), 0, 0);
    getch();
}

void UIDisplayItemHelp (void)
{
    unsigned centerx = COLS / 2u, centery = LINES / 2u;
    UISupportDrawBox (centerx - 18, centery - 2, centerx + 18, centery + 3);

    attron (WA_REVERSE);
    // Keys
    const int offset = 16, offsetstr = 6;
    mvprintw (centery - 1, centerx - offset, "%c, <-:", _settings.keybindings.prev);
    mvprintw (centery, centerx - offset, "%c, ->:", _settings.keybindings.next);
    mvprintw (centery + 1, centerx - offset, "%c:", _settings.keybindings.urljump);
    mvprintw (centery + 2, centerx - offset, "%c", _settings.keybindings.prevmenu);
    add_utf8 (_(", enter:"));
    // Descriptions
    mvadd_utf8 (centery - 1, centerx - offsetstr, _("Previous item"));
    mvadd_utf8 (centery, centerx - offsetstr, _("Next item"));
    mvadd_utf8 (centery + 1, centerx - offsetstr, _("Open link"));
    mvadd_utf8 (centery + 2, centerx - offsetstr, _("Return to overview"));
    attroff (WA_REVERSE);

    UIStatus (_("Press the any(tm) key to exit help screen."), 0, 0);
    getch();
}

// Add/remove categories for given feed. This takes over the main interface while running.
void CategorizeFeed (struct feed* current_feed)
{
    // Return if we got passed a NULL pointer (no feeds added to main program).
    if (current_feed == NULL)
	return;

    // Determine number of global categories.
    unsigned nglobalcat = 0;
    for (const struct categories * c = _settings.global_categories; c; c = c->next)
	++nglobalcat;

    // We're taking over the program!
    while (1) {
	// Determine number of categories for current_feed.
	unsigned nfeedcat = 0;
	for (const struct feedcategories * c = current_feed->feedcategories; c; c = c->next)
	    ++nfeedcat;

	UISupportDrawBox ((COLS / 2) - 37, 2, (COLS / 2) + 37, 1 + 4 + nfeedcat + 1);

	attron (WA_REVERSE);
	char titlebuf [256];
	snprintf (titlebuf, sizeof(titlebuf), _("Category configuration for %s"), current_feed->title);
	mvadd_utf8 (3, (COLS - utf8_length(titlebuf))/2u, titlebuf);

	char catletter = '1';
	// No category defined yet
	if (current_feed->feedcategories == NULL) {
	    unsigned y = 5;
	    mvadd_utf8 (y, (COLS / 2) - 33, _("No category defined. Select one or add a new category:"));
	    for (const struct categories * c = _settings.global_categories; c; ++y, c = c->next) {
		mvprintw (y + 1, COLS / 2 - 33, "%c. %s", catletter, c->name);
		if (++catletter == '9' + 1)
		    catletter = 'a';	// Fast forward to 'a' after the digits
	    }
	    char tmp[128];
	    snprintf (tmp, sizeof (tmp), _("Select category number to add, press 'A' to add a new category or '%c' to quit."), _settings.keybindings.quit);
	    UIStatus (tmp, 0, 0);
	} else {
	    unsigned y = 5;
	    mvadd_utf8 (y, (COLS / 2) - 33, _("Categories defined for this feed:"));
	    for (const struct feedcategories * c = current_feed->feedcategories; c; ++y, c = c->next) {
		mvprintw (y + 1, COLS / 2 - 33, "%c. %s", catletter, c->name);
		if (++catletter == '9' + 1)
		    catletter = 'a';	// Fast forward to 'a' after the digits
	    }
	    char tmp[128];
	    snprintf (tmp, sizeof (tmp), _("Select a category number to delete, press 'A' to add a new one or '%c' to quit."), _settings.keybindings.quit);
	    UIStatus (tmp, 0, 0);
	}
	refresh();

	int uiinput = getch();
	if (uiinput == _settings.keybindings.quit)
	    return;
	if (uiinput == 'A') {
	    if (nfeedcat && nglobalcat) {
		// Clear screen area we want to "draw" to.
		UISupportDrawBox ((COLS / 2) - 37, 5, (COLS / 2) + 37, 2 + 3 + nglobalcat + 1);

		attron (WA_REVERSE);
		catletter = '1';
		unsigned y = 5;
		mvadd_utf8 (y, (COLS / 2) - 33, _("Select a new category or add a new one:"));
		for (const struct categories * c = _settings.global_categories; c; ++y, c = c->next) {
		    mvprintw (y + 1, COLS / 2 - 33, "%c. %s", catletter, c->name);
		    if (++catletter == '9' + 1)
			catletter = 'a';	// Fast forward to 'a' after the digits
		}
		char tmp[128];
		snprintf (tmp, sizeof (tmp), _("Select category number to add, press 'A' to add a new category or '%c' to quit."), _settings.keybindings.quit);
		UIStatus (tmp, 0, 0);

		uiinput = getch();
	    }
	    if (uiinput == 'A') {
		// Clear screen area we want to "draw" to.
		UISupportDrawBox ((COLS / 2) - 37, 5, (COLS / 2) + 37, 1 + 5 + nglobalcat + 1);

		attron (WA_REVERSE);
		UIStatus (_("Enter new category name."), 0, 0);
		char* newcategory = UIOneLineEntryField (COLS / 2 - 33, 6);
		if (strlen (newcategory) == 0)
		    UIStatus (_("Aborted."), 1, 0);
		else if (!FeedCategoryExists (current_feed, newcategory))
		    FeedCategoryAdd (current_feed, newcategory);
		else
		    UIStatus (_("Category already defined for this feed!"), 1, 0);
		free (newcategory);
	    } else	       // To add category below.
		nfeedcat = 0;
	}
	// If uiinput < i (ASCII of max selectable element) ignore event.
	if (((uiinput >= '1' && uiinput <= '9') || (uiinput >= 'a' && uiinput <= 'z')) && uiinput < catletter) {
	    unsigned selcat;
	    if (uiinput <= '9')
		selcat = uiinput - '0';
	    else
		selcat = uiinput - ('a' - 10);
	    if (nfeedcat) {    // feed categories are only deleted in this dialog
		const struct feedcategories* c = current_feed->feedcategories;
		for (; c && selcat > 1; --selcat, c = c->next) {
		}
		FeedCategoryDelete (current_feed, c->name);
	    } else {	       // global categories are only added
		const struct categories* c = _settings.global_categories;
		for (; c && selcat > 1; --selcat, c = c->next) {
		}
		FeedCategoryAdd (current_feed, c->name);
	    }
	}
    }
}

// Allocates and returns a filter string the user has chosen from the list of
// available categories.
// If no filter is chosen a NULL pointer is returned.
char* DialogGetCategoryFilter (void)
{
    // Determine number of global categories.
    unsigned nglobalcat = 0;
    for (const struct categories * c = _settings.global_categories; c; c = c->next)
	++nglobalcat;

    UISupportDrawBox ((COLS / 2) - 35, 2, (COLS / 2) + 35, nglobalcat + 4);

    attron (WA_REVERSE);
    const char* title = _("Select filter to apply");
    mvaddstr (3, (COLS - utf8_length(title)) / 2u, title);

    char catletter = '1';
    unsigned y = 3;
    for (const struct categories * c = _settings.global_categories; c; ++y, c = c->next) {
	mvprintw (y + 1, (COLS / 2) - 33, "%c. %s", catletter, c->name);
	if (++catletter == '9' + 1)
	    catletter = 'a';   // Fast forward to 'a' after the digits
    }

    UIStatus (_("Select a category number or any other key to reset filters."), 0, 0);

    refresh();

    // If uiinput < catletter (ASCII of max selectable element) ignore event.
    int uiinput = getch();
    if (uiinput < '1' || (uiinput > '9' && uiinput < 'a') || uiinput > 'z' || uiinput >= catletter)
	return NULL;
    unsigned sel;
    if (uiinput <= '9')
	sel = uiinput - '0';
    else
	sel = uiinput - ('a' - 10);
    const struct categories* c = _settings.global_categories;
    for (; c && sel > 1; --sel, c = c->next) {
    }
    return strdup (c->name);
}

int UIPerFeedFilter (struct feed* current_feed)
{
    if (current_feed->smartfeed != 0)
	return -1;

    // Clear screen area we want to "draw" to.
    attron (WA_REVERSE);
    UISupportDrawBox (3, 5, COLS - 4, 7);

    UIStatus (_("Enter script to pipe feed through."), 0, 0);

    char* newstring = UIOneLineEntryField (5, 6);
    if (strlen (newstring) == 0) {	// If strlen is zero, return 1.
	free (newstring);
	return 1;
    } else if (strstr (newstring, "|") != NULL) {	// If newname contains "|", abort since this is used as a delimiter for the config file.
	free (newstring);
	return 3;
    } else {
	free (current_feed->perfeedfilter);
	current_feed->perfeedfilter = newstring;
	return 0;
    }
}
