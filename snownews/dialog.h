// This file is part of Snownews - A lightweight console RSS newsreader
//
// Copyright (c) 2003-2004 Oliver Feiler <kiza@kcore.de>
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

#pragma once
#include "main.h"

void UIHelpScreen (void);
void UIDisplayFeedHelp (void);
void UIDisplayItemHelp (void);
char* UIOneLineEntryField (int x, int y);
void UIChangeBrowser (void);
void UIChangeFeedName (struct feed* cur_ptr);
int UIAddFeed (char* newurl);
void FeedInfo (const struct feed* current_feed);
bool UIDeleteFeed (const char* feedname);
void CategorizeFeed (struct feed* current_feed);
char* DialogGetCategoryFilter (void);
int UIPerFeedFilter (struct feed* current_feed);
