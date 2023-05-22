#include "config.hpp"
#include "error.hpp"
#include "log.hpp"
#include "utils.hpp"
#include "constants.hpp"
#include "error.hpp"

// RapidJSON
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/rapidjson.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>


// fmt
#include <fmt/format.h>
#include <cxxopts.hpp>
#include <iostream>
#include <string>
#include <string_view>
#include <ostream>
#include <fstream>
#include <ios>
#include <memory>
#include <set>
#include <unordered_map>


namespace gdpm::config{
	context config;
	std::string to_json(const context& params){
		auto _build_json_array = [](std::set<std::string> a){
			std::string o{"["};
			for(const std::string& src : a)
				o += "\"" + src + "\",";
			if(o.back() == ',')
				o.pop_back();
			o += "]";
			return o;
		};

		auto _build_json_object = [](const string_map& m){
			string o{"{"};
			std::for_each(m.begin(), m.end(), [&o](const string_pair& p){
				o += std::format("\n\"{}\": \"{}\",", p.first, p.second);
			});
			if(o.back() == ',')
				o.pop_back();
			o += "}";
			return o;
		};

		/* Build a JSON string to pass to document */
		string json{
			"{\"username\":\"" + params.username + "\","
			+ "\"password\":\"" + params.password + "\","
			+ "\"path\":\"" + params.path + "\","
			+ "\"token\":\"" + params.token + "\","
			+ "\"godot_version\":\"" + params.godot_version + "\","
			+ "\"packages_dir\":\"" + params.packages_dir + "\","
			+ "\"tmp_dir\":\"" + params.tmp_dir + "\","
			+ "\"remote_sources\":" + _build_json_object(params.remote_sources) + ","
			+ "\"threads\":" + fmt::to_string(params.threads) + ","
			+ "\"timeout\":" + fmt::to_string(params.timeout) + ","
			+ "\"enable_sync\":" + fmt::to_string(params.enable_sync) + ","
			+ "\"enable_file_logging\":" + fmt::to_string(params.enable_file_logging)
			+ "}"
		};
		return json;
	}


	error load(
		std::filesystem::path path, 
		context& config, 
		int verbose
	){
		std::fstream file;
		file.open(path, std::ios::in);
		if(!file){
			if(verbose)
				log::info("No configuration file found. Creating a new one.");
			config = make_context();
			save(config.path, config, verbose);
			return error();
		}
		else if(file.is_open()){
			/* 
			 * See RapidJson docs:
			 *
			 * https://rapidjson.org/md_doc_tutorial.html
			 */
			using namespace rapidjson;
			
			/* Read JSON from config, parse, and check document. Must make sure that program does not crash here and use default config instead! */
			string contents, line;
			while(std::getline(file, line))
				contents += line + "\n";

			if(verbose > 0)
				log::info("Loading configuration file...\n{}", contents.c_str());
			
			Document doc;
			ParseErrorCode status = doc.Parse(contents.c_str()).GetParseError();

			if(!doc.IsObject()){
				error error(
					constants::error::FILE_NOT_FOUND,
					"Could not load config file."
				);
				log::error(error);
				return error;
			}

			error error = validate(doc);
			if(error()){
				log::error(error);
				return error;
			}

			/* Make sure contents were read correctly. */
			// if(!status){
			// 	log::error("config::load: Could not parse contents of file (Error: {}/{}).", GetParseError_En(status), doc.GetErrorOffset());

			// 	return context();
			// }

			/* Must check if keys exists first, then populate `_config_params`. */
			if(doc.HasMember("remote_sources")){
				if(doc["remote_sources"].IsArray()){
					const Value& srcs = doc["remote_sources"];
					for(auto& src : srcs.GetObject()){
						// config.remote_sources.push_back(src.GetString());
						config.remote_sources.insert(
							std::pair(src.name.GetString(), src.value.GetString())
						);
					}
				} else {
					gdpm::error error(
						constants::error::INVALID_KEY,
						"Could not read key `remote_sources`."
					);
					log::error(error);
					return error;
				}
			}
			auto _get_value_string = [](Document& doc, const char *property){
				if(doc.HasMember(property))
					if(doc[property].IsString())
						return doc[property].GetString();
				return "";
			};
			auto _get_value_int = [](Document& doc, const char *property){
				if(doc.HasMember(property))
					if(doc[property].IsInt())
						return doc[property].GetInt();
				return 0;
			};

			config.username 			= _get_value_string(doc, "username");
			config.password 			= _get_value_string(doc, "password");
			config.path 				= _get_value_string(doc, "path");
			config.token 				= _get_value_string(doc, "token");
			config.godot_version 		= _get_value_string(doc, "godot_version");
			config.packages_dir 		= _get_value_string(doc, "packages_dir");
			config.tmp_dir 				= _get_value_string(doc, "tmp_dir");
			config.threads 				= _get_value_int(doc, "threads");
			config.enable_sync 			= _get_value_int(doc, "enable_sync");
			config.enable_file_logging 	= _get_value_int(doc, "enable_file_logging");
		}
		return error();
	}


	error save(
		std::filesystem::path path, 
		const context& config, 
		int verbose
	){
		using namespace rapidjson;

		/* Build a JSON string to pass to document */
		string json = to_json(config);
		if(verbose > 0)
			log::info("Saving configuration file...\n{}", json.c_str());
		
		/* Dump JSON config to file */
		Document doc;
		doc.Parse(json.c_str());
		std::ofstream ofs(path);
		OStreamWrapper osw(ofs);

		PrettyWriter<OStreamWrapper> writer(osw);
		doc.Accept(writer);

		return gdpm::error();
	}


	context make_context(
		const string& username, 
		const string& password, 
		const string& path, 
		const string& token, 
		const string& godot_version, 
		const string& packages_dir, 
		const string& tmp_dir, 
		const string_map& remote_sources, 
		size_t threads, 
		size_t timeout, 
		bool enable_sync, 
		bool enable_file_logging, 
		int verbose
	){
		context config {
			.username = username,
			.password = password,
			.path = path,
			.token = token,
			.godot_version = godot_version,
			.packages_dir = (packages_dir.empty()) ? string(getenv("HOME")) + ".gdpm" : packages_dir,
			.tmp_dir = tmp_dir,
			.remote_sources = remote_sources,
			.threads = threads,
			.timeout = timeout,
			.enable_sync = enable_sync,
			.enable_file_logging = enable_file_logging,
			.verbose = verbose
		};
		return config;
	}


	error validate(const rapidjson::Document& doc){
		error error(constants::error::INVALID_CONFIG, "");
		if(!doc.IsObject()){
			error.set_message("Document is not a JSON object.");
			return error;
		}
		if(!doc.HasMember("remote_sources")){
			error.set_message("Could not find `remote_sources` in config.");
			return error;
		}
		if(!doc["remote_sources"].IsObject()){
			error.set_message("Key `remote_sources` is not a JSON object.");
			return error;
		}
		error.set_code(constants::error::NONE);
		return error;
	}

}