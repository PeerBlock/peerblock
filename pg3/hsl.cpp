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
#include "hsl.hpp"

hsla rgb_hsl(const agg::rgba &rgb) {
	double r=rgb.r, g=rgb.g, b=rgb.b;
	double h, s, l;

	if(r==g && g==b) {
		h=0.0;
		s=0.0;
		l=r;
	}
	else {
		double minc=std::min(r, std::max(g, b));
		double maxc=std::max(r, std::max(g, b));

		l=(minc+maxc)*0.5;

		if(l<0.5) s=(maxc-minc)/(maxc+minc);
		else s=(maxc-minc)/(2.0-maxc-minc);

		if(r==maxc) h=(g-b)/(maxc-minc);
		else if(g==maxc) h=2.0+(b-r)/(maxc-minc);
		else h=4.0+(r-g)/(maxc-minc);

		if(h>1) h/=6.0;
		if(h<0) ++h;
	}

	return hsla(h, s, l, rgb.a);
}

hsla rgb_hsl(double r, double g, double b, double a) {
	return rgb_hsl(agg::rgba(r, g, b, a));
}

agg::rgba hsl_rgb(const hsla &hsl) {
	double h=hsl.h, s=hsl.s, l=hsl.l;
	double r, g, b;

	if(s==0.0) {
		r=l;
		g=l;
		b=l;
	}
	else {
		double tmp1, tmp2, tmpred, tmpgreen, tmpblue;

		if(l<0.5) tmp2=l*(1.0+s);
		else tmp2=(l+s)-(l*s);

		tmp1=2.0*l-tmp2;

		tmpred=h+1.0/3.0;
		if(tmpred>1.0) --tmpred;

		tmpgreen=h;

		tmpblue=h-1.0/3.0;
		if(tmpblue<0.0) ++tmpblue;

		if(tmpred < 1.0/6.0) r=tmp1+(tmp2-tmp1)*6.0*tmpred;
		else if(tmpred<0.5) r=tmp2;
		else if(tmpred < 2.0/3.0) r=tmp1+(tmp2-tmp1)*((2.0/3.0)-tmpred)*6.0;
		else r=tmp1;

		if(tmpgreen < 1.0/6.0) g=tmp1+(tmp2-tmp1)*6.0*tmpgreen;
		else if(tmpgreen<0.5) g=tmp2;
		else if(tmpgreen < 2.0/3.0) g=tmp1+(tmp2-tmp1)*((2.0/3.0)-tmpgreen)*6.0;
		else g=tmp1;

		if(tmpblue < 1.0/6.0) b=tmp1+(tmp2-tmp1)*6.0*tmpblue;
		else if(tmpblue<0.5) b=tmp2;
		else if(tmpblue < 2.0/3.0) b=tmp1+(tmp2-tmp1)*((2.0/3.0)-tmpblue)*6.0;
		else b=tmp1;
	}

	return agg::rgba(r, g, b, hsl.a);
}

agg::rgba hsl_rgb(double h, double s, double l, double a) {
	return hsl_rgb(hsla(h, s, l, a));
}

agg::rgba shift_lightness(const agg::rgba &rgb, double value) {
	hsla c=rgb_hsl(rgb);

	c.l=std::max(std::min(c.l+value, 1.0), 0.0);

	return hsl_rgb(c);
}
