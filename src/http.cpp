
#include "http.hpp"
#include "utils.hpp"
#include "log.hpp"
#include <curl/curl.h>
#include <stdio.h>
#include <chrono>


namespace gdpm::http{

	string url_escape(const string &url){
		CURL *curl = nullptr;
		curl_global_init(CURL_GLOBAL_ALL);
		char *escaped_url = curl_easy_escape(curl, url.c_str(), url.size());
		std::string url_copy = escaped_url;
		curl_global_cleanup();
		return escaped_url;
	}

	response request_get(
		const string& url, 
		const http::request_params& params
	){
		CURL *curl = nullptr;
		CURLcode res;
		utils::memory_buffer buf = utils::make_buffer();
		response r;

#if (GDPM_DELAY_HTTP_REQUESTS == 1)
		using namespace std::chrono_literals;
		utils::delay();
#endif

		curl_global_init(CURL_GLOBAL_ALL);
		curl = curl_easy_init();
		if(curl){
			curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
			// curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "name=daniel&project=curl");
			curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "GET");
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&buf);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, utils::curl_write_to_buffer);
			curl_easy_setopt(curl, CURLOPT_USERAGENT, constants::UserAgent.c_str());
			curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, params.timeout);
			res = curl_easy_perform(curl);
			curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &r.code);
			if(res != CURLE_OK && params.verbose > 0)
				log::error("_make_request.curl_easy_perform(): {}", curl_easy_strerror(res));
			curl_easy_cleanup(curl);
		}

		r.body = buf.addr;
		utils::free_buffer(buf);
		curl_global_cleanup();
		return r;
	}


	response request_post(
		const string& url, 
		const http::request_params& params
	){
		CURL *curl = nullptr;
		CURLcode res;
		utils::memory_buffer buf = utils::make_buffer();
		response r;

#if (GDPM_DELAY_HTTP_REQUESTS == 1)
		using namespace std::chrono_literals;
		utils::delay();
#endif
		string h;
		std::for_each(
			params.headers.begin(),
			params.headers.end(),
			[&h](const string_pair& kv){
				h += kv.first + "=" + kv.second + "&";
			}
		);
		h.pop_back();
		h = url_escape(h);

		// const char *post_fields = "";
		curl_global_init(CURL_GLOBAL_ALL);
		curl = curl_easy_init();
		if(curl){
			curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
			// curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "name=daniel&project=curl");
			curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, h.size());
			curl_easy_setopt(curl, CURLOPT_POSTFIELDS, h.c_str());
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&buf);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, utils::curl_write_to_buffer);
			curl_easy_setopt(curl, CURLOPT_USERAGENT, constants::UserAgent.c_str());
			curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, params.timeout);
			res = curl_easy_perform(curl);
			curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &r.code);
			if(res != CURLE_OK && params.verbose > 0)
				log::error("_make_request.curl_easy_perform(): {}", curl_easy_strerror(res));
			curl_easy_cleanup(curl);
		}

		r.body = buf.addr;
		utils::free_buffer(buf);
		curl_global_cleanup();
		return r;
	}


	response download_file(
		const string& url, 
		const string& storage_path, 
		const http::request_params& params
	){
		CURL *curl = nullptr;
		CURLcode res;
		response r;
		FILE *fp;

#if (GDPM_DELAY_HTTP_REQUESTS == 1)
		using namespace std::chrono_literals;
		utils::delay();
#endif

		curl_global_init(CURL_GLOBAL_ALL);
		curl = curl_easy_init();
		if(curl){
			fp = fopen(storage_path.c_str(), "wb");
			// if(!config.username.empty() && !config.password.empty()){
			// 	std::string curlopt_userpwd{config.username + ":" + config.password};
			// 	curl_easy_setopt(curl, CURLOPT_USERPWD, curlopt_userpwd.c_str());
			// }

			// /* Switch on full protocol/debug output while testing and disable
			// * progress meter by setting to 0L */
			// if(config.verbose){
			// 	curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
			// 	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
			// }
			curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
			// curl_easy_setopt(curl, CURLOPT_USERPWD, "user:pass");
			curl_easy_setopt(curl, CURLOPT_FAILONERROR, true);
			curl_easy_setopt(curl, CURLOPT_HEADER, 0);
			curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, true);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, utils::curl_write_to_stream);
			curl_easy_setopt(curl, CURLOPT_USERAGENT, constants::UserAgent.c_str());
			curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, params.timeout);
			res = curl_easy_perform(curl);
			curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &r.code);
			if(res != CURLE_OK && params.verbose > 0){
				log::error("download_file.curl_easy_perform() failed: {}", curl_easy_strerror(res));
			}
			fclose(fp);
		}
		curl_global_cleanup();
		return r;
	}
}