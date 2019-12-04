//          Copyright Marc Stevens 2010 - 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "core.hpp"
#include "network.hpp"

#include <contrib/string_algo.hpp>
namespace sa = string_algo;

/*** download citations ***/
bool download_dblp_citation(const std::string& key, bool prepend = true)
{
	auto hdr_html = url_get("https://dblp.org/rec/bib" + std::to_string(params.dblpformat) + "/" + key.substr(5));
	auto& html = hdr_html.second;
	if (html.empty())
		return false;

	auto it = sa::find(html, '@');
	if (it == html.end())
		return false;
	auto it2 = sa::find(html, '@', it+1);
	html.erase(it2, html.end());

	if (html.empty())
		return false;
	std::cout << "Downloaded bibtex entry:" << std::endl << html << std::endl;
	add_entry_to_mainbibfile(html, prepend);
	return true;
}

bool download_cryptoeprint_citation(const std::string& key, bool prepend = true)
{
	std::string year = std::string(key.begin()+key.find(':')+1, key.begin()+key.find_last_of(':'));
	std::string paper = key.substr(key.find_last_of(':')+1);
	auto hdr_html = url_get("https://eprint.iacr.org/eprint-bin/cite.pl?entry=" + year + "/" + paper);
	auto& html = hdr_html.second;
	if (html.empty())
		return false;

	html.erase(html.begin(), sa::ifind(html,"<pre>")+5);
	html.erase(sa::ifind(html,"</pre>"), html.end());

	if (html.empty())
		return false;
	std::cout << "Downloaded bibtex entry:" << std::endl << html << std::endl;
	add_entry_to_mainbibfile(html, prepend);
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
