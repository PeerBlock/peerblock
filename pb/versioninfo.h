/*
	Copyright (C) 2009-2010 PeerBlock, LLC

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

#define DO_MAKE_STR(x) #x
#define MAKE_STR(x) DO_MAKE_STR(x)

// only one of these should be uncommented at any given time
#define PB_RELTYPE_STABLE
//#define PB_RELTYPE_BETA
//#define PB_RELTYPE_TEST
//#define PB_RELTYPE_DEV

// BUILDDATE: YYMMDDnnnn, where YY MM and DD are the year, month, and day of this build, and nnnn is for build-number
#define BUILDDATE 1011060$WCREV$	// PB_REV

#define PB_VER_BUILDNUM $WCREV$

#define PB_VER_MAJOR 1
#define PB_VER_MINOR 1
#define PB_VER_BUGFIX 0

#ifdef PB_RELTYPE_STABLE
//#define PB_BLDSTR "PeerBlock " MAKE_STR(PB_VER_MAJOR) "." MAKE_STR(PB_VER_MINOR) "." MAKE_STR(PB_VER_BUGFIX) 
#define PB_BLDSTR "PeerBlock " MAKE_STR(PB_VER_MAJOR) "." MAKE_STR(PB_VER_MINOR)
#endif
#ifdef PB_RELTYPE_BETA
#define PB_BLDSTR "PeerBlock " MAKE_STR(PB_VER_MAJOR) "." MAKE_STR(PB_VER_MINOR) "+" " (r" MAKE_STR(PB_VER_BUILDNUM) ") - BETA RELEASE" 
//#define PB_BLDSTR "PeerBlock r" MAKE_STR(PB_VER_BUILDNUM) " - BETA RELEASE" 
#endif
#ifdef PB_RELTYPE_TEST
#define PB_BLDSTR "PeerBlock r" MAKE_STR(PB_VER_BUILDNUM) " - INTERNAL TEST RELEASE" 
#endif
#ifdef PB_RELTYPE_DEV
#define PB_BLDSTR "PeerBlock r" MAKE_STR(PB_VER_BUILDNUM) " - DEV BUILD" 
#endif
