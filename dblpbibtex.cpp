//          Copyright Marc Stevens 2010 - 2011.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <fstream>
#include <cstdio>
#include <vector>
#include <set>
#include <map>
#include <string>
#include <locale>

#include <boost/program_options.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>

namespace po = boost::program_options;
using namespace std;

#define VERSION "1.10"

#define DBLPBIBTEX_CATCH_EXCEPTIONS

/*** global variables ***/
string bibtexargs; /* from bibtex command line */
string auxfile; /* from bibtex command line */
vector<string> includedirs; /* from bibtex command line */
vector<string> bibfiles; /* from auxfile */
set<string> citations; /* from auxfile */
vector<string> citreferences; /* parsed from citations from bibfiles or downloaded */

set<string> havecitations; /* parsed from bibfiles: ALL LOWER CASE !!! */
map<string, string> tosearchcitations_complete; /* parsed from to search bibfiles */
set<string> havecitreferences; /* parsed from bibfiles */
string mainbibfile; /* first bibfile encountered will be used to insert downloaded citations and references */
string mainbibfilecontent; /* content from main bibfile to insert downloaded citations and references */
set<string> checkedcitations; /* only download citation once per run to prevent mistakes */

/* parameters values loaded from: environment variables, configuration file and auxfile parameters (overwritten in that order)*/
struct parameters_type {
	string bibtexcmd;
	string mainbibfile; /* this value is later stored in the global mainbibfile */
	vector<string> bibtexaddargs;
	bool nonewversioncheck;
	bool nodownload;
	bool nodblp;
	bool nocryptoeprint;
	bool enablesearch; /* can only be enabled inside tex file with \nocite{dblpbibtex:enablesearch} */
	bool cleanupmainbib; /* can only be enable inside tex file with \nocite{dblpbibtex:cleanupmainbib} */
};
parameters_type params;	

/*** small utility functions ***/
string to_lower(const string& str) {
	string tmp = str;
	locale loc;
	for (string::iterator it = tmp.begin(); it != tmp.end(); ++it)
		*it = tolower(*it, loc);
	return tmp;
}
string trim(const string& str, const string& trimchars = " ") {
	string tmp;
	string::size_type pos = str.find_first_not_of(trimchars);
	if (pos != string::npos)
		tmp = str.substr(str.find_first_not_of(trimchars));
	pos = tmp.find_last_not_of(trimchars);
	if (pos != string::npos)
		tmp.erase(pos+1);
	return tmp;
}
bool has_prefix(const string& tosrch, const string& prefix)
{
	if (prefix.length() > tosrch.length()) return false;
	return tosrch.substr(0, prefix.length()) == prefix;
}
bool has_suffix(const string& tosrch, const string& suffix)
{
	if (suffix.length() > tosrch.length()) return false;
	return tosrch.substr(tosrch.length()-suffix.length()) == suffix;
}
string read_istream(std::istream& is) {
	string tmp;
	char buffer[1024];
	while (is) {
		is.read(buffer, 1024);
		tmp += string(buffer, buffer + is.gcount());
	}
	return tmp;
}
string wget_http(const string& host, const string& page, const string& postdata = "") {
	boost::asio::ip::tcp::iostream stream(host.c_str(), "http");
	if (!stream) { cout << " - wget failed!" << endl; return string(); }
	if (postdata.empty()) {
		stream 
			<< "GET " << page << " HTTP/1.0\r\n" 
			<< "Host: " << host << "\r\n\r\n" << std::flush;
	} else {
		stream
			<< "POST " << page << " HTTP/1.0\r\n"
			<< "Host: " << host << "\r\n"
			<< "Content-Length: " << postdata.length() << "\r\n"
			<< "Content-Type: multipart/form-data; boundary=---------------------------41184676334\r\n"
			<< "Content-Type: application/x-www-form-urlencoded\r\n\r\n"
			<< postdata << "\r\n\r\n" << std::flush;
	}
	return read_istream(stream);
}
string getenvvar(const string& key) {
	char* str = getenv(key.c_str());
	if (str == 0)
		return string();
	return string(str);
}

/*** check for new version ***/
void check_new_version()
{
	string html = wget_http("ro-svn.marc-stevens.nl", "/dblpbibtex/version");
	string version = html.substr(html.find("VERSION:")).substr(8,4);
	if (version == "")
		cout << "New version check failed!" << endl;
	else if (version != VERSION)
		cout << "Warning--New version " << version << " is available on:" << endl << "\thttp://marc-stevens.nl/dblpbibtex/" << endl;
}


/*** main bibfile operations: load, save, add entry ***/
bool load_mainbibfile()
{
	if (mainbibfile.empty()) return false;
	std::ifstream ifs(mainbibfile.c_str());
	if (!ifs) return false;
	mainbibfilecontent = read_istream(ifs);
	return true;
}
bool save_mainbibfile()
{
	if  (mainbibfile.empty()) return false;
	try {
		if (boost::filesystem::exists(mainbibfile + ".bak"))
			boost::filesystem::remove(mainbibfile + ".bak");
		if (boost::filesystem::exists(mainbibfile))
			boost::filesystem::copy_file(mainbibfile, mainbibfile + ".bak");
//			boost::filesystem::rename(mainbibfile, mainbibfile + ".bak");
	} catch (std::exception& e) { cerr << e.what() << endl; return false; } catch (...) { return false; }
	std::ofstream ofs(mainbibfile.c_str());
	if (!ofs) return false;
	ofs << mainbibfilecontent << endl;
	if (!ofs) return false;
	return true;
}
bool add_entry_to_mainbibfile(const string& entry, bool prepend = true)
{
	if (prepend)
		mainbibfilecontent = entry + "\n" + mainbibfilecontent;
	else
		mainbibfilecontent += entry + "\n";
	return true;
}

/*** search citations ***/
/* These functions will replace the searchcommand with top 5 found citations
   !for all the .tex files in the same directory! */
void texfile_replace_key(const string& filename, const string& key, const string& cits)
{
	string content;
	{
		std::ifstream ifs(filename.c_str());
		content = read_istream(ifs);
	}
	bool changed = false;
	while (content.find(key) != string::npos) {
		string::size_type pos = content.find(key);
		content.erase(pos, key.length());
		content.insert(pos, cits);
		changed = true;
	}
	if (changed) {
		cout << "Replaced '" << key << "' with '" << cits << "' in tex file: '" << filename << "'" << endl;
		try {
			if (boost::filesystem::exists(filename + ".bak"))
				boost::filesystem::remove(filename + ".bak");
			//boost::filesystem::rename(filename, filename + ".bak");
			boost::filesystem::copy_file(filename, filename + ".bak");
		} catch (std::exception& e) { cerr << e.what() << endl; return; } catch (...) { return; }
		std::ofstream ofs(filename.c_str());
		if (!ofs)
			return;
		ofs << content << std::flush;
	}
}
void texfiles_replace_key(const string& key, const vector<string>& cits)
{
	if (cits.empty()) return;
	string citsstr = cits.front();
	for (unsigned i = 1; i < cits.size(); ++i)
		citsstr += "," + cits[i];
	boost::filesystem::directory_iterator it(".");
	for (; it != boost::filesystem::directory_iterator(); ++it) {
		if (has_suffix(it->path().string(), ".tex"))
			texfile_replace_key(it->path().string(), key, citsstr);
	}
}
/* search functions */
bool search_citation_bib(const string& key)
{
	string searchstr = to_lower(key.substr(key.find(':')+1));
	vector<string> keywords;
	while (searchstr.find('+') != string::npos) {
		string keyword = trim(searchstr.substr(0,searchstr.find('+')));
		if (!keyword.empty()) keywords.push_back(keyword);
		searchstr.erase(0, searchstr.find('+')+1);
	}
	string keyword = trim(searchstr);
	if (!keyword.empty()) keywords.push_back(keyword);

	vector<string> cits;
	for (map<string,string>::const_iterator cit = tosearchcitations_complete.begin(); cit != tosearchcitations_complete.end(); ++cit)
	{
		bool ok = true;
		for (unsigned i = 0; i < keywords.size(); ++i)
			if (cit->second.find(keywords[i]) == string::npos) {
				ok = false;
				break;
			}
		if (ok) 
			cits.push_back(cit->first);
	}
	if (cits.size() == 0) return false;
	if (cits.size() > 5) cits.resize(5);
	texfiles_replace_key(key, cits);
	return true;
}
bool search_citation_dblp(const string& key)
{
	string searchphrase = key.substr(key.find(':')+1);
	if (searchphrase.length() < 5) {
		cout << "Search phrase too short <5 chars: '" << searchphrase << "'" << endl;
		return false;
	}
	string html = wget_http("dblp.org", "/search/?q=" + searchphrase);
	html.erase(0, to_lower(html).find("<body"));
	set<string> cits2;
	while (html.find("/rec/bibtex/") != string::npos)
	{
		string cit = html.substr(html.find("/rec/bibtex/"));
		cit.erase(to_lower(cit).find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789:/.+-$&@=!?"));
		cit.erase(0, string("/rec/bibtex/").length());
		cit = "DBLP:" + cit;
		cits2.insert(cit);
		html.erase(0, html.find("/rec/bibtex/")+1);
		cout << "Found citations: " << cit << endl;
	}
	vector<string> cits(cits2.begin(), cits2.end());
	if (cits.size() == 0) return false;
	if (cits.size() > 5) cits.resize(5);
	texfiles_replace_key(key, cits);
	return true;
}
bool search_citation_cryptoeprint(const string& key)
{
	vector<string> cits;
	string searchstr = to_lower(key.substr(key.find(':')+1));
	while (searchstr.find('+') != string::npos) {
		searchstr.insert( searchstr.find('+'), " ");
		searchstr.erase( searchstr.find('+'), 1);		
	}
	if (searchstr.length() < 5) {
		cout << "Search phrase too short <5 chars: '" << searchstr << "'" << endl;
		return false;
	}
	string postdata = string()
		+ "-----------------------------41184676334\r\n"
		+ "Content-Disposition: form-data; name=\"anywords\"\r\n\r\n"
		+ searchstr + "\r\n"
		+ "-----------------------------41184676334\r\n"
		;
	string html = wget_http("eprint.iacr.org", "/eprint-bin/search.pl", postdata);
	html.erase(0, to_lower(html).find("<body"));
	while (to_lower(html).find("<a href=") != string::npos) {
		// find all href strings
		html.erase(0, to_lower(html).find("<a href="));
		html.erase(0, html.find_first_of("'\"")+1);
		string href = html.substr(0, html.find_first_of("'\""));
		// check if href string is of correct form /year/paper where year and paper are numerics
		if (href.empty() || href[0] != '/') continue;
		href.erase(0, 1);
		if (href.find('/') == string::npos) continue;
		string year = href.substr(0, href.find('/'));
		string paper = href.substr(href.find('/')+1);
		if (year.find_first_not_of("0123456789") != string::npos) continue;
		if (paper.find_first_not_of("0123456789") != string::npos) continue;
		cits.push_back("cryptoeprint:" + year + ":" + paper);
		cout << "Found citations: " << cits.back() << endl;
	}
	if (cits.size() == 0) return false;
	if (cits.size() > 5) cits.resize(5);
	texfiles_replace_key(key, cits);
	return true;
}

/*** download citations ***/
bool download_dblp_citation(const string& key, bool prepend = true)
{
	string html = wget_http("dblp.org", "/rec/bib2/" + key.substr(5));
	if (html.empty()) return false;
	if (html.find("301 Moved Permanently") != string::npos)
		throw std::runtime_error("DBLP website has moved, inform dblpbibtex maintainer");
	if (html.find("404 Not Found") != string::npos)
	{
		cout << "DBLP key " << key << " does not exist on the server!" << endl;
		return false;
	}
	if (html.find("200 OK") == string::npos)
		throw std::runtime_error("DBLP website error, check if dblp.org is online, otherwise inform dblpbibtex maintainer");
/*
	size_t pos = to_lower(html).find("<pre"); pos = html.find(">", pos);
	html.erase(0, pos+1);
	html.erase(to_lower(html).find("</pre>"));
	pos = to_lower(html).find("<a ");
	if (pos < html.size())
	{
		html.erase(pos, html.find(">", pos)-pos+1);
		html.erase(to_lower(html).find("</a>"), 4);
	}
*/
	if (html.empty()) return false;
	cout << "Downloaded bibtex entry:" << endl << html << endl;
	return add_entry_to_mainbibfile(html, prepend);
}
bool download_cryptoeprint_citation(const string& key, bool prepend = true)
{
	string year = string(key.begin()+key.find(':')+1, key.begin()+key.find_last_of(':'));
	string paper = key.substr(key.find_last_of(':')+1);
	string html = wget_http("eprint.iacr.org", "/eprint-bin/cite.pl?entry=" + year + "/" + paper);
	if (html.empty()) return false;
	html.erase(0, to_lower(html).find("<pre>")+5);
	html.erase(to_lower(html).find("</pre>"));
//	string::size_type pos = to_lower(html).find("http://eprint.iacr.org/");
//	if (pos != string::npos) {
//		html.erase(pos, string("http://eprint.iacr.org/").length());
//		html.insert(pos, "http://eprint.iacr.org/" + year + "/" + paper);
//	}
	if (html.empty()) return false;
	cout << "Downloaded bibtex entry:" << endl << html << endl;
	return add_entry_to_mainbibfile(html, prepend);
}
bool download_citation(const string& key, bool prepend = true)
{
	if (mainbibfile.length() == 0) {
		cout << "No main bib file found!" << endl;
		return false;
	}
	if (!params.nodblp && has_prefix(to_lower(key), "dblp:"))
		return download_dblp_citation(key, prepend);
	if (!params.nocryptoeprint && has_prefix(to_lower(key), "cryptoeprint:"))
		return download_cryptoeprint_citation(to_lower(key), prepend);
	if (params.enablesearch && has_prefix(to_lower(key), "search:")) {
		if (search_citation_bib(key)) return true;
		if (search_citation_dblp(key)) return true;
		if (search_citation_cryptoeprint(key)) return true;
		return false;
	}
	if (params.enablesearch && has_prefix(to_lower(key), "search-dblp:"))
		return search_citation_dblp(key);
	if (params.enablesearch && has_prefix(to_lower(key), "search-cryptoeprint:"))
		return search_citation_cryptoeprint(key);
	if (params.enablesearch && has_prefix(to_lower(key), "search-bib:"))
		return search_citation_bib(key);
	return false;
}

/*** parse bibfiles ***/
string::size_type bibfilestr_searchentry(const string& bibstr, string::size_type offset)
{
	while (true) {
		string::size_type pos = bibstr.find('@', offset);
		if (pos == string::npos) return string::npos;
		string::size_type pos2 = bibstr.find_first_of("{(", pos);
		if (pos2 == string::npos) return string::npos;
		string cittype = trim(to_lower(bibstr.substr(pos+1, pos2-pos-1)));
		if (cittype == "article" || cittype == "book" || cittype == "booklet" || cittype == "inbook" || cittype == "incollection" || cittype == "inproceedings" || cittype == "manual" || cittype == "mastersthesis" || cittype == "misc" || cittype == "phdthesis" || cittype == "proceedings" || cittype == "techreport" || cittype == "unpublished")
			return pos;
		offset = pos+1;
	}
}
void parse_bibfile(const string& bibfile, bool verbose = true)
{
	if (mainbibfile.length() == 0)
		mainbibfile = bibfile;
	if (verbose)
		cout << "Parsing bibfile: '" << bibfile << "'" << endl;
	string bibstr;
	{
		std::ifstream ifs(bibfile.c_str());
		bibstr = read_istream(ifs);
	}
	/* find all cross references */
	string::size_type pos = to_lower(bibstr).find("crossref");
	while (pos  < bibstr.length()) {
		string tmp = trim(bibstr.substr(pos + 8));
		if (tmp.length() == 0 || tmp[0] != '=') {
			pos = to_lower(bibstr).find("crossref", pos+1);
			continue;
		}		
		tmp.erase(tmp.find(','));
		tmp = trim(tmp, " =,{}'\"");
		if (!tmp.empty()) {
			if (verbose)
				cout << "\t crossref: '" << tmp << "'" << endl;
			havecitreferences.insert(tmp);
		}
		pos = bibstr.find("crossref", pos+1);
	}
	/* find all citations */
	string::size_type pos2;
	pos = bibfilestr_searchentry(bibstr, 0);
	while (pos < bibstr.length()) {
		pos2 = bibfilestr_searchentry(bibstr, pos+1);
		string entry = trim(bibstr.substr(pos, (pos2==string::npos ? pos2 : pos2-pos) ));
		string key = entry.substr(entry.find_first_of("{(")+1);
		entry = to_lower(entry);
		key.erase(key.find(','));
		key = trim(key);
		string lowerkey = to_lower(key); // case-insensitive cite-key
		if (!key.empty()) {
			tosearchcitations_complete[key] = entry;
			havecitations.insert(lowerkey);
			if (verbose) {
				string cittype = trim(entry.substr(1, bibstr.find_first_of("{(")-1));
				cout << "\t " << cittype << ": '" << key << "'" << endl;
			}
		}
		pos = pos2;
	}
}
void parse_bibfiles(bool verbose = true) {
	havecitations.clear();
	havecitreferences.clear();
	tosearchcitations_complete.clear();
	for (unsigned i = 0; i < bibfiles.size(); ++i) {
		std::ifstream ifs(bibfiles[i].c_str());
		if (ifs) {
			ifs.close();
			parse_bibfile(bibfiles[i], verbose);
			continue;
		}
		bool hasparsed = false;
		for (unsigned j = 0; j < includedirs.size(); ++j) {
			ifs.open((includedirs[j] + "/" + bibfiles[i]).c_str());
			if (ifs) {
				ifs.close();
				parse_bibfile(includedirs[j] + "/" + bibfiles[i], verbose);
				hasparsed = true;
				break;
			}
		}
		if (hasparsed) continue;
		cout << "Cannot find bibfile: '" << bibfiles[i] << "' in one of these directories: '.'";
		for (unsigned j = 0; j < includedirs.size(); ++j)
			cout << ", '" << includedirs[j] << "'";
		cout << endl;
	}
}

int main(int argc, char** argv)
{
#ifdef DBLPBIBTEX_CATCH_EXCEPTIONS
try {
#endif
	// needs to be enabled inside tex file with \nocite{dblpbibtex:enablesearch}
	// searches change your tex files, use with care!
	cout << "DBLPBibTeX - version " << VERSION << " - Copyright Marc Stevens 2010-2011" << endl
		 << "Projectpage: http://marc-stevens.nl/dblpbibtex/" << endl;
	params.enablesearch = false; 

	/* set default options for configuration file, override with environment variables*/
	string BIBTEX = getenvvar("BIBTEXORG");
	if (BIBTEX.empty())
		BIBTEX = "bibtex";
	/* load configuration file from either ./, $HOME or %APPDATA% in that order */

	po::options_description desc("Allowed options");
	desc.add_options()
		("bibtex"
			, po::value<string>(&params.bibtexcmd)->default_value(BIBTEX)
			, "Set bibtex command.\n\tEnvironment variable 'BIBTEXORG' overrides default value 'bibtex'")
		("mainbibfile"
			, po::value<string>(&params.mainbibfile)->default_value("")
			, "Set main bib file to insert new citations.\nDefaults to first bib file given in parsed aux file")
		("enablesearch"
			, po::bool_switch(&params.enablesearch)
			, "Allow searches to insert results in .tex files")
		("cleanupmainbibfile"
			, po::bool_switch(&params.cleanupmainbib)
			, "Remove unused bibitems from main .bib file")
		("nodownload"
			, po::bool_switch(&params.nodownload)
			, "Do not download any new citations or crossrefs. Prevent main bib file from changes.")
		("nodblp"
			, po::bool_switch(&params.nodblp)
			, "Do not download new citations or crossrefs from DBLP.")
		("nocryptoeprint"
			, po::bool_switch(&params.nocryptoeprint)
			, "Do not download new citations from IACR crypto eprint.")
		("addbibtexoption"
			, po::value< vector<string> >(&params.bibtexaddargs)
			, "Prepends string to bibtex commandline arguments")
		("nonewversioncheck"
			, po::bool_switch(&params.nonewversioncheck)
			, "Disable check for new version")
	;
	po::variables_map vm;
	switch (0) {
	case 0: default:
		std::ifstream ifs("dblpbibtex.cfg");
		if (ifs) { 
			cout << "Loading configuration from: dblpbibtex.cfg" << endl;
			po::store(po::parse_config_file(ifs, desc), vm);
			break;
		}
		string HOME = getenvvar("HOME");
		if (!HOME.empty()) {
			string filename = (boost::filesystem::path(HOME) / "dblpbibtex.cfg").string();
			ifs.open(filename.c_str());
			if (ifs) { 
				cout << "Loading configuration from: '" << filename << "'" << endl;
				po::store(po::parse_config_file(ifs, desc), vm);
				break;
			}
		}
		string APPDATA = getenvvar("APPDATA");
		if (!APPDATA.empty()) {
			string filename = (boost::filesystem::path(HOME) / "dblpbibtex.cfg").string();
			ifs.open(filename.c_str());
			if (ifs) {
				cout << "Loading configuration from: '" << filename << "'" << endl;
				po::store(po::parse_config_file(ifs, desc), vm);
				break;
			}
		}
		cout << "Cannot find configuration file 'dblpbibtex.cfg' in directories './', '$HOME/' and '%APPDATA%/'." << endl;
		ifs.close();
		po::store(po::parse_config_file(ifs, desc), vm);
	}
	po::notify(vm);
	for (unsigned i = 0; i < params.bibtexaddargs.size(); ++i) {
		if (bibtexargs.length())
			bibtexargs += " ";
		bibtexargs += params.bibtexaddargs[i];
	}

	/* look at command line parameters */
	vector<string> cmdlparams;
	for (int i = 1; i < argc; ++i) {
		cmdlparams.push_back(argv[i]);
		if (cmdlparams.back() == "-h" || cmdlparams.back() == "--help" || cmdlparams.back() == "-help") {
			cout << desc << endl;
			cout << endl << "Running 'bibtex --help':" << endl;
			return system((params.bibtexcmd + "--help").c_str());
		}
		if (cmdlparams.back()[0] != '-') {
			if (has_suffix(cmdlparams.back(), ".aux"))
				auxfile = cmdlparams.back();
			else
				auxfile = cmdlparams.back() + ".aux";
		}
		if (has_prefix(cmdlparams.back(), "--include-directory="))
			includedirs.push_back(cmdlparams.back().substr(string("--include-directory=").length()));
		if (has_prefix(cmdlparams.back(), "-include-directory="))
			includedirs.push_back(cmdlparams.back().substr(string("-include-directory=").length()));
		if (bibtexargs.length()) 
			bibtexargs += " ";
		bibtexargs += "\"" + cmdlparams.back() + "\"";
	}
	

	/* parse auxfile */
	if (auxfile.length() == 0) {
		cout << desc << endl;
		cout << endl << "Running 'bibtex --help':" << endl;
		return system((params.bibtexcmd + " --help").c_str());
	}
	set<string> auxfiles;
	auxfiles.insert(auxfile);
	while (auxfiles.size()) {
		string auxfile2 = *auxfiles.begin();
		auxfiles.erase(auxfile2);
		cout << "Parsing auxfile: '" << auxfile2 << "'." << endl;
		std::ifstream ifs(auxfile2.c_str());
		if (!ifs) {
			cout << "Cannot open auxfile!" << endl;
			continue;
		}
		string auxline;
		while (!!ifs) {
			//cout << "@" << std::flush;
			getline(ifs, auxline);
			auxline = trim(auxline);
			//cout << auxline << endl;
			if (has_prefix(auxline, "\\citation{")) {
				string citationsstr = auxline.substr(auxline.find('{')+1);
				citationsstr.erase( citationsstr.find('}') );
				vector<string> subcitations;
				while (citationsstr.find(',') != string::npos) {
					subcitations.push_back(citationsstr.substr(0, citationsstr.find(',')));
					citationsstr.erase(0, citationsstr.find(',')+1);
				}
				subcitations.push_back(citationsstr);
				for (unsigned i = 0; i < subcitations.size(); ++i) {
					string& citation = subcitations[i];
					if (has_prefix(to_lower(citation), "dblpbibtex:")) {
						/* parse options from aux file */
						if (has_prefix(to_lower(citation), "dblpbibtex:nodownload")) 
							params.nodownload = true;
						if (has_prefix(to_lower(citation), "dblpbibtex:nodblp")) 
							params.nodblp = true;
						if (has_prefix(to_lower(citation), "dblpbibtex:nocryptoeprint"))
							params.nocryptoeprint = true;
						if (has_prefix(to_lower(citation), "dblpbibtex:mainbibfile:")) {
							params.mainbibfile = citation.substr( string("dblpbibtex:mainbibfile:").length() );
							if (params.mainbibfile.find_last_of('.') == string::npos)
								params.mainbibfile += ".bib";
							else if (params.mainbibfile.substr(params.mainbibfile.find_last_of('.')) != ".bib")
								params.mainbibfile += ".bib";							
						}
						if (has_prefix(to_lower(citation), "dblpbibtex:bibtex:"))
							params.bibtexcmd = citation.substr( string("dblpbibtex:bibtex:").length() );
						if (has_prefix(to_lower(citation), "dblpbibtex:enablesearch"))
							params.enablesearch = true;
						if (has_prefix(to_lower(citation), "dblpbibtex:cleanupmainbibfile"))
							params.cleanupmainbib = true;
						if (has_prefix(to_lower(citation), "dblpbibtex:addbibtexoption:"))
							bibtexargs.insert(0, " \"" + citation.substr( string("dblpbibtex:addbibtexoption:").length() ) + "\" ");
						continue;
					}
					if (!citation.empty())
						citations.insert(citation);
				}
			} else if (has_prefix(auxline, "\\bibdata{")) {
				string bibfile = auxline.substr(auxline.find('{')+1);
				bibfile.erase( bibfile.find('}') );
				if (!bibfile.empty()) {
					while (bibfile.find(',') != string::npos) {
						string bibfile2 = bibfile.substr(0, bibfile.find(','));
						bibfile2 += ".bib";
						bibfiles.push_back(bibfile2);
						bibfile.erase(0, bibfile.find(',')+1);
					}
					bibfile += ".bib";
					bibfiles.push_back(bibfile);
				}
			} else if (has_prefix(auxline, "\\@input{") || has_prefix(auxline, "\\input{") || has_prefix(auxline, "\\@include{") || has_prefix(auxline, "\\include{")) {
				string auxfile3 = auxline.substr(auxline.find_first_of("{")+1);
				auxfile3.erase(auxfile3.find_first_of("}"));
				auxfiles.insert(auxfile3);
			}
		}
	}

	cout << "Configuration:" << endl;
	cout << "\tbibtex: '" << params.bibtexcmd << "'" << endl;
	if (params.mainbibfile.empty())
		cout << "\tmainbibfile: defaults to first found bib file" << endl;
	else
		cout << "\tmainbibfile: '" << params.mainbibfile << "'" << endl;
	if (params.nodownload)
		cout << "\tNo downloads of new citations or crossrefs." << endl;
	if (params.nodblp)
		cout << "\tNo downloads from DBLP." << endl;
	if (params.nocryptoeprint)
		cout << "\tNo downloads from cryptoeprint." << endl;

	cout << "Bib files:";
	for (unsigned i = 0; i < bibfiles.size(); ++i)
		cout << " '" << bibfiles[i] << "'";
	cout << endl;
	cout << "Bib search directories: '.'";
	for (unsigned i = 0; i < includedirs.size(); ++i)
		cout << " '" << includedirs[i] << "'";
	cout << endl;

	mainbibfile = params.mainbibfile;
	// force include mainbibfile to prevent mainbibfile to endlessly grow when new citations are not found on next runs
	if (!mainbibfile.empty())
		bibfiles.push_back(mainbibfile);

	set<string> downloadedcitations;
	while (true) {
		parse_bibfiles(false);		

		downloadedcitations.clear();
		
		if (!load_mainbibfile()) {
			cout << "Failed to load main bibfile: '" << mainbibfile << "'!" << endl;
			break;
		}
		cout << "Loaded content of main bibfile: '" << mainbibfile << "'!" << endl;
		for (set<string>::const_iterator cit = citations.begin(); cit != citations.end(); ++cit)
			if (havecitations.find(to_lower(*cit)) == havecitations.end()) {
				if (checkedcitations.find(to_lower(*cit)) != checkedcitations.end()) 
					continue;
				checkedcitations.insert(to_lower(*cit));
				cout << "New citation: '" << *cit << "'" << endl;
				if (download_citation(*cit))
					downloadedcitations.insert(*cit);				
			}
		for (set<string>::const_iterator cit = havecitreferences.begin(); cit != havecitreferences.end(); ++cit)
			if (havecitations.find(to_lower(*cit)) == havecitations.end()) {
				cout << "(NEW) crossref: '" << *cit << "'" << endl;
				if (checkedcitations.find(to_lower(*cit)) != checkedcitations.end()) 
					continue;
				checkedcitations.insert(to_lower(*cit));
				cout << "New crossref: '" << *cit << "'" << endl;
				if (download_citation(*cit, false)) // always add crossrefs at the end
					downloadedcitations.insert(*cit);
			}
		if (downloadedcitations.empty() || params.nodownload) {
			cout << "No updates to save to main bibfile: '" << mainbibfile << "'!" << endl;
			break;
		}
		if (!save_mainbibfile()) {
			cout << "Failed to save main bibfile: '" << mainbibfile << "'!" << endl;
			break;
		}
		cout << "Saved new content of main bibfile: '" << mainbibfile << "'!" << endl;
	}

	/* when enabled in .tex file, remove all obsolete entries from main bib file */
	while (params.cleanupmainbib) {
		if (!load_mainbibfile()) {
			cout << "Failed to load main bibfile: '" << mainbibfile << "'!" << endl;
			break;
		}
		cout << "Loaded content of main bibfile: '" << mainbibfile << "'!" << endl;
		parse_bibfiles(false);
		// merge aux citations and all crossrefs
		set<string> needed_bib_entries = citations;
		needed_bib_entries.insert(havecitreferences.begin(), havecitreferences.end());
		// split mainbibfilecontent into parts
		vector<string::size_type> mainbibentryoffsets;
		vector< pair<string,string> > mainbibentries;
		/* find all citations */
		string::size_type pos_start = 0;
		while (true) {
			string::size_type pos = mainbibfilecontent.find('@', pos_start);
			if (pos == string::npos) break;
			pos_start = pos+1;
			string bibstr = mainbibfilecontent.substr(pos);
			string cittype = trim(to_lower(bibstr.substr(1, bibstr.find_first_of("{(")-1)));
			if (cittype == "article" || cittype == "book" || cittype == "booklet" || cittype == "inbook" || cittype == "incollection" || cittype == "inproceedings" || cittype == "manual" || cittype == "mastersthesis" || cittype == "misc" || cittype == "phdthesis" || cittype == "proceedings" || cittype == "techreport" || cittype == "unpublished")
				mainbibentryoffsets.push_back(pos);
		}
		bool changed = false;
		for (unsigned i = 0; i < mainbibentryoffsets.size(); ++i) {
			string::size_type pos = mainbibentryoffsets[i], pos2 = mainbibfilecontent.length();
			if (i+1 < mainbibentryoffsets.size())
				pos2 = mainbibentryoffsets[i+1];
			string bibstr = mainbibfilecontent.substr(pos, pos2-pos);
			string cittype = trim(to_lower(bibstr.substr(0, bibstr.find_first_of("{("))));
			bibstr.erase(0, bibstr.find_first_of("{(")+1);
			string key = trim(bibstr.substr(0, bibstr.find(',')));
			if (needed_bib_entries.find(key) != needed_bib_entries.end())
				mainbibentries.push_back( pair<string,string>(key, trim(mainbibfilecontent.substr(pos, pos2-pos), " \r\n")) );
			else {
				cout << "Removed entry from main bibfile: '" << key << "'" << endl;
				changed = true;
			}
		}
		mainbibfilecontent.clear();
		for (unsigned i = 0; i < mainbibentries.size(); ++i)
			mainbibfilecontent += mainbibentries[i].second + "\n\n";
		if (!changed) {
			cout << "No clean up changes to save to main bibfile: '" << mainbibfile << "'!" << endl;
			break;
		}
		if (!save_mainbibfile()) {
			cout << "Failed to save main bibfile: '" << mainbibfile << "'!" << endl;
			break;
		}
		cout << "Saved new content of main bibfile: '" << mainbibfile << "'!" << endl;
	}
#ifdef DBLPBIBTEX_CATCH_EXCEPTIONS
} catch (std::exception& e) {
	cerr << "Caught exception: " << e.what() << endl;
}
#endif

try {
	//Check for new version
	if (!params.nonewversioncheck)
		check_new_version();
} catch (std::exception& e) {
	cerr << "Caught exception while checking new version: " << e.what() << endl;
}

	/* Run bibtex */
	cout << "Running bibtex: '" << params.bibtexcmd + " " + bibtexargs << "'." << endl;
	return system((params.bibtexcmd + " " + bibtexargs).c_str());
}
