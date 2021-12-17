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

struct feed* newFeedStruct (void);
int UpdateFeed (struct feed* cur_ptr);
int UpdateAllFeeds (void);
int LoadFeed (struct feed* cur_ptr);
int LoadAllFeeds (unsigned numfeeds);
void AddFeedToList (struct feed* new_feed);
void AddFeed (const char* url, const char* cname, const char* categories, const char* filter);
void WriteCache (void);
