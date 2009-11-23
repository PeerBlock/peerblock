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

#include "defs.hpp"

#include <cstring>
#include <ostream>
#include <fstream>
#include <iostream>
#include <windows.h>
#include <psapi.h>
#include "p2p/key.hpp"
#include "p2p/list.hpp"
#include "p2p/save.hpp"
#include "config.hpp"
#include "timer.hpp"

static const wchar_t* const g_help =
L"usage: p2pconv [-of <file>] [-ot <type>] [-ol <level>] [<file>[*key],...] [-<file>[*key],...]\n"
L"  -of <file>     set the output file.  if not specified, writes to stdout.\n"
L"  -ot <type>     set the output type.  if not specified, uses up2p.\n"
L"                   p2p:  PeerGuardian text list.\n"
L"                   up2p: PeerGuardian text list (Unicode).\n"
L"                   dat:  eMule text list.\n"
L"                   udat: eMule text list (Unicode).\n"
L"                   p2b:  PeerGuardian binary list (P2Bv4).\n"
L"                   p2b1: PeerGuardian binary list (P2Bv1).\n"
L"                   p2b2: PeerGuardian binary list (P2Bv2).\n"
L"                   p2b3: PeerGuardian binary list (P2Bv3).\n"
L"                   p2b4: PeerGuardian binary list (P2Bv4).\n"
L"  -ol <level>    sets the output level on an eMule text list, from 0 to 255.\n"
L"                   < 128: block.\n"
L"                   >= 128: allow.\n"
L"  -k  <file>     sign a P2Bv4 file with the private key specified.\n"
L"  -g             generates a private key.\n"
L"  -pk            derives a public key from the given private key.\n"
L"  -p  <password> specify the password for a private key.\n"
L"  -d             print diagnostic information (cpu times, memory usage)\n\n"
L"If multiple lists are given, they are merged.  If a list starts with\n"
L"a -, it will erase any ranges in it from what has been read already.\n\n"
L"example: listconv -of out.p2p list1.p2b*key.txt list2.txt -list3.txt list4.txt\n\n"
L"This will merge list1.p2b with list2.txt (verifying that list1.p2b is\n"
L"authentic against the key stored in key.txt), subtract list3.txt from them,\n"
L"then merge list4.txt with what is left.  The resulting list will be written to\n"
L"out.p2p\n";

static void print_help() {
	std::wcerr << g_help << std::flush;
}

enum save_type { list_p2p, list_up2p, list_dat, list_udat, list_p2b1, list_p2b2, list_p2b3, list_p2b4 };

int __cdecl wmain(int argc, wchar_t *argv[]) {
	bool diag = false;
	timer t;

	try {
		crypto::init();

		p2p::list list;

		std::ostream *out = &std::cout;
		std::ofstream outfile;
		char outbuf[8192];

		p2p::key key;
		bool genkey = false;
		bool genpublic = false;
		const wchar_t *keyfile = NULL;
		const wchar_t *password = L"";

		save_type type = list_up2p;
		unsigned int level = 100;

		unsigned int lists = 0;

		t.start();

		for(int i = 1; i < argc; ++i) {
			if(!wcscmp(argv[i], L"-h") || !wcscmp(argv[i], L"--help")) {
				print_help();
				return 0;
			}
			else if(!wcscmp(argv[i], L"-of")) {
				if(++i < argc) {
					outfile.open(argv[i], std::ofstream::out | std::ofstream::binary);

					if(outfile.is_open()) {
						// buffered for faster output!
						outfile.rdbuf()->pubsetbuf(outbuf, 8192);
						out = &outfile;
					}
					else {
						std::wcerr << L"error: unable to open output file \"" << argv[i] << L'"' << std::endl;
						return -1;
					}
				}
				else {
					print_help();
					return 0;
				}
			}
			else if(!wcscmp(argv[i], L"-ot")) {
				if(++i < argc) {
					if(!wcscmp(argv[i], L"p2p")) type = list_p2p;
					else if(!wcscmp(argv[i], L"up2p")) type = list_up2p;
					else if(!wcscmp(argv[i], L"dat")) type = list_dat;
					else if(!wcscmp(argv[i], L"udat")) type = list_udat;
					else if(!wcscmp(argv[i], L"p2b1")) type = list_p2b1;
					else if(!wcscmp(argv[i], L"p2b2")) type = list_p2b2;
					else if(!wcscmp(argv[i], L"p2b3")) type = list_p2b3;
					else if(!wcscmp(argv[i], L"p2b") || !wcscmp(argv[i], L"p2b4")) type = list_p2b4;
					else {
						std::wcerr << L"error: unknown list type \"" << argv[i] << L'"' << std::endl;
						return -1;
					}
				}
				else {
					print_help();
					return 0;
				}
			}
			else if(!wcscmp(argv[i], L"-ol")) {
				if(++i < argc) {
					wchar_t *end;
					level = wcstoul(argv[i], &end, 10);

					if(!level && end != wcschr(argv[i], L'\0') || level > 255) {
						std::wcerr << L"error: invalid level.  please enter a number between 0 and 255." << std::endl;
						return -1;
					}
				}
				else {
					print_help();
					return 0;
				}
			}
			else if(!wcscmp(argv[i], L"-k")) {
				if(++i < argc) {
					keyfile = argv[i];
				}
				else {
					print_help();
					return 0;
				}
			}
			else if(!wcscmp(argv[i], L"-g")) {
				genkey = true;
			}
			else if(!wcscmp(argv[i], L"-p")) {
				if(++i < argc) {
					password = argv[i];
				}
				else {
					print_help();
					return 0;
				}
			}
			else if(!wcscmp(argv[i], L"-pk")) {
				genpublic = true;
			}
			else if(!wcscmp(argv[i], L"-d")) {
				diag = true;
			}
			else {
				p2p::key k;

				wchar_t *sep = wcschr(argv[i], L'*');
				if(sep) {
					*sep = L'\0';
					k.load(++sep);
				}

				p2p::list tmp;
				load_list(tmp, (argv[i][0] != L'-') ? argv[i] : (argv[i] + 1), sep ? &k : 0);

				if(argv[i][0] != L'-') {
					tmp.sort();
					list.merge(tmp);
				}
				else {
					list.erase(tmp);
				}

				++lists;
			}
		}

		if(keyfile) {
			key.load(keyfile, password);

			if(type == list_p2b4 && key.type() != crypto::ecc_key::private_key) {
				std::wcerr << L"error: when signing a P2Bv4 list, a private key must be specified with -k." << std::endl;
				return -1;
			}
		}

		if(genkey) {
			key.generate();
			key.save_private(*out, password);
			if(!genpublic) return 0;
		}

		if(genpublic) {
			if(key.type() != crypto::ecc_key::private_key) {
				std::wcerr << L"error: when using -pk, a private key must be specified with -k." << std::endl;
				return -1;
			}

			key.save_public(*out);
			return 0;
		}

		if(!lists) {
			print_help();
			return 0;
		}
		
		list.sort();
		list.optimize();

		if(type != list_p2b4) {
			if(!list.m_list6.empty()) {
				std::wcerr << L"warning: list contains IPv6 addresses but output format does not support IPv6." << std::endl;
			}

			if(keyfile) {
				std::wcerr << L"warning: private key specified, but output format does not support digital signatures." << std::endl;
			}
		}

		switch(type) {
			case list_p2p: p2p::save_p2p(*out, list, false); break;
			case list_up2p: p2p::save_p2p(*out, list, true); break;
			case list_dat: p2p::save_dat(*out, list, level, false); break;
			case list_udat: p2p::save_dat(*out, list, level, true); break;
			case list_p2b1: p2p::save_p2b1(*out, list); break;
			case list_p2b2: p2p::save_p2b2(*out, list); break;
			case list_p2b3: p2p::save_p2b3(*out, list); break;
			case list_p2b4: p2p::save_p2b4(*out, list, keyfile ? &key : 0); break;
		}

		(*out) << std::flush;

		if(outfile.is_open()) {
			outfile.close();
		}

		t.stop();
	}
	catch(std::exception &ex) {
		std::cerr
			<< "exception occured:" << std::endl
			<< typeid(ex).name() << std::endl
			<< ex.what() << std::endl;

		return -1;
	}

	if(diag) {
		PROCESS_MEMORY_COUNTERS pmc = {0};
		pmc.cb = sizeof(pmc);

		GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));

		fwprintf(stderr,
			L"diagnostics:\n"
			L"--------------\n"
			L"user:             %.4f seconds.\n"
			L"kernel:           %.4f seconds.\n"
			L"total:            %.4f seconds.\n"
			L"wall:             %.4f seconds.\n"
			L"peak working set: %.2f MiB.\n",
			t.user(), t.kernel(), t.total(), t.wall(),
			pmc.PeakWorkingSetSize / 1048576.0);
	}

	return 0;
}
