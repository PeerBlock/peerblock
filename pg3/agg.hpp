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

#include <agg_basics.h>
#include <agg_rendering_buffer.h>
#include <agg_rasterizer_scanline_aa.h>
#include <agg_renderer_scanline.h>
#include <agg_renderer_primitives.h>
#include <agg_scanline_p.h>
#include <agg_conv_stroke.h>
#include <agg_conv_bspline.h>
#include <agg_conv_close_polygon.h>
#include <agg_conv_transform.h>
#include <agg_ellipse.h>
#include <agg_path_storage.h>
#include <agg_pixfmt_rgb.h>
#include <platform/win32/agg_win32_bmp.h>

typedef agg::pixfmt_bgr24 pixfmt;
#define org_color agg::org_color24

typedef agg::renderer_base<pixfmt> renderer_base;
typedef agg::renderer_scanline_aa_solid<renderer_base> renderer_aa;
typedef agg::renderer_primitives<renderer_base> renderer_primitives;
typedef agg::rasterizer_scanline_aa<> rasterizer_scanline;
typedef agg::scanline_p8 scanline;
