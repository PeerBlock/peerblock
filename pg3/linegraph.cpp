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
#include "agg.hpp"
#include "hsl.hpp"
#include "helpers.hpp"
#include "linegraph.hpp"
#include "win32_error.hpp"

struct LineStep {
	double step;
	std::wstring label;
};

struct Line {
	agg::rgba color, bgcolor, ptcolor;
	std::deque<LineStep> steps;
	std::wstring label;
	double width, ptsize;
	COLORREF textcolor;
	bool smooth, drawbg, drawpoints;
};

struct LineGraphState {
	std::vector<Line> lines;
	agg::rgba bgcolor;
	HFONT font;
	UINT maxsteps;
	bool rightalign;

	LineGraphState() : bgcolor(1.0, 1.0, 1.0), font(NULL), maxsteps(100), rightalign(false) {}
};

static BOOL LineGraph_OnCreate(HWND hwnd, LPCREATESTRUCT) {
	LineGraphState *state = new LineGraphState;
	SetWindowStatePtr(hwnd, state);

	return TRUE;
}

static void LineGraph_OnDestroy(HWND hwnd) {
	delete GetWindowStatePtr<LineGraphState>(hwnd);
}

class LineGraphLine {
public:
	bool CanDrawPoints() const { return m_drawpoints; }

	LineGraphLine(const Line &line, double maxheight, UINT maxsteps, bool smooth, bool drawpoints, int width, int height) : m_line(line), m_smooth(smooth), m_drawpoints(drawpoints), m_width(width), m_height(height) {
		m_stepsize = width / double(maxsteps - 1);
		m_startx = (maxsteps - line.steps.size()) * m_stepsize;
		m_maxheight = maxheight;
		m_availheight = height - 5;

		m_storage.move_to(m_startx, line.steps[0].step / maxheight * m_availheight);

		for(std::deque<LineStep>::size_type i = 1; i < line.steps.size(); ++i)
			m_storage.line_to(m_startx + m_stepsize * i, line.steps[i].step / maxheight * m_availheight);
	}

	void DrawBackground(rasterizer_scanline &ras, scanline &sl, renderer_aa &ren) {
		agg::path_storage storage;

		if(!m_smooth || m_line.steps.size() == 2) {
			storage = m_storage;
		}
		else {
			agg::conv_bspline<agg::path_storage> bsps(m_storage);
			bsps.interpolation_step(0.125 / m_line.steps.size());

			bsps.rewind(0);

			double x, y;

			bsps.vertex(&x, &y);
			storage.move_to(x, y);

			while(!agg::is_stop(bsps.vertex(&x, &y)))
				storage.line_to(x, y);
		}
		
		storage.line_to(m_width, 0.0);
		storage.line_to(m_startx, 0.0);
	
		agg::conv_close_polygon<agg::path_storage> ps(storage);
		ras.add_path(ps);

		ren.color(m_line.bgcolor);
		agg::render_scanlines(ras, sl, ren);
	}

	void DrawLine(rasterizer_scanline &ras, scanline &sl, renderer_aa &ren) {
		if(!m_smooth || m_line.steps.size()==2) {
			agg::conv_stroke<agg::path_storage> ps(m_storage);

			ps.width(m_line.width);
			ras.add_path(ps);
		}
		else {
			agg::conv_bspline<agg::path_storage> bsps(m_storage);
			bsps.interpolation_step(0.125 / m_line.steps.size());

			agg::conv_stroke<agg::conv_bspline<agg::path_storage> > ps(bsps);

			ps.width(m_line.width);
			ras.add_path(ps);
		}

		ren.color(m_line.color);
		agg::render_scanlines(ras, sl, ren);
	}

	void DrawPoints(rasterizer_scanline &ras, scanline &sl, renderer_aa &ren) {
		for(std::deque<LineStep>::size_type i = 0; i < m_line.steps.size(); ++i) {
			agg::ellipse e(m_startx + m_stepsize * i, m_line.steps[i].step / m_maxheight * m_availheight, m_line.ptsize, m_line.ptsize);
			ras.add_path(e);
		}

		ren.color(m_line.ptcolor);
		agg::render_scanlines(ras, sl, ren);
	}

	void DrawLabels(HDC hdc) {
		COLORREF oldtc=SetTextColor(hdc, m_line.textcolor);

		for(std::deque<LineStep>::size_type i = 0; i < m_line.steps.size(); ++i) {
			const LineStep &step = m_line.steps[i];

			if(step.label.size()) {
				int x = int(m_startx + m_stepsize * i + m_line.ptsize + 1.0);
				int y = m_height - int(step.step / m_maxheight * m_availheight);

				RECT rc = {0};
				DrawText(hdc, step.label.c_str(), (int)step.label.length(), &rc, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_CALCRECT);

				if(x + (rc.right - rc.left) > m_width) rc.left = m_width - (int(m_line.ptsize) + 1) - (rc.right - rc.left);
				else rc.left = x;

				if(y + (rc.bottom - rc.top) > m_height) rc.top = m_height - int(m_line.ptsize) - (rc.bottom - rc.top);
				else rc.top = y;

				rc.right = m_width;
				rc.bottom = m_height;

				DrawText(hdc, step.label.c_str(), (int)step.label.length(), &rc, DT_LEFT | DT_TOP | DT_SINGLELINE);
			}
		}

		SetTextColor(hdc, oldtc);
	}

private:
	const Line &m_line;
	double m_stepsize, m_startx, m_maxheight, m_availheight;
	int m_width, m_height;
	bool m_smooth, m_drawpoints;
	agg::path_storage m_storage;
};

static void LineGraph_OnPaint(HWND hwnd) {
	const LineGraphState &state = *GetWindowStatePtr<LineGraphState>(hwnd);

	RECT rc;
	GetClientRect(hwnd, &rc);

	unsigned int width = rc.right - rc.left;
	unsigned int height = rc.bottom - rc.top;

	agg::pixel_map pixbuf;
	pixbuf.create(width, height, org_color);

	agg::rendering_buffer rbuf(pixbuf.buf(), width, height, pixbuf.stride());

	pixfmt pf(rbuf);
	renderer_base ren(pf);
	
	ren.clear(state.bgcolor);

	boost::ptr_vector<LineGraphLine> lines;

	if(state.lines.size()) {
		renderer_aa ren_aa(ren);
		rasterizer_scanline ras_aa;
		scanline sl;

		double maxheight = 1.0;

		for(std::vector<Line>::const_iterator iter = state.lines.begin(), end = state.lines.end(); iter != end; ++iter) {
			for(std::deque<LineStep>::const_iterator siter = iter->steps.begin(), send = iter->steps.end(); siter != send; ++siter) {
				if(siter->step > maxheight) maxheight = siter->step;
			}
		}

		for(std::vector<Line>::const_iterator iter = state.lines.begin(), end = state.lines.end(); iter != end; ++iter) {
			if(iter->steps.size() >= 2) {
				LineGraphLine *ln = new LineGraphLine(*iter, maxheight, state.maxsteps, iter->smooth, iter->drawpoints, width, height);
				lines.push_back(ln);

				if(iter->drawbg) {
					ln->DrawBackground(ras_aa, sl, ren_aa);
				}
			}
		}

		for(boost::ptr_vector<LineGraphLine>::size_type i = 0; i < lines.size(); ++i)
			lines[i].DrawLine(ras_aa, sl, ren_aa);

		for(boost::ptr_vector<LineGraphLine>::size_type i = 0; i < lines.size(); ++i) {
			if(lines[i].CanDrawPoints()) lines[i].DrawPoints(ras_aa, sl, ren_aa);
		}
	}

	// paint to screen.

	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(hwnd, &ps);

	pixbuf.draw(hdc);

	HGDIOBJ oldfont;
	if(state.font != NULL) oldfont = SelectObject(hdc, (HGDIOBJ)state.font);

	int oldmode = SetBkMode(hdc, TRANSPARENT);

	for(boost::ptr_vector<LineGraphLine>::size_type i = 0; i < lines.size(); ++i) {
		lines[i].DrawLabels(hdc);
	}

	SetBkMode(hdc, oldmode);
	if(state.font != NULL) SelectObject(hdc, oldfont);

	EndPaint(hwnd, &ps);
}

static void LineGraph_OnSize(HWND hwnd, UINT, int, int) {
	InvalidateRect(hwnd, NULL, FALSE);
}

static void LineGraph_OnSetFormat(HWND hwnd, const LINEGRAPH_FORMAT &fmt) {
	LineGraphState &state = *GetWindowStatePtr<LineGraphState>(hwnd);

	bool changed = false;

	if(fmt.dwFlags & LINEGRAPH_FLAG_ALIGN) {
		bool ra = (fmt.dwAlign == LINEGRAPH_FLAG_ALIGN);

		if(state.rightalign != ra) {
			state.rightalign = ra;
			changed = true;
		}
	}

	if(fmt.dwFlags & LINEGRAPH_FLAG_BGCOLOR) {
		agg::rgba c(
			GetRValue(fmt.crBackgroundColor) / 255.0,
			GetGValue(fmt.crBackgroundColor) / 255.0,
			GetBValue(fmt.crBackgroundColor) / 255.0);

		if(state.bgcolor.r != c.r || state.bgcolor.g != c.g || state.bgcolor.b != c.b) {
			state.bgcolor = c;
			changed = true;
		}
	}

	if((fmt.dwFlags & LINEGRAPH_FLAG_MAXSTEPS) && state.maxsteps != fmt.uMaxSteps) {
		state.maxsteps = fmt.uMaxSteps;

		for(std::vector<Line>::size_type i = 0; i < state.lines.size(); ++i) {
			Line &l = state.lines[i];
			while(l.steps.size() > state.maxsteps) l.steps.pop_front();
		}

		changed = true;
	}

	if((fmt.dwFlags & LINEGRAPH_FLAG_FONT) && state.font != fmt.hFont) {
		state.font = fmt.hFont;
		changed = true;
	}

	if(changed) InvalidateRect(hwnd, NULL, FALSE);
}

static void LineGraph_OnGetFormat(HWND hwnd, LINEGRAPH_FORMAT &fmt) {
	const LineGraphState &state = *GetWindowStatePtr<LineGraphState>(hwnd);

	if(fmt.dwFlags & LINEGRAPH_FLAG_ALIGN) {
		fmt.dwAlign = state.rightalign ? LINEGRAPH_ALIGN_RIGHT : LINEGRAPH_ALIGN_LEFT;
	}

	if(fmt.dwFlags & LINEGRAPH_FLAG_BGCOLOR) {
		fmt.crBackgroundColor = RGB(state.bgcolor.r * 255.0, state.bgcolor.g * 255.0, state.bgcolor.b * 255.0);
	}

	if(fmt.dwFlags & LINEGRAPH_FLAG_MAXSTEPS) {
		fmt.uMaxSteps = state.maxsteps;
	}

	if(fmt.dwFlags & LINEGRAPH_FLAG_FONT) {
		fmt.hFont = state.font;
	}
}

static UINT LineGraph_OnAddLine(HWND hwnd, const LINEGRAPH_LINE &line) {
	LineGraphState &state = *GetWindowStatePtr<LineGraphState>(hwnd);

	Line l;

	l.width = PointsToPixels((line.dwFlags & LINEGRAPH_FLAG_WIDTH) ? line.dWidth : 1.0);
	l.ptsize = PointsToPixels((line.dwFlags & LINEGRAPH_FLAG_POINTSIZE) ? (line.dPointSize * 0.5) : 3.0);

	if((line.dwFlags & LINEGRAPH_FLAG_LABEL) && line.pszLabel) {
		l.label = line.pszLabel;
	}

	if(line.dwFlags & LINEGRAPH_FLAG_COLOR) {
		l.color.r = GetRValue(line.crColor) / 255.0;
		l.color.g = GetGValue(line.crColor) / 255.0;
		l.color.b = GetBValue(line.crColor) / 255.0;
		l.color.a = 1.0;
	}
	else {
		l.color = agg::rgba(0.0, 0.0, 0.0);
	}

	if(line.dwFlags & LINEGRAPH_FLAG_BGCOLOR) {
		l.bgcolor.r = GetRValue(line.crBackgroundColor) / 255.0;
		l.bgcolor.g = GetGValue(line.crBackgroundColor) / 255.0;
		l.bgcolor.b = GetBValue(line.crBackgroundColor) / 255.0;
		l.bgcolor.a = 1.0;
	}
	else {
		l.bgcolor = agg::rgba(l.color, 0.125);
	}

	if(line.dwFlags & LINEGRAPH_FLAG_POINTCOLOR) {
		l.ptcolor.r = GetRValue(line.crPointColor) / 255.0;
		l.ptcolor.g = GetGValue(line.crPointColor) / 255.0;
		l.ptcolor.b = GetBValue(line.crPointColor) / 255.0;
		l.ptcolor.a = 1.0;
	}
	else {
		l.ptcolor = shift_lightness(l.color, -0.25);
	}

	if(line.dwFlags & LINEGRAPH_FLAG_TEXTCOLOR) {
		l.textcolor = line.crTextColor;
	}
	else {
		l.textcolor = RGB(l.ptcolor.r * 255.0, l.ptcolor.g * 255.0, l.ptcolor.b * 255.0);
	}

	l.smooth = (line.dwFlags & LINEGRAPH_FLAG_SMOOTH) && (line.bSmooth != FALSE);
	l.drawbg = (line.dwFlags & LINEGRAPH_FLAG_DRAWBG) && (line.bDrawBackground != FALSE);
	l.drawpoints = (line.dwFlags & LINEGRAPH_FLAG_DRAWPOINTS) && (line.bDrawPoints != FALSE);

	UINT ret = (UINT)state.lines.size();
	state.lines.push_back(l);

	return ret;
}

static void LineGraph_OnSetLine(HWND hwnd, UINT id, const LINEGRAPH_LINE &line) {
	LineGraphState &state = *GetWindowStatePtr<LineGraphState>(hwnd);

	Line &l = state.lines[id];

	bool changed = false;

	if(line.dwFlags & LINEGRAPH_FLAG_WIDTH) {
		double px = PointsToPixels(line.dWidth);

		if(l.width != px) {
			l.width = px;
			changed = true;
		}
	}

	if(line.dwFlags & LINEGRAPH_FLAG_POINTSIZE) {
		double px = PointsToPixels(line.dPointSize * 0.5);

		if(l.ptsize != px) {
			l.ptsize = px;
			changed = true;
		}
	}

	if(line.dwFlags & LINEGRAPH_FLAG_LABEL) {
		if(line.pszLabel) l.label = line.pszLabel;
		else l.label.clear();

		changed = true;
	}

	if(line.dwFlags & LINEGRAPH_FLAG_COLOR) {
		agg::rgba c(
			GetRValue(line.crColor) / 255.0,
			GetGValue(line.crColor) / 255.0,
			GetBValue(line.crColor) / 255.0);

		if(l.color.r != c.r || l.color.g != c.g || l.color.b != c.b) {
			l.color = c;
			changed = true;
		}
	}

	if(line.dwFlags & LINEGRAPH_FLAG_BGCOLOR) {
		agg::rgba c(
			GetRValue(line.crBackgroundColor) / 255.0,
			GetGValue(line.crBackgroundColor) / 255.0,
			GetBValue(line.crBackgroundColor) / 255.0);

		if(l.bgcolor.r != c.r || l.bgcolor.g != c.g || l.bgcolor.b != c.b) {
			l.bgcolor = c;
			changed = true;
		}
	}

	if(line.dwFlags & LINEGRAPH_FLAG_POINTCOLOR) {
		agg::rgba c(
			GetRValue(line.crPointColor) / 255.0,
			GetGValue(line.crPointColor) / 255.0,
			GetBValue(line.crPointColor) / 255.0);

		if(l.ptcolor.r != c.r || l.ptcolor.g != c.g || l.ptcolor.b != c.b) {
			l.ptcolor = c;
			changed = true;
		}
	}

	if((line.dwFlags & LINEGRAPH_FLAG_TEXTCOLOR) && l.textcolor!=line.crTextColor) {
		l.textcolor = line.crTextColor;
		changed = true;
	}

	if(line.dwFlags & LINEGRAPH_FLAG_SMOOTH) {
		bool smooth = (line.bSmooth != FALSE);

		if(l.smooth != smooth) {
			l.smooth = smooth;
			changed = true;
		}
	}

	if(line.dwFlags & LINEGRAPH_FLAG_DRAWBG) {
		bool drawbg = (line.bDrawBackground != FALSE);

		if(l.drawbg != drawbg) {
			l.drawbg = drawbg;
			changed = true;
		}
	}

	if(line.dwFlags & LINEGRAPH_FLAG_DRAWPOINTS) {
		bool drawpoints = (line.bDrawPoints != FALSE);

		if(l.drawpoints != drawpoints) {
			l.drawpoints = drawpoints;
			changed = true;
		}
	}

	if(changed) InvalidateRect(hwnd, NULL, FALSE);
}

static void LineGraph_OnGetLine(HWND hwnd, UINT id, LINEGRAPH_LINE &line) {
	const LineGraphState &state = *GetWindowStatePtr<LineGraphState>(hwnd);

	const Line &l = state.lines[id];

	if(line.dwFlags & LINEGRAPH_FLAG_WIDTH) {
		line.dWidth = PixelsToPoints(l.width);
	}

	if(line.dwFlags & LINEGRAPH_FLAG_POINTSIZE) {
		line.dPointSize = PixelsToPoints(l.ptsize) * 2.0;
	}

	if(line.dwFlags & LINEGRAPH_FLAG_LABEL) {
		if(l.label.size()) {
			if(l.label.size() > line.dwLabelMax) {
				line.pszLabel = NULL;
				line.dwLabelMax = (DWORD)l.label.size();
			}
			else {
				wcscpy(line.pszLabel, l.label.c_str());
			}
		}
		else {
			line.pszLabel = NULL;
			line.dwLabelMax = 0;
		}
	}

	if(line.dwFlags & LINEGRAPH_FLAG_COLOR) {
		line.crColor = RGB(l.color.r * 255.0, l.color.g * 255.0, l.color.b * 255.0);
	}

	if(line.dwFlags & LINEGRAPH_FLAG_BGCOLOR) {
		line.crBackgroundColor = RGB(l.bgcolor.r * 255.0, l.bgcolor.g * 255.0, l.bgcolor.b * 255.0);
	}

	if(line.dwFlags & LINEGRAPH_FLAG_POINTCOLOR) {
		line.crPointColor = RGB(l.ptcolor.r * 255.0, l.ptcolor.g * 255.0, l.ptcolor.b * 255.0);
	}

	if(line.dwFlags & LINEGRAPH_FLAG_TEXTCOLOR) {
		line.crTextColor = l.textcolor;
	}

	if(line.dwFlags & LINEGRAPH_FLAG_SMOOTH) {
		line.bSmooth = l.smooth;
	}

	if(line.dwFlags & LINEGRAPH_FLAG_DRAWBG) {
		line.bDrawBackground = l.drawbg;
	}

	if(line.dwFlags & LINEGRAPH_FLAG_DRAWPOINTS) {
		line.bDrawPoints = l.drawpoints;
	}
}

static UINT LineGraph_OnGetLineCount(HWND hwnd) {
	const LineGraphState &state = *GetWindowStatePtr<LineGraphState>(hwnd);

	return (UINT)state.lines.size();
}

static void LineGraph_OnRemoveLine(HWND hwnd, UINT line) {
	LineGraphState &state = *GetWindowStatePtr<LineGraphState>(hwnd);

	state.lines.erase(state.lines.begin() + line);

	InvalidateRect(hwnd, NULL, FALSE);
}

static void LineGraph_OnClearLines(HWND hwnd) {
	LineGraphState &state = *GetWindowStatePtr<LineGraphState>(hwnd);

	state.lines.clear();

	InvalidateRect(hwnd, NULL, FALSE);
}

static UINT LineGraph_OnAddStep(HWND hwnd, UINT line, const LINEGRAPH_STEP &asd) {
	LineGraphState &state = *GetWindowStatePtr<LineGraphState>(hwnd);

	Line &l = state.lines[line];

	LineStep step;

	step.step = (asd.dwFlags & LINEGRAPH_FLAG_STEP) ? asd.dStep : 1.0;

	if((asd.dwFlags & LINEGRAPH_FLAG_LABEL) && asd.pszLabel) {
		step.label = asd.pszLabel;
	}

	l.steps.push_back(step);
	while(l.steps.size() > state.maxsteps) l.steps.pop_front();

	UINT id = (UINT)(l.steps.size() - 1);

	InvalidateRect(hwnd, NULL, FALSE);

	return id;
}

static void LineGraph_OnSetStep(HWND hwnd, UINT line, const LINEGRAPH_STEP &asd) {
	LineGraphState &state = *GetWindowStatePtr<LineGraphState>(hwnd);

	LineStep &step = state.lines[line].steps[asd.uStep];

	bool changed = false;

	if((asd.dwFlags & LINEGRAPH_FLAG_STEP) && step.step!=asd.dStep) {
		step.step = asd.dStep;
		changed = true;
	}

	if(asd.dwFlags & LINEGRAPH_FLAG_LABEL) {
		if(asd.pszLabel) {
			step.label = asd.pszLabel;
		}
		else {
			step.label.clear();
		}

		changed = true;
	}

	if(changed) InvalidateRect(hwnd, NULL, FALSE);
}

static void LineGraph_OnGetStep(HWND hwnd, UINT line, LINEGRAPH_STEP &asd) {
	const LineGraphState &state = *GetWindowStatePtr<LineGraphState>(hwnd);

	const LineStep &step = state.lines[line].steps[asd.uStep];

	if(asd.dwFlags & LINEGRAPH_FLAG_STEP) {
		asd.dStep = step.step;
	}

	if(asd.dwFlags & LINEGRAPH_FLAG_LABEL) {
		if(step.label.size()) {
			if(step.label.size() > asd.dwLabelMax) {
				asd.pszLabel = NULL;
				asd.dwLabelMax = (DWORD)step.label.size();
			}
			else {
				wcscpy(asd.pszLabel, step.label.c_str());
			}
		}
		else {
			asd.pszLabel = NULL;
			asd.dwLabelMax = 0;
		}
	}
}

static UINT LineGraph_OnGetStepCount(HWND hwnd, UINT line) {
	const LineGraphState &state = *GetWindowStatePtr<LineGraphState>(hwnd);
	return (UINT)state.lines[line].steps.size();
}

static void LineGraph_OnRemoveStep(HWND hwnd, UINT line, UINT step) {
	LineGraphState &state = *GetWindowStatePtr<LineGraphState>(hwnd);

	std::deque<LineStep> &steps = state.lines[line].steps;
	steps.erase(steps.begin() + step);
}

static void LineGraph_OnClearSteps(HWND hwnd, UINT id) {
	LineGraphState &state = *GetWindowStatePtr<LineGraphState>(hwnd);
	state.lines[id].steps.clear();
}

static LRESULT CALLBACK LineGraph_WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch(msg) {
		HANDLE_MSG(hwnd, WM_CREATE, LineGraph_OnCreate);
		HANDLE_MSG(hwnd, WM_DESTROY, LineGraph_OnDestroy);
		HANDLE_MSG(hwnd, WM_PAINT, LineGraph_OnPaint);
		HANDLE_MSG(hwnd, WM_SIZE, LineGraph_OnSize);
		case WM_LINEGRAPH_SETFORMAT:
			LineGraph_OnSetFormat(hwnd, *(const LINEGRAPH_FORMAT*)lParam);
			return 0;
		case WM_LINEGRAPH_GETFORMAT:
			LineGraph_OnGetFormat(hwnd, *(LINEGRAPH_FORMAT*)lParam);
			return 0;
		case WM_LINEGRAPH_ADDLINE:
			return LineGraph_OnAddLine(hwnd, *(const LINEGRAPH_LINE*)lParam);
		case WM_LINEGRAPH_SETLINE:
			LineGraph_OnSetLine(hwnd, (UINT)wParam, *(const LINEGRAPH_LINE*)lParam);
			return 0;
		case WM_LINEGRAPH_GETLINE:
			LineGraph_OnGetLine(hwnd, (UINT)wParam, *(LINEGRAPH_LINE*)lParam);
			return 0;
		case WM_LINEGRAPH_GETLINECOUNT:
			return LineGraph_OnGetLineCount(hwnd);
		case WM_LINEGRAPH_REMLINE:
			LineGraph_OnRemoveLine(hwnd, (UINT)wParam);
			return 0;
		case WM_LINEGRAPH_CLEARLINES:
			LineGraph_OnClearLines(hwnd);
			return 0;
		case WM_LINEGRAPH_ADDSTEP:
			return LineGraph_OnAddStep(hwnd, (UINT)wParam, *(const LINEGRAPH_STEP*)lParam);
		case WM_LINEGRAPH_SETSTEP:
			LineGraph_OnSetStep(hwnd, (UINT)wParam, *(const LINEGRAPH_STEP*)lParam);
			return 0;
		case WM_LINEGRAPH_GETSTEP:
			LineGraph_OnGetStep(hwnd, (UINT)wParam, *(LINEGRAPH_STEP*)lParam);
			return 0;
		case WM_LINEGRAPH_GETSTEPCOUNT:
			return LineGraph_OnGetStepCount(hwnd, (UINT)wParam);
		case WM_LINEGRAPH_REMSTEP:
			LineGraph_OnRemoveStep(hwnd, (UINT)wParam, (UINT)lParam);
			return 0;
		case WM_LINEGRAPH_CLEARSTEPS:
			LineGraph_OnClearSteps(hwnd, (UINT)wParam);
			return 0;
		default: return DefWindowProc(hwnd, msg, wParam, lParam);
	}
}

void RegisterLineGraph() {
	WNDCLASS wc = {0};
	wc.lpszClassName = LINEGRAPH_CLASS;
	wc.hInstance = GetModuleHandle(NULL);
	wc.lpfnWndProc = LineGraph_WndProc;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.cbWndExtra = sizeof(LineGraphState*);

	if(!RegisterClass(&wc)) {
		throw win32_error("RegisterClass");
	}
}
