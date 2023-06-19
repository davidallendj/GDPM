
#include "package_manager.hpp"
#include "error.hpp"
#include "package.hpp"
#include "utils.hpp"
#include "rest_api.hpp"
#include "remote.hpp"
#include "config.hpp"
#include "constants.hpp"
#include "log.hpp"
#include "http.hpp"
#include "cache.hpp"
#include "types.hpp"

#include <algorithm>
#include <curl/curl.h>
#include <curl/easy.h>

#include <filesystem>
#include <regex>
#include <fmt/printf.h>
#include <rapidjson/document.h>
#include <cxxopts.hpp>
#include "clipp.h"

#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/prettywriter.h>
#include <system_error>
#include <future>


/*
 * For cURLpp examples...see the link below:
 *
 * https://github.com/jpbarrette/curlpp/tree/master/examples
 */

namespace gdpm::package_manager{
	CURL 			*curl;
	CURLcode 		res;
	config::context config;
	action_e 		action;

	// opts_t opts;
	bool skip_prompt 	= false;
	bool clean_tmp_dir 	= false;
	int priority 		= -1;

	error initialize(int argc, char **argv){
		// curl_global_init(CURL_GLOBAL_ALL);
		curl 		= curl_easy_init();
		config 		= config::make_context();
		action 		= action_e::none;

		/* Check for config and create if not exists */
		if(!std::filesystem::exists(config.path)){
			config::save(config.path, config);
		}
		error error = config::load(config.path, config);
		if(error.has_occurred()){
			log::error(error);
			return error;
		}

		/* Create the local databases if it doesn't exist already */
		error = cache::create_package_database();
		if(error.has_occurred()){
			log::error(error);
			return error;
		}

		return error;
	}


	error finalize(){
		curl_easy_cleanup(curl);
		error error = config::save(config.path, config);
		return error;
	}


	error parse_arguments(int argc, char **argv){
		using namespace clipp;

		/* Replace cxxopts with clipp */
		action_e action = action_e::none;
		package::title_list package_titles;
		package::params params;

		auto doc_format = clipp::doc_formatting{}
			.first_column(7)
			.doc_column(45)
			.last_column(99);
		
		/* Set global options */
		auto debugOpt			= option("-d", "--debug").set(config.verbose, to_int(log::DEBUG)) % "show debug output";
		auto configOpt 			= option("--config-path").set(config.path) % "set config path";
		auto fileOpt 			= repeatable(option("--file", "-f").set(params.args)  % "read file as input");
		auto pathOpt				= option("--path").set(params.paths) % "specify a path to use with command";
		auto typeOpt 			= option("--type").set(config.info.type) % "set package type (any|addon|project)";
		auto sortOpt 			= option("--sort").set(config.api_params.sort) % "sort packages in order (rating|cost|name|updated)";
		auto supportOpt 			= option("--support").set(config.api_params.support) % "set the support level for API (all|official|community|testing)";
		auto maxResultsOpt 		= option("--max-results").set(config.api_params.max_results) % "set the request max results";
		auto godotVersionOpt 	= option("--godot-version").set(config.api_params.godot_version) % "set the request Godot version";
		auto packageDirOpt		= option("--package-dir").set(config.packages_dir) % "set the global package location";
		auto tmpDirOpt			= option("--tmp-dir").set(config.tmp_dir) % "set the temporary download location";
		auto timeoutOpt			= option("--timeout").set(config.timeout) % "set the request timeout";
		auto verboseOpt 				= joinable(repeatable(option("-v", "--verbose").call([]{ config.verbose += 1; }))) % "show verbose output";	
		auto versionOpt			= option("--version").set(action, action_e::version);

		/* Set the options */
		auto cleanOpt 			= option("--clean").set(config.clean_temporary) % "enable/disable cleaning temps";
		auto parallelOpt 		= option("--jobs").set(config.jobs) % "set number of parallel jobs";
		auto cacheOpt 			= option("--enable-cache").set(config.enable_cache) % "enable/disable local caching";
		auto syncOpt 			= option("--enable-sync").set(config.enable_sync) % "enable/disable remote syncing";
		auto skipOpt 			= option("--skip-prompt").set(config.skip_prompt) % "skip the y/n prompt";
		auto remoteOpt			= option("--remote").set(params.remote_source) % "set remote source to use";
		
		auto packageValues 		= values("packages", package_titles);
		auto requiredPath 		= required("--path", params.args);

		auto installCmd = "install" % (
			command("install").set(action, action_e::install),
			packageValues % "packages to install from asset library",
			godotVersionOpt, cleanOpt, parallelOpt, syncOpt, skipOpt, remoteOpt
		);
		auto addCmd = "add" % (
			command("add").set(action, action_e::add),
			packageValues % "package(s) to add to local project", 
			parallelOpt, skipOpt, remoteOpt
		);
		auto removeCmd = "remove" % (
			command("remove").set(action, action_e::remove),
			packageValues % "package(s) to remove from local project"
		);
		auto updateCmd = "update" % (
			command("update").set(action, action_e::update),
			packageValues % "update package(s)"
		);
		auto searchCmd = "search" % (
			command("search").set(action, action_e::search),
			packageValues % "package(s) to search for"
		);
		auto exportCmd = "export" % (
			command("export").set(action, action_e::p_export),
			values("paths", params.args) % "export installed package list to file"
		);
		auto listCmd = "show installed packages" % (
			command("list").set(action, action_e::list)
		);
		auto linkCmd = "link" % (
			command("link").set(action, action_e::link),
			value("package", package_titles) % "package name to link",
			value("path", params.args) % "path to project"
		);
		auto cloneCmd = "clone" % (
			command("clone").set(action, action_e::clone),
			value("package", package_titles) % "packages to clone",
			value("path", params.args) % "path to project"
		);
		auto cleanCmd = "clean" % (
			command("clean").set(action, action_e::clean),
			values("packages", package_titles) % "package temporary files to remove"
		);
		auto configCmd = "get/set config properties" % (
			command("config").set(action, action_e::config_get),
			(
				( greedy(command("get")).set(action, action_e::config_get),
				  option(repeatable(values("properties", params.args))) % "get config properties"
				)
				| 
				( command("set").set(action, action_e::config_set), 
				  value("property", params.args[1]).call([]{}) % "config property",
				  value("value", params.args[2]).call([]{}) % "config value"
				)
			)
		);
		auto fetchCmd = "fetch" % (
			command("fetch").set(action, action_e::fetch),
			option(values("remote", params.args)) % "remote to fetch asset data"
		);
		auto add_arg = [&params](string arg) { params.args.emplace_back(arg); };
		auto remoteCmd = (
			command("remote").set(action, action_e::remote_list).if_missing(
				[]{
					remote::print_repositories(config);
				}
			),
			( 
				"add a remote source" % ( command("add").set(action, action_e::remote_add),
					word("name").call(add_arg) % "remote name", 
					value("url").call(add_arg) % "remote URL"
				)
				| 
				"remove a remote source" % ( command("remove").set(action, action_e::remote_remove),
					words("names", params.args) % "remote name(s)"
				)
				|
				"list remote sources" % ( command("list").set(action, action_e::remote_list))
			)
		);
		auto uiCmd = "start with UI" % (
			command("ui").set(action, action_e::ui)
		);
		auto helpCmd = "show this message and exit" % (
			command("help").set(action, action_e::help)
		);

		auto cli = (
			debugOpt, configOpt, verboseOpt, versionOpt,
			(installCmd | addCmd | removeCmd | updateCmd | searchCmd | exportCmd |
			listCmd | linkCmd | cloneCmd | cleanCmd | configCmd | fetchCmd |
			remoteCmd | uiCmd | helpCmd)
		);

		/* Make help output */
		string man_page_format("");
		auto man_page = make_man_page(cli, argv[0], doc_format)
			.prepend_section("DESCRIPTION", "\tManage Godot Game Engine assets from the command-line.")
			.append_section("LICENSE", "\tSee the 'LICENSE.md' file for more details.");
		std::for_each(man_page.begin(), man_page.end(), 
			[&man_page_format](const man_page::section& s){
				man_page_format += s.title() + "\n";
				man_page_format += s.content() + "\n\n";
			}
		);

		// log::level = config.verbose;
		if(clipp::parse(argc, argv, cli)){
			log::level = config.verbose;
			switch(action){
				case action_e::install: 		package::install(config, package_titles, params); break;
				case action_e::add:				package::add(config, package_titles);
				case action_e::remove: 			package::remove(config, package_titles, params); break;
				case action_e::update:			package::update(config, package_titles, params); break;
				case action_e::search: 			package::search(config, package_titles, params); break;
				case action_e::p_export:		package::export_to(params.args); break;
				case action_e::list: 			package::list(config, params); break;
												/* ...opts are the paths here */
				case action_e::link:			package::link(config, package_titles, params); break;
				case action_e::clone:			package::clone(config, package_titles, params); break;
				case action_e::clean:			package::clean_temporary(config, package_titles); break;
				case action_e::config_get:		config::print_properties(config, params.args); break;
				case action_e::config_set:		config::handle_config(config, package_titles, params.opts); break;
				case action_e::fetch:			package::synchronize_database(config, package_titles); break;
				case action_e::sync: 			package::synchronize_database(config, package_titles); break;
				case action_e::remote_list:		remote::print_repositories(config); break;
				case action_e::remote_add: 		remote::add_repository(config, params.args); break;
				case action_e::remote_remove: 	remote::remove_respositories(config, params.args); break;
				case action_e::ui:				log::println("UI not implemented yet"); break;
				case action_e::help: 			log::println("{}", man_page_format); break;
				case action_e::version:			break;
				case action_e::none:			/* ...here to run with no command */ break;
			}
		} else {
			log::println("usage:\n{}", usage_lines(cli, argv[0]).str());
		}
		return error();
	}

} // namespace gdpm::package_manager
