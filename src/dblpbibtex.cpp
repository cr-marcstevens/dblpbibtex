//          Copyright Marc Stevens 2010 - 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#define DBLPBIBTEX_CATCH_EXCEPTIONS
//#define GET_URL_DEBUG

#include "core.hpp"
#include "network.hpp"
#include "bib_search.hpp"
#include "bib_get.hpp"

#include <contrib/program_options.hpp>
namespace po = program_options;

#include <contrib/string_algo.hpp>
namespace sa = string_algo;

#include <iostream>
#include <fstream>
#include <cstdio>
#include <locale>

#include <string>
#include <vector>
#include <set>
#include <map>

using namespace std;

parameters_type params;

/*** parse bibfiles ***/
string::size_type bibfilestr_searchentry(const string& bibstr, string::size_type offset)
{
	while (true) {
		string::size_type pos = bibstr.find('@', offset);
		if (pos == string::npos) return string::npos;
		string::size_type pos2 = bibstr.find_first_of("{(", pos);
		if (pos2 == string::npos) return string::npos;
		string cittype = sa::trim_copy(sa::to_lower_copy(bibstr.substr(pos+1, pos2-pos-1)));
		if (cittype == "article" || cittype == "book" || cittype == "booklet"
		    || cittype == "inbook" || cittype == "incollection" || cittype == "inproceedings"
		    || cittype == "manual" || cittype == "mastersthesis" || cittype == "misc"
		    || cittype == "phdthesis" || cittype == "proceedings" || cittype == "techreport"
		    || cittype == "unpublished")
			return pos;
		offset = pos+1;
	}
}
void parse_bibfile(const string& bibfile, bool verbose = true)
{
	if (mainbibfile.empty())
		mainbibfile = bibfile;
	if (verbose)
		cout << "Parsing bibfile: '" << bibfile << "'" << endl;
	string bibstr;
	read_file(bibfile, bibstr);

	/* find all cross references */
	string::size_type pos = sa::to_lower_copy(bibstr).find("crossref");
	while (pos  < bibstr.length()) {
		string tmp = sa::trim_copy(bibstr.substr(pos + 8));
		if (tmp.length() == 0 || tmp[0] != '=') {
			pos = sa::to_lower_copy(bibstr).find("crossref", pos+1);
			continue;
		}
		tmp.erase(tmp.find(','));
		tmp = sa::trim_copy(tmp, " =,{}'\"");
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
		string entry = sa::trim_copy(bibstr.substr(pos, (pos2==string::npos ? pos2 : pos2-pos) ));
		string key = entry.substr(entry.find_first_of("{(")+1);
		sa::to_lower(entry);
		key.erase(key.find(','));
		sa::trim(key);
		string lowerkey = sa::to_lower_copy(key); // case-insensitive cite-key
		if (!key.empty()) {
			tosearchcitations_complete[key] = entry;
			havecitations.insert(lowerkey);
			if (verbose) {
				string cittype = sa::trim_copy(entry.substr(1, bibstr.find_first_of("{(")-1));
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
		ifs.close();
		bool hasparsed = false;
		for (unsigned j = 0; j < includedirs.size(); ++j) {
			ifs.open((includedirs[j] + "/" + bibfiles[i]).c_str());
			if (ifs) {
				ifs.close();
				parse_bibfile(includedirs[j] + "/" + bibfiles[i], verbose);
				hasparsed = true;
				break;
			}
			ifs.close();
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
	cout << "DBLPBibTeX - version " << VERSION << " - Copyright Marc Stevens 2010-2019" << endl
		 << "Projectpage: https://github.com/cr-marcstevens/dblpbibtex/" << endl;
	params.enablesearch = false;
	std::string dblpformat;

	/* set default options for configuration file, override with environment variables*/
	string BIBTEX = getenvvar("BIBTEXORG");
	if (BIBTEX.empty())
		BIBTEX = "bibtex";
	/* load configuration file from either ./, $HOME or %APPDATA% in that order */

	po::options_description desc("Allowed options", 98);
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
		("dblpformat"
			, po::value<string>(&dblpformat)->default_value("standard")
			, "Choose DBLP bib format: 'compact'/'0', 'standard'/'1', 'crossref'/'2'")
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
	string HOME = getenvvar("HOME");
	if (HOME.empty())
		HOME = getenvvar("APPDATA");
	switch (0) {
	case 0: default:
		std::ifstream ifs("dblpbibtex.cfg");
		if (ifs) {
			cout << "Loading configuration from: dblpbibtex.cfg" << endl;
			po::store(po::parse_config_file(ifs, desc), vm);
			break;
		}
		if (!HOME.empty()) {
			string filename = (fs::path(HOME) / "dblpbibtex.cfg").string();
			ifs.open(filename.c_str());
			if (ifs) {
				cout << "Loading configuration from: '" << filename << "'" << endl;
				po::store(po::parse_config_file(ifs, desc), vm);
				break;
			}
		}
		cout << "Cannot find configuration file 'dblpbibtex.cfg' in directories './' and '" << HOME << "/'." << endl;
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
			return system((params.bibtexcmd + " --help").c_str());
		}
		if (cmdlparams.back()[0] != '-') {
			if (sa::ends_with(cmdlparams.back(), ".aux"))
				auxfile = cmdlparams.back();
			else
				auxfile = cmdlparams.back() + ".aux";
		}
		if (sa::starts_with(cmdlparams.back(), "--include-directory="))
			includedirs.push_back(cmdlparams.back().substr(string("--include-directory=").length()));
		if (sa::starts_with(cmdlparams.back(), "-include-directory="))
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
			sa::trim(auxline);
			//cout << auxline << endl;
			if (sa::starts_with(auxline, "\\citation{")) {
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
					if (sa::istarts_with(citation, "dblpbibtex:")) {
						/* parse options from aux file */
						if (sa::istarts_with(citation, "dblpbibtex:nodownload"))
							params.nodownload = true;
						if (sa::istarts_with(citation, "dblpbibtex:nodblp"))
							params.nodblp = true;
						if (sa::istarts_with(citation, "dblpbibtex:dblpformat:"))
							dblpformat = citation.substr( string("dblpbibtex:dblpformat:").length() );
						if (sa::istarts_with(citation, "dblpbibtex:nocryptoeprint"))
							params.nocryptoeprint = true;
						if (sa::istarts_with(citation, "dblpbibtex:mainbibfile:")) {
							params.mainbibfile = citation.substr( string("dblpbibtex:mainbibfile:").length() );
							if (params.mainbibfile.find_last_of('.') == string::npos)
								params.mainbibfile += ".bib";
							else if (params.mainbibfile.substr(params.mainbibfile.find_last_of('.')) != ".bib")
								params.mainbibfile += ".bib";
						}
						if (sa::istarts_with(citation, "dblpbibtex:bibtex:"))
							params.bibtexcmd = citation.substr( string("dblpbibtex:bibtex:").length() );
						if (sa::istarts_with(citation, "dblpbibtex:enablesearch"))
							params.enablesearch = true;
						if (sa::istarts_with(citation, "dblpbibtex:cleanupmainbibfile"))
							params.cleanupmainbib = true;
						if (sa::istarts_with(citation, "dblpbibtex:addbibtexoption:"))
							bibtexargs.insert(0, " \"" + citation.substr( string("dblpbibtex:addbibtexoption:").length() ) + "\" ");
						continue;
					}
					if (!citation.empty())
						citations.insert(citation);
				}
			} else if (sa::starts_with(auxline, "\\bibdata{")) {
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
			} else if (sa::starts_with(auxline, "\\@input{") || sa::starts_with(auxline, "\\input{")
					|| sa::starts_with(auxline, "\\@include{") || sa::starts_with(auxline, "\\include{")) {
				string auxfile3 = auxline.substr(auxline.find_first_of("{")+1);
				auxfile3.erase(auxfile3.find_first_of("}"));
				auxfiles.insert(auxfile3);
			}
		}
	}

	// determine DBLP format
	params.dblpformat = 1; // standard format
	sa::to_lower(dblpformat);
	if (dblpformat == "0" || dblpformat == "compact")
		params.dblpformat = 0;
	if (dblpformat == "2" || dblpformat == "crossref")
		params.dblpformat = 2;

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
	else
		cout << "\tDBLP format: " << dblpformat_name(params.dblpformat) << endl;
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
			if (havecitations.find(sa::to_lower_copy(*cit)) == havecitations.end()) {
				if (checkedcitations.find(sa::to_lower_copy(*cit)) != checkedcitations.end())
					continue;
				checkedcitations.insert(sa::to_lower_copy(*cit));
				cout << "New citation: '" << *cit << "'" << endl;
				if (download_citation(*cit))
					downloadedcitations.insert(*cit);
			}
		for (set<string>::const_iterator cit = havecitreferences.begin(); cit != havecitreferences.end(); ++cit)
			if (havecitations.find(sa::to_lower_copy(*cit)) == havecitations.end()) {
				cout << "(NEW) crossref: '" << *cit << "'" << endl;
				if (checkedcitations.find(sa::to_lower_copy(*cit)) != checkedcitations.end())
					continue;
				checkedcitations.insert(sa::to_lower_copy(*cit));
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
			string cittype = sa::trim_copy(sa::to_lower_copy(bibstr.substr(1, bibstr.find_first_of("{(")-1)));
			if (cittype == "article" || cittype == "book" || cittype == "booklet" || cittype == "inbook"
				|| cittype == "incollection" || cittype == "inproceedings" || cittype == "manual"
				|| cittype == "mastersthesis" || cittype == "misc" || cittype == "phdthesis"
				|| cittype == "proceedings" || cittype == "techreport" || cittype == "unpublished")
				mainbibentryoffsets.push_back(pos);
		}
		bool changed = false;
		for (unsigned i = 0; i < mainbibentryoffsets.size(); ++i) {
			string::size_type pos = mainbibentryoffsets[i], pos2 = mainbibfilecontent.length();
			if (i+1 < mainbibentryoffsets.size())
				pos2 = mainbibentryoffsets[i+1];
			string bibstr = mainbibfilecontent.substr(pos, pos2-pos);
			string cittype = sa::trim_copy(sa::to_lower_copy(bibstr.substr(0, bibstr.find_first_of("{("))));
			bibstr.erase(0, bibstr.find_first_of("{(")+1);
			string key = sa::trim_copy(bibstr.substr(0, bibstr.find(',')));
			if (needed_bib_entries.find(key) != needed_bib_entries.end())
				mainbibentries.push_back( pair<string,string>(key, sa::trim_copy(mainbibfilecontent.substr(pos, pos2-pos), " \r\n")) );
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
	if (!params.nonewversioncheck && url_get._havesuccess)
		check_new_version();
} catch (std::exception& e) {
	cerr << "Caught exception while checking new version: " << e.what() << endl;
}

	/* Run bibtex */
	cout << "Running bibtex: '" << params.bibtexcmd + " " + bibtexargs << "'." << endl;
	return system((params.bibtexcmd + " " + bibtexargs).c_str());
}
