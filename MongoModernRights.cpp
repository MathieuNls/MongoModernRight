// MongoModernRights.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include <map>
#include <set>
#include <string>
#include <fstream>
#include <streambuf>

#include <cpprest/http_client.h>
#include <cpprest/filestream.h>

#include <cpprest/http_listener.h>              // HTTP server
#include <cpprest/json.h>                       // JSON library
#include <cpprest/uri.h>                        // URI library
#include <cpprest/ws_client.h>                  // WebSocket client
#include <cpprest/containerstream.h>            // Async streams backed by STL containers
#include <cpprest/interopstream.h>              // Bridges for integrating Async streams with STL and WinRT streams
#include <cpprest/rawptrstream.h>               // Async streams backed by raw pointer to memory
#include <cpprest/producerconsumerstream.h>     // Async streams for producer consumer scenarios

using namespace utility;                    // Common utilities like string conversions
using namespace web;                        // Common features like URIs.
using namespace web::http;                  // Common HTTP functionality
using namespace web::http::client;          // HTTP client features
using namespace concurrency::streams;       // Asynchronous streams

using namespace web::http::experimental::listener;          // HTTP server
using namespace web::experimental::web_sockets::client;     // WebSockets client
using namespace web::json;                                  // JSON library

using namespace std;
#define TRACE(msg)            wcout << msg


json::value config;
http_client dbclient(U("http://192.168.99.100:32769/"));


void display_field_map_json(const json::value & jvalue, std::string tab = "")
{
	if (!jvalue.is_null())
	{
		for (auto const & e : jvalue.as_object())
		{

			if (e.second.is_object()){
				wcout << e.first << L" : {" << endl;
				display_field_map_json(e.second, tab + "  ");
				wcout << L"}" << endl;
			}
			else {
				wcout << tab.c_str() << e.first << L" : " << e.second << endl;
			}
		}
	}
}



pplx::task<http_response> make_task_request(http_client & client,method mtd,json::value const & jvalue)
{
	display_field_map_json(jvalue);
	http_request request(mtd);
	request.headers().add(L"Accept", L"application/json");
	request.set_request_uri(L"/");
	return client.request(request);
}

void make_request(http_client & client, method mtd, json::value const & jvalue, http_request & request)
{
	make_task_request(client, mtd, jvalue)
	.then([](http_response response)
	{
		if (response.status_code() == status_codes::OK)
		{
			return response.extract_json();
		}
		return pplx::task_from_result(json::value());
	})
	.then([request](pplx::task<json::value> previousTask)
	{
		try
		{
			display_field_map_json(previousTask.get());
			request.reply(status_codes::OK, previousTask.get());
		}
		catch (http_exception const & e)
		{
			wcout << e.what() << endl;
		}
	})
	.wait();
}

void handle_get(http_request & request)
{
	TRACE(L"\nRECEIVED");
	make_request(dbclient, methods::GET, json::value::null(), request);
}

void handle_request(http_request request,
	function<void(json::value &)> action)
{

	request
		.extract_json()
		.then([&action](pplx::task<json::value> task) {
		try
		{
			auto & jvalue = task.get();

			if (!jvalue.is_null())
			{
				action(jvalue);
			}
		}
		catch (http_exception const & e)
		{
			wcout << e.what() << endl;
		}
	})
	.wait();

}

void handle_post(http_request request)
{

	TRACE("\nhandle POST\n");

	try
	{
		json::value obj = request.extract_json(true).get();
		obj[L"owner_id"] = json::value::string(L"35092_35092_353");
		display_field_map_json(obj);
	}
	catch (json_exception const & e)
	{
		wcout << e.what() << endl;
	}
}

void load_config() {
	try {
		string_t        importFile = L"config.json";		            // extract filename
		ifstream_t      f(importFile);                              // filestream of working file
		stringstream_t  s;                                          // string stream for holding JSON read from file

		if (f) {
			s << f.rdbuf();                                         // stream results of reading from file stream into string stream
			f.close();                                              // close the filestream

			config = json::value::parse(s);                                   // parse the resultant string stream.
			
			wcout << L"STARTING WITH CONFIG:";
			display_field_map_json(config);
		}
	}
	catch (web::json::json_exception excep) {
		std::cout << "ERROR Parsing JSON: ";
		std::cout << excep.what();
	}

}


int main()
{
	
	load_config();

	auto const & config_obj = config.as_object();
	string_t t = config_obj.at(L"host").as_string();

	wcout << config_obj.at(L"host");

	try
	{

		http_listener listener((config_obj.at(L"host").as_string()));


		listener.support(methods::GET, handle_get);
		listener.support(methods::POST, handle_post);


		listener
			.open()
			.then([&listener]() {TRACE(L"\nstarting to listen\n"); })
			.wait();

		while (true);
	}
	catch (exception const & e)
	{
		wcout << e.what() << endl;
	}
	




	

	return 0;
}