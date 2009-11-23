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

#include <algorithm>
#include "agg.hpp"

struct hsla {
	double h;
	double s;
	double l;
	double a;

	hsla() {}
	hsla(double h, double s, double l, double a = 1.0) : h(h),s(s),l(l),a(a) {}

	hsla& shift_hue(double v) {
		h += v;
		if(h < 0.0 || h > 1.0) h -= floor(h);

		return *this;
	}

	hsla& shift_lightness(double v) {
		l = std::max(std::min(l + v, 1.0), 0.0);
		return *this;
	}
};

hsla rgb_hsl(const agg::rgba &rgb);
hsla rgb_hsl(double r, double g, double b, double a = 1.0);

agg::rgba hsl_rgb(const hsla &hsl);
agg::rgba hsl_rgb(double h, double s, double l, double a = 1.0);

agg::rgba shift_lightness(const agg::rgba &rgb, double value);
