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

#include "agg.hpp"
#include "hsl.hpp"

class VistaBar {
public:
	VistaBar() { SetHue(0.0); }
	VistaBar(double hue) { SetHue(hue); }

	VistaBar(const VistaBar &bar) { SetHue(bar.m_hue); }
	VistaBar& operator=(const VistaBar &bar) { SetHue(bar.m_hue); return *this; }

	void SetHue(double hue);
	double Hue() const { return m_hue; }

	void Gradient(const VistaBar &to, double k);

	void Draw(renderer_base &ren, unsigned int width, unsigned int height) const;
	void Draw(HDC hdc, unsigned int width, unsigned int height);

	const hsla& GetBaseColor() const { return m_base; }

private:
	hsla m_base;

	double m_hue;
	agg::rgba m_backleft, m_backright;
	struct { agg::rgba topleft, bottomleft, topright, bottomright; } m_bottomgloss;

	bool m_changed;
	agg::pixel_map m_pixmap;
};
