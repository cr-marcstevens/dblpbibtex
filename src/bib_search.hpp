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
	auto p_header_body = url_get("https://dblp.org/search?q=" + searchphrase);
	auto& html = p_header_body.second;

	html.erase(html.begin(), sa::ifind(html,"<body"));
	std::set<std::string> cits2;
	while (html.find("/rec/") != std::string::npos)
	{
		std::string cit = html.substr(html.find("/rec/"));
		cit.erase(sa::to_lower_copy(cit).find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789:/.+-$&@=!?"));
		cit.erase(0, std::string("/rec/").length());
		if (cit.substr(cit.size()-5) == ".html")
		{
			cit.resize(cit.size()-5);
			cit = "DBLP:" + cit;
			cits2.insert(cit);
			std::cout << "Found citations: " << cit << std::endl;
		}
		html.erase(0, html.find("/rec/")+1);
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
    std::cout << "Searching cryptoeprint" << std::endl;
	std::string key = citkey.substr(citkey.find(':')+1);
	std::string searchphrase = key;
	if (searchphrase.length() < 5) {
		std::cout << "Search phrase too short <5 chars: '" << searchphrase << "'" << std::endl;
		return false;
	}
	auto p_header_body = url_get("https://eprint.iacr.org/search?q=" + searchphrase);
    std::cout <<  "https://eprint.iacr.org/search?q=" + searchphrase << std::endl;
	auto& html = p_header_body.second;

	html.erase(html.begin(), sa::ifind(html,"<body"));
	std::set<std::string> cits2;
    std::string denominator = "class=\"paperlink\"";
	while (html.find(denominator) != std::string::npos)
	{
          
        std::string cit = html.substr(html.find(denominator));
        cit.erase(0, cit.find('>')+1);
        cit.erase(cit.find('<'), cit.size()-1);
        cit = "cryptoeprint:" + cit;
        cits2.insert(cit);
        std::cout << "Found citations: " << cit << std::endl;
		html.erase(0, html.find(denominator)+1);

	}
	std::vector<std::string> cits(cits2.begin(), cits2.end());
	if (cits.size() == 0)
		return false;
	if (cits.size() > 5)
		cits.resize(5);
	texfiles_replace_key(citkey, cits);
	return true;
}

#endif
