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
#include "vistabar.hpp"

void VistaBar::SetHue(double hue) {
	m_hue = hue;
	m_base = hsla(0.0, 0.91, 0.27).shift_hue(hue);

	m_backleft = hsl_rgb(m_base);
	m_backright = hsl_rgb(hsla(-0.05, 0.65, 0.32).shift_hue(hue));

	m_bottomgloss.topleft = m_bottomgloss.bottomleft = hsl_rgb(hsla(-0.04, 0.65, 0.37).shift_hue(hue));
	m_bottomgloss.topleft.a = 0.0;

	m_bottomgloss.topright = m_bottomgloss.bottomright = hsl_rgb(hsla(-0.06, 0.55, 0.4).shift_hue(hue));
	m_bottomgloss.topright.a = 0.0;
	
	m_changed = true;
}

void VistaBar::Gradient(const VistaBar &to, double k) {
	m_backleft = m_backleft.gradient(to.m_backleft, k);
	m_backright = m_backright.gradient(to.m_backright, k);

	m_bottomgloss.topleft = m_bottomgloss.topleft.gradient(to.m_bottomgloss.topleft, k);
	m_bottomgloss.bottomleft = m_bottomgloss.bottomleft.gradient(to.m_bottomgloss.bottomleft, k);
	m_bottomgloss.topright = m_bottomgloss.topright.gradient(to.m_bottomgloss.topright, k);
	m_bottomgloss.bottomright = m_bottomgloss.bottomright.gradient(to.m_bottomgloss.bottomright, k);
}

void VistaBar::Draw(renderer_base &ren, unsigned int width, unsigned int height) const {
	{ // draw background gradient.
		const double dwidth = width - 1;

		for(unsigned int i = 0; i < width; ++i) {
			const double k = i / dwidth;

			const agg::rgba c = m_backleft.gradient(m_backright, k);
			ren.copy_vline(i, 0, height - 1, c);
		}
	}

	{ // draw top gloss.
		const double dheight = height - 1;

		const agg::rgba gtop(1.0, 1.0, 1.0, 0.3);
		const agg::rgba gbottom(0.5, 0.5, 0.5, 0.1);

		for(unsigned int i = height / 2; i < height; ++i) {
			const double k = i / dheight;

			const agg::rgba c = gbottom.gradient(gtop, k);
			ren.blend_hline(0, i, width - 1, c, 255);
		}
	}

	{ // draw bottom gloss.
		const unsigned int gheight = height / 4;
		const double dgheight = gheight - 1;
		const double dwidth = width - 1;

		for(unsigned int i = 0; i < width; ++i) {
			const double wk = i / dwidth;

			const agg::rgba tc = m_bottomgloss.topleft.gradient(m_bottomgloss.topright, wk);
			const agg::rgba bc = m_bottomgloss.bottomleft.gradient(m_bottomgloss.bottomright, wk);

			for(unsigned int j = 0; j < gheight; ++j) {
				const double hk = j / dgheight;

				const agg::rgba c = bc.gradient(tc, hk);
				ren.blend_pixel(i, j, c, 255);
			}
		}
	}

	{ // draw border lines to make it pop out a little.
		//TODO: this needs to be made DPI-aware.

		const agg::rgba c(1.0, 1.0, 1.0, 1.0 / 3.0);

		ren.blend_hline(0, 0, width - 1, c, 255);
		ren.blend_hline(0, height - 1, width - 1, c, 255);

		ren.blend_vline(0, 0, height - 1, c, 255);
		ren.blend_vline(width - 1, 0, height - 1, c, 255);
	}
}

void VistaBar::Draw(HDC hdc, unsigned int width, unsigned int height) {
	if(!m_pixmap.bitmap_info() || m_pixmap.width() != width || m_pixmap.height() != height) {
		m_pixmap.create(width, height, org_color);
		m_changed = true;
	}

	if(m_changed) {
		agg::rendering_buffer rbuf(m_pixmap.buf(), width, height, m_pixmap.stride());

		pixfmt pf(rbuf);
		renderer_base ren(pf);

		Draw(ren, width, height);
		m_changed = false;
	}

	m_pixmap.draw(hdc);
}
