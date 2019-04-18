//          Copyright Marc Stevens 2010 - 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef DBLPBIBTEX_NETWORK_HPP
#define DBLPBIBTEX_NETWORK_HPP

#include "core.hpp"

extern "C"
{
#include <curl/curl.h>
}

#include <string>
#include <iostream>

size_t _curl_write_callback(char* ptr, size_t size, size_t nmemb, void* _data)
{
	std::string& data = *static_cast<std::string*>(_data);
	data.append(ptr, size * nmemb);
	return size * nmemb;
}
size_t _curl_header_callback(char* ptr, size_t size, size_t nitems, void* _data)
{
	std::string& data = *static_cast<std::string*>(_data);
	data.append(ptr, size * nitems);
	return size * nitems;
}

// usage: url_get(url [,postdata]) with e.g. url="https://dblp.org/"
// returns: pair<string,string> containing (header string, data string)
// on error it returns empty header string and data string and outputs error message to cerr
class url_get_t {
public:
	url_get_t()
		: _curl(nullptr), _havesuccess(false)
	{
	}
	~url_get_t()
	{
		if (_curl == nullptr)
			return;
		curl_global_cleanup();
	}

	// TODO 1: detect protocol and hostname to reuse connections
	// TODO 2: detect internet connection loss and stop trying
	std::pair<std::string,std::string> operator()(const std::string& url, const std::vector<std::pair<std::string,std::string>>& postdata = std::vector<std::pair<std::string,std::string>>())
	{
		std::string _header, _data;

		// initialize
		_init();
		_curl = curl_easy_init();
		curl_mime* mime = nullptr;
		curl_mimepart* part = nullptr;

#ifdef GET_URL_DEBUG
		curl_easy_setopt(_curl, CURLOPT_VERBOSE, 1L);
#endif
		// set up request
		curl_easy_setopt(_curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(_curl, CURLOPT_HEADERFUNCTION, _curl_header_callback);
		curl_easy_setopt(_curl, CURLOPT_HEADERDATA, &_header);
		curl_easy_setopt(_curl, CURLOPT_WRITEFUNCTION, _curl_write_callback);
		curl_easy_setopt(_curl, CURLOPT_WRITEDATA, &_data);

		// process postdata fields
		if (!postdata.empty())
		{
			mime = curl_mime_init(_curl);
			for (auto& pd : postdata)
			{
#ifdef GET_URL_DEBUG
				std::cerr << "POSTDATA:" << pd.first << ":" << pd.second << std::endl;
#endif
				part = curl_mime_addpart(mime);
				curl_mime_type(part, "multipart/form-data");
				curl_mime_name(part, pd.first.c_str());
				curl_mime_data(part, pd.second.c_str(), pd.second.size());
			}
			part = curl_mime_addpart(mime);
			curl_easy_setopt(_curl, CURLOPT_MIMEPOST, mime);
		}

		// execute request and return results
		CURLcode _res = curl_easy_perform(_curl);
		curl_easy_cleanup(_curl);
		if (mime != nullptr)
			curl_mime_free(mime);
			
#ifdef GET_URL_DEBUG
		std::cerr
			<< "url_get::url=" << url << std::endl
			<< "url_get::_res=" << _res << std::endl
			<< "url_get::_header=:" << std::endl << _header << std::endl
			<< "url_get::_data=:" << std::endl << _data << std::endl
			<< "url_get::end" << std::endl;
#endif

		if (_res != CURLE_OK)
		{
			std::cerr << "Error in retrieving URL '" << url << "':" << std::endl << curl_easy_strerror(_res) << std::endl;
			_header.clear();
			_data.clear();
		}
		_havesuccess = true;
		return std::pair<std::string,std::string>(_header,_data);
	}

	bool _havesuccess;
private:
	void _init()
	{
		if (_curl == nullptr)
			return;
		curl_global_init(CURL_GLOBAL_DEFAULT);
	}

	CURL* _curl;
};
url_get_t url_get;


/*** check for new version ***/
void check_new_version()
{
	std::string html = url_get("https://raw.githubusercontent.com/cr-marcstevens/dblpbibtex/master/version").second;
	std::string version = html.substr(html.find("VERSION:")+8);
	html.erase(html.find_first_not_of(".0123456789"));
	if (version == "")
		std::cout << "New version check failed!" << std::endl;
	else if (version != VERSION)
		std::cout << "Warning--New version " << version << " is available on:" << std::endl 
			<< "\thttps://github.com/cr-marcstevens/dblpbibtex/" << std::endl;
}

#endif // DBLPBIBTEX_NETWORK_HPP
