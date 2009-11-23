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

#pragma once

#include <cstddef>
#include <vector>

// decompresses a .gz file in memory to a buffer.
void ungzip(std::vector<char> &buf, const char *mem, std::size_t len);

// decompresses all the files in a .7z file in memory to a vector of buffers.
void un7zip(std::vector<std::vector<char> > &bufs, const char *mem, std::size_t len);

// decompresses all the files in a .zip file in memory to a vector of buffers.
void unzip(std::vector<std::vector<char> > &bufs, const char *mem, std::size_t len);
