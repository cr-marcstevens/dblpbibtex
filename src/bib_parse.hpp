//          Copyright Marc Stevens 2010 - 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef DBLPBIBTEX_BIB_PARSE_HPP
#define DBLPBIBTEX_BIB_PARSE_HPP

#include <string>

#include <contrib/string_algo.hpp>
namespace sa = string_algo;

// finds starting position of the next bib-entry in bibstr
std::string::size_type str_findbibentry(const std::string& bibstr, std::string::size_type offset = 0)
{
	const auto npos = std::string::npos;
	while (true) {
		auto pos = bibstr.find('@', offset);
		if (pos == npos) return npos;
		auto pos2 = bibstr.find_first_of("{(", pos);
		if (pos2 == npos) return npos;
		std::string cittype = sa::trim_copy(sa::to_lower_copy(bibstr.substr(pos + 1, pos2 - pos - 1)));
		if (cittype == "article" || cittype == "book" || cittype == "booklet"
			|| cittype == "inbook" || cittype == "incollection" || cittype == "inproceedings"
			|| cittype == "manual" || cittype == "mastersthesis" || cittype == "misc"
			|| cittype == "phdthesis" || cittype == "proceedings" || cittype == "techreport"
			|| cittype == "unpublished")
			return pos;
		offset = pos + 1;
	}
}

// extracts first complete bib-entry from str starting at pos
std::string extract_bibentry(const std::string& str, std::string::size_type pos)
{
	// check that bibentry indeed starts at pos
	if (pos >= str.size() || str[pos] != '@')
		return std::string();
	// find first opening bracket
	auto posbr = str.find_first_of("{", pos+1);
	if (posbr > str.size())
		return std::string();
	// find position of closing bracket
	int open_brackets = 1;
	posbr = str.find_first_of("{}", posbr+1);
	while (posbr < str.size())
	{
		int countslashes = 0;
		while (str[posbr - countslashes - 1] == '\\')
			++countslashes;
		// if bracket is not escaped then count it
		if ((countslashes & 1) == 0)
		{
			if (str[posbr] == '{')
				++open_brackets;
			else if (str[posbr] == '{')
				if (--open_brackets == 0)
					break;
		}
		posbr = str.find_first_of("{}", posbr + 1);
	}
	if (open_brackets == 0)
		return str.substr(pos, posbr-pos+1);
	return std::string();
}

// extracts first complete bib-entry from str if any
std::string extract_bibentry(const std::string& str)
{
	std::string::size_type pos = str_findbibentry(str);
	while (pos < str.size())
	{
		std::string ret = extract_bibentry(str, pos);
		if (!ret.empty())
			return ret;
		pos = str_findbibentry(str, pos + 1);
	}
	return std::string();
}

#endif
