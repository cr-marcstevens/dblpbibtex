//          Copyright Marc Stevens 2010 - 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef DBLPBIBTEX_BIB_SEARCH_HPP
#define DBLPBIBTEX_BIB_SEARCH_HPP

#include "core.hpp"

#include <contrib/string_algo.hpp>
namespace sa = string_algo;

/* search functions */
bool search_citation_bib(const std::string& citkey)
{
	std::string key = citkey.substr(citkey.find(':')+1);
	std::vector<std::string> keywords = sa::split(sa::to_lower_copy(key), '+');
	for (auto& keyword : keywords)
		sa::trim(keyword);

	std::vector<std::string> cits;
	for (auto& cit : tosearchcitations_complete)
	{
		bool ok = true;
		for (auto& keyword : keywords)
		{
			if (cit.second.find(keyword) == std::string::npos)
			{
				ok = false;
				break;
			}
		}
		if (ok)
			cits.push_back(cit.first);
	}
	if (cits.empty())
		return false;
	if (cits.size() > 5)
		cits.resize(5);
	texfiles_replace_key(citkey, cits);
	return true;
}

bool search_citation_dblp(const std::string& citkey)
{
	std::string key = citkey.substr(citkey.find(':')+1);
	std::string searchphrase = key;
	if (searchphrase.length() < 5) {
		std::cout << "Search phrase too short <5 chars: '" << searchphrase << "'" << std::endl;
		return false;
	}
	auto p_header_body = url_get("https://dblp.org/search/?q=" + searchphrase);
	auto& html = p_header_body.second;

	html.erase(html.begin(), sa::ifind(html,"<body"));
	std::set<std::string> cits2;
	while (html.find("/rec/bibtex/") != std::string::npos)
	{
		std::string cit = html.substr(html.find("/rec/bibtex/"));
		cit.erase(sa::to_lower_copy(cit).find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789:/.+-$&@=!?"));
		cit.erase(0, std::string("/rec/bibtex/").length());
		cit = "DBLP:" + cit;
		cits2.insert(cit);
		html.erase(0, html.find("/rec/bibtex/")+1);
		std::cout << "Found citations: " << cit << std::endl;
	}
	std::vector<std::string> cits(cits2.begin(), cits2.end());
	if (cits.size() == 0)
		return false;
	if (cits.size() > 5)
		cits.resize(5);
	texfiles_replace_key(citkey, cits);
	return true;
}

bool search_citation_cryptoeprint(const std::string& citkey)
{
	std::string key = citkey.substr(citkey.find(':')+1);
	std::vector<std::string> cits;
	std::string searchstr = sa::replace_all_copy(sa::to_lower_copy(key), std::string("+"), std::string(" "));
	if (searchstr.length() < 5) {
		std::cout << "Search phrase too short <5 chars: '" << searchstr << "'" << std::endl;
		return false;
	}
	std::vector<std::pair<std::string,std::string>> postdata;
	postdata.emplace_back("anywords", searchstr);
/*	std::string postdata = std::string()
		+ "-----------------------------41184676334\r\n"
		+ "Content-Disposition: form-data; name=\"anywords\"\r\n\r\n"
		+ searchstr + "\r\n"
		+ "-----------------------------41184676334\r\n"
		;*/
	auto p_hdr_html = url_get("https://eprint.iacr.org/eprint-bin/search.pl", postdata);
	auto& html = p_hdr_html.second;

	html.erase(html.begin(), sa::ifind(html,"<body"));
	while (sa::ifind(html,"<a href=") != html.end())
	{
		// find all href strings
		html.erase(html.begin(), sa::ifind(html, "<a href="));
		html.erase(0, html.find_first_of("'\"")+1);
		std::string href = html.substr(0, html.find_first_of("'\""));
		// check if href string is of correct form /year/paper where year and paper are numerics
		if (href.empty() || href[0] != '/') continue;
		href.erase(0, 1);
		if (href.find('/') == std::string::npos) continue;
		std::string year = href.substr(0, href.find('/'));
		std::string paper = href.substr(href.find('/')+1);
		if (year.find_first_not_of("0123456789") != std::string::npos)
			continue;
		if (paper.find_first_not_of("0123456789") != std::string::npos)
			continue;
		cits.push_back("cryptoeprint:" + year + ":" + paper);
		std::cout << "Found citations: " << cits.back() << std::endl;
	}
	if (cits.size() == 0)
		return false;
	if (cits.size() > 5)
		cits.resize(5);
	texfiles_replace_key(citkey, cits);
	return true;
}

#endif
