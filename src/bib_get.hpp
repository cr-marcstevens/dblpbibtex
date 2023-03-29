//          Copyright Marc Stevens 2010 - 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef DBLPBIBTEX_BIB_GET_HPP
#define DBLPBIBTEX_BIB_GET_HPP

#include "core.hpp"
#include "network.hpp"
#include "bib_parse.hpp"

#include <contrib/string_algo.hpp>
namespace sa = string_algo;

/*** download citations ***/
bool download_dblp_citation(const std::string& key, bool prepend = true)
{
	std::string url = "https://dblp.org/rec/" + key.substr(5) + ".bib?param=" + std::to_string(params.dblpformat);
	auto hdr_html = url_get(url);
	auto& html = hdr_html.second;
	if (html.empty())
		return false;

	auto bibentry = extract_bibentry(html);

	if (bibentry.empty())
		return false;
	std::cout << "Downloaded bibtex entry:" << std::endl << bibentry << std::endl;
	add_entry_to_mainbibfile(bibentry, prepend);
	return true;
}

bool download_cryptoeprint_citation(const std::string& key, bool prepend = true)
{
	std::string year = key.substr(key.find_first_of(':')+1, 4);
	std::string paper = key.substr(key.find_last_of(":/")+1);
    std::string url ="https://eprint.iacr.org/" + year + "/" + paper;
    auto hdr_html = url_get(url);
	auto& html = hdr_html.second;
	if (html.empty())
		return false;

    auto bibentry = extract_bibentry(html.substr(html.find("<pre id=\"bibtex\">"), html.find("</pre>")-html.find("<pre id=\"bibtex\">")));

	if (bibentry.empty())
		return false;
	std::cout << "Downloaded bibtex entry:" << std::endl << bibentry << std::endl;
	add_entry_to_mainbibfile(bibentry, prepend);
	return true;
}

bool download_citation(const std::string& key, bool prepend = true)
{
	if (mainbibfile.empty()) {
		std::cout << "No main bib file found!" << std::endl;
		return false;
	}

	// download actual citation from DBLP or IACR ePrint
	if (!params.nodblp && sa::istarts_with(key, "dblp:"))
		return download_dblp_citation(key, prepend);
	if (!params.nocryptoeprint && sa::istarts_with(key, "cryptoeprint:"))
		return download_cryptoeprint_citation(key, prepend);

	// process search citation and replace with results
	if (key.find(':') == std::string::npos)
		return false;
	if (params.enablesearch && sa::istarts_with(key, "search:")) {
		if (search_citation_bib(key))
			return true;
		if (search_citation_dblp(key))
			return true;
		if (search_citation_cryptoeprint(key))
			return true;
		return false;
	}
	if (params.enablesearch && sa::istarts_with(key, "search-dblp:"))
		return search_citation_dblp(key);
	if (params.enablesearch && sa::istarts_with(key, "search-cryptoeprint:"))
		return search_citation_cryptoeprint(key);
	if (params.enablesearch && sa::istarts_with(key, "search-bib:"))
		return search_citation_bib(key);
	return false;
}

#endif
