//          Copyright Marc Stevens 2010 - 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef DBLPBIBTEX_CORE_HPP
#define DBLPBIBTEX_CORE_HPP

#include <iostream>
#include <fstream>
#include <cstdio>
#include <stdexcept>

#include <string>
#include <vector>
#include <set>
#include <map>

#include <contrib/string_algo.hpp>
namespace sa = string_algo;

#ifdef HAVE_CONFIG_H
#include "config.h"
#else
#define DBLPBIBTEX_CXX17FILESYSTEMHEADER <filesystem>
#define DBLPBIBTEX_CXX17FILESYSTEMNAMESPACE std::filesystem
#define _CRT_SECURE_NO_WARNINGS
#define CURL_STATICLIB
#endif

//#define USE_CURL_FORM // use for old versions of curl that doesn't have curl_mime yet

#define DBLPBIBTEX_VERSION "2.3"

#include DBLPBIBTEX_CXX17FILESYSTEMHEADER           /* <filesystem> / <experimental/filesystem> */
namespace fs = DBLPBIBTEX_CXX17FILESYSTEMNAMESPACE; /* std::filesystem / std::experimental::filesystem; */

/*** global variables ***/
std::string bibtexargs; /* from bibtex command line */
std::string auxfile; /* from bibtex command line */
std::vector<std::string> includedirs; /* from bibtex command line */
std::vector<std::string> bibfiles; /* from auxfile */
std::set<std::string> citations; /* from auxfile */
std::vector<std::string> citreferences; /* parsed from citations from bibfiles or downloaded */

std::set<std::string> havecitations; /* parsed from bibfiles: ALL LOWER CASE !!! */
std::map<std::string, std::string> tosearchcitations_complete; /* parsed from to search bibfiles */
std::set<std::string> havecitreferences; /* parsed from bibfiles */
std::string mainbibfile; /* first bibfile encountered will be used to insert downloaded citations and references */
std::string mainbibfilecontent; /* content from main bibfile to insert downloaded citations and references */
std::set<std::string> checkedcitations; /* only download citation once per run to prevent mistakes */

#define DBLP_FORMAT_COMPACT       0
#define DBLP_FORMAT_STANDARD      1
#define DBLP_FORMAT_EXT_CROSSREF  2

/* parameters values loaded from: environment variables, configuration file and auxfile parameters (overwritten in that order)*/
struct parameters_type {
	std::string bibtexcmd;
	std::string mainbibfile; /* this value is later stored in the global mainbibfile */
	std::vector<std::string> bibtexaddargs;
	bool nonewversioncheck;
	bool nodownload;
	bool nodblp;
	int dblpformat;
	bool nocryptoeprint;
	bool enablesearch; /* can only be enabled inside tex file with \nocite{dblpbibtex:enablesearch} */
	bool cleanupmainbib; /* can only be enable inside tex file with \nocite{dblpbibtex:cleanupmainbib} */
};
extern parameters_type params;

/*** small utility functions ***/
std::string dblpformat_name(int dblpformat)
{
	switch (dblpformat)
	{
	default:
	case 1:
		return "standard";
	case 0:
		return "compact";
	case 2:
		return "crossref";
	}
}
std::string read_istream(std::istream& is) {
	std::string tmp;
	char buffer[1024];
	while (is) {
		is.read(buffer, 1024);
		tmp.append(buffer, is.gcount());
	}
	return tmp;
}

bool read_file(const std::string& path, std::string& content)
{
	content.clear();
	if (path.empty())
		return false;
	std::ifstream is(path.c_str());
	if (!is)
		return false;
	content = read_istream(is);
	return true;
}

bool safe_write_file(const std::string& filename, const std::string& content)
{
	if (filename.empty())
		return false;
	try {
		if (fs::exists(filename + ".bak"))
			fs::remove(filename + ".bak");
		if (fs::exists(filename))
			fs::copy_file(filename, filename + ".bak");
	} catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
		return false;
	} catch (...) {
		return false;
	}
	std::ofstream ofs(filename.c_str());
	if (!ofs) return false;
	ofs << content << std::endl;
	if (!ofs) return false;
	return true;
}

std::string getenvvar(const std::string& key) {
	char* str = getenv(key.c_str());
	if (str == 0)
		return std::string();
	return std::string(str);
}


/*** main bibfile operations: load, save, add entry ***/
bool load_mainbibfile()
{
	return read_file(mainbibfile, mainbibfilecontent);
}
bool save_mainbibfile()
{
	return safe_write_file(mainbibfile, mainbibfilecontent);
}
void prepend_to_mainbibfile(const std::string& entry)
{
	mainbibfilecontent = entry + "\n" + mainbibfilecontent;
}
void append_to_mainbibfile(const std::string& entry)
{
	mainbibfilecontent.append(entry + "\n");
}
void add_entry_to_mainbibfile(const std::string& entry, bool prepend = true)
{
	if (prepend)
		prepend_to_mainbibfile(entry);
	else
		append_to_mainbibfile(entry);
}

void replace_in_file(const std::string& filename, const std::string& needle, const std::string& replacement)
{
	std::string content;
	if (!read_file(filename, content))
		return;
	bool changed = false;
	std::string::size_type pos;
	while ((pos = content.find(needle)) != std::string::npos) {
		content.erase(pos, needle.size());
		content.insert(pos, replacement);
		changed = true;
	}
	if (changed) {
		std::cout << "Replaced '" << needle << "' with '" << replacement << "' in tex file: '" << filename << "'" << std::endl;
		safe_write_file(filename, content);
	}
}

/*** replace search citations ***/
/* !for all the .tex files in the same directory! */
void texfiles_replace_key(const std::string& key, const std::vector<std::string>& cits)
{
	if (cits.empty())
		return;
	std::string citsstr = sa::join(cits, ",");
	fs::directory_iterator it(".");
	for (; it != fs::directory_iterator(); ++it)
	{
		if (sa::ends_with(it->path().string(), ".tex"))
			replace_in_file(it->path().string(), key, citsstr);
	}
}

#endif
