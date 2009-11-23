/*
	Copyright (C) 2007 Cory Nelson

	This software is provided 'as-is', without any express or implied
	warranty.  In no event will the authors be held liable for any damages
	arising from the use of this software.

	Permission is granted to anyone to use this software for any purpose,
	including commercial applications, and to alter it and redistribute it
	freely, subject to the following restrictions:

	1. The origin of this software must not be misrepresented; you must not
		claim that you wrote the original software. If you use this software
		in a product, an acknowledgment in the product documentation would be
		appreciated but is not required.
	2. Altered source versions must be plainly marked as such, and must not be
		misrepresented as being the original software.
	3. This notice may not be removed or altered from any source distribution.
*/

#include "stdafx.h"
#include "path.hpp"
#include "xmllite.hpp"
#include "config.hpp"

pg3_position::pg3_position() {
	x = y = cx = cy = -1;
}

user_config::user_config() {
	main.position.cx = 640;
	main.position.cy = 480;

	main.columns.time = 40;
	main.columns.address = 96;
	main.columns.label = 128;
	main.columns.count = 40;

	//TODO: find good defaults for these.
	lists.presets.publisher = 64;
	lists.presets.label = 64;
	lists.presets.type = 64;
	lists.presets.status = 64;
	lists.custom.label = 64;
	lists.custom.location = 64;
	lists.custom.type = 64;
	lists.custom.status = 64;
	lists.quicklist.label = 64;
	lists.quicklist.address = 64;
	lists.quicklist.type = 64;
	lists.quicklist.expire = 64;
	lists.apps.label = 64;
	lists.apps.application = 64;
	lists.apps.type = 64;
}

user_config::~user_config() {
}

bool user_config::load() {
	return true;
}

void user_config::save() const {
	path p = path::temp_file(L"pg3");
	xml_writer writer(p.c_str());

	writer.indent();
	writer.start_document();

	writer.start_element(L"PeerGuardian");
	writer.attribute(L"Version", L"3.0");

	//

	writer.comment(L"All sizes are specified in points.  1 point is 1/72 of an inch.");

	writer.start_element(L"Main");
	{
		wchar_t buf[33];

		writer.start_element(L"Position");
		{
			writer.comment(L"Size/position of the main window");

			writer.attribute(L"x", _itow(main.position.x, buf, 10));
			writer.attribute(L"y", _itow(main.position.y, buf, 10));
			writer.attribute(L"width", _itow(main.position.cx, buf, 10));
			writer.attribute(L"height", _itow(main.position.cy, buf, 10));
		}
		writer.end_element();

		writer.start_element(L"Columns");
		{
			writer.comment(L"Width of the log columns in the main window");

			writer.element_string(L"Time", _ultow(main.columns.time, buf, 10));
			writer.element_string(L"Address", _ultow(main.columns.address, buf, 10));
			writer.element_string(L"Label", _ultow(main.columns.label, buf, 10));
			writer.element_string(L"Count", _ultow(main.columns.count, buf, 10));
		}
		writer.end_element();
	}
	writer.end_element();

	writer.start_element(L"ListManager");
	{
		wchar_t buf[33];

		writer.start_element(L"Presets");
		{
			writer.comment(L"Width of presets columns in the Lists tab");

			writer.element_string(L"Label", _ultow(lists.presets.label, buf, 10));
			writer.element_string(L"Type", _ultow(lists.presets.type, buf, 10));
			writer.element_string(L"Status", _ultow(lists.presets.status, buf, 10));
		}
		writer.end_element();

		writer.start_element(L"CustomLists");
		{
			writer.comment(L"Width of custom list columns in the Lists tab");

			writer.element_string(L"Label", _ultow(lists.custom.label, buf, 10));
			writer.element_string(L"Location", _ultow(lists.custom.location, buf, 10));
			writer.element_string(L"Type", _ultow(lists.custom.type, buf, 10));
			writer.element_string(L"Status", _ultow(lists.custom.status, buf, 10));
		}
		writer.end_element();

		writer.start_element(L"QuickList");
		{
			writer.comment(L"Width of columns in the Quick List tab");

			writer.element_string(L"Label", _ultow(lists.quicklist.label, buf, 10));
			writer.element_string(L"Address", _ultow(lists.quicklist.address, buf, 10));
			writer.element_string(L"Type", _ultow(lists.quicklist.type, buf, 10));
			writer.element_string(L"Expire", _ultow(lists.quicklist.expire, buf, 10));
		}
		writer.end_element();

		writer.start_element(L"ApplicationList");
		{
			writer.comment(L"Width of custom list columns in the Applications tab");

			writer.element_string(L"Label", _ultow(lists.apps.label, buf, 10));
			writer.element_string(L"Application", _ultow(lists.apps.application, buf, 10));
			writer.element_string(L"Type", _ultow(lists.apps.type, buf, 10));
		}
		writer.end_element();

		writer.start_element(L"AddApplication");
		{
			writer.comment(L"Sizes for the Add Application dialog");

			writer.start_element(L"Position");
			{
				writer.comment(L"Size/position of the window");

				writer.attribute(L"x", _itow(lists.addapp.position.x, buf, 10));
				writer.attribute(L"y", _itow(lists.addapp.position.y, buf, 10));
				writer.attribute(L"width", _itow(lists.addapp.position.cx, buf, 10));
				writer.attribute(L"height", _itow(lists.addapp.position.cy, buf, 10));
			}
			writer.end_element();

			writer.start_element(L"Columns");
			{
				writer.comment(L"Width of list columns");

				writer.element_string(L"Name", _ultow(lists.addapp.columns.name, buf, 10));
				writer.element_string(L"Path", _ultow(lists.addapp.columns.path, buf, 10));
			}
		}
		writer.end_element();
	}
	writer.end_element();

	//
	
	writer.end_element();
	writer.end_document();

	writer.close();
	path::move(p, config_path(), true);
}

path user_config::base_path() {
	return path::base_dir();
	//return path::appdata_dir() / L"PeerGuardian 3";
}

path user_config::config_path() {
	return base_path() / L"user.conf";
}
