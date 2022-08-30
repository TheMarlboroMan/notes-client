#include <notes_client/client.h>
#include <notes_client/exception.h>
#include <notes_client/param_maker.h>
#include <curlw/curl_request_lib.h>
#include <lm/log.h>
#include <tools/json.h>
#include <iostream>

using namespace notes_client;

client::client(
	const std::string& _base_uri,
	lm::logger& _logger
):
	logger{_logger},
	base_uri{_base_uri}
{

}

void client::login(
	const std::string& _username,
	const std::string& _pass
) {

	param_maker pm{};
	pm.make_param("username", _username);
	pm.make_param("pass", _pass);

	const std::string uri=base_uri+"/login";
	curlw::curl_request request{uri, curlw::curl_request::methods::POST};

	auto response=request.set_payload(pm.to_string())
		.add_header("content-type", "application/json")
		.send();

	lm::log(logger).info()<<"sent request to "<<uri<<", got "<<response.get_status_code()<<std::endl;

	if(200!=response.get_status_code()) {

		throw login_exception(response.get_status_code(), response.get_body());
	}

	//locate the token header...
	for(const auto& header : response.get_headers()) {

		if(header.name=="notes-auth_token") {

			token=header.value;
			break;
		}
	}

	if(!token.size()) {

		lm::log(logger).warning()<<"got no token from response"<<std::endl;
		throw no_token_exception();
	}

	lm::log(logger).info()<<"got token "<<token<<" from response"<<std::endl;
}

std::vector<note> client::get_notes() {

	const std::string uri=base_uri+"/notes";
	curlw::curl_request request{uri, curlw::curl_request::methods::GET};

	auto response=request.add_header("notes-auth_token", token)
		.send();

	lm::log(logger).info()<<"sent request to "<<uri<<", got status code "<<response.get_status_code()<<std::endl;

	if(401==response.get_status_code()) {

		lm::log(logger).warning()<<"unauthorized attempt to retrieve notes"<<std::endl;
		throw unauthorized_exception("retrieving notes");
	}

	if(200!=response.get_status_code()) {

		lm::log(logger).warning()<<"invalid status code, response was "<<response.to_string()<<std::endl;
		throw server_exception("unexpected status code retrieving notes");
	}

	try {

		std::vector<note> result;
		auto doc=tools::parse_json_string(response.get_body());
		for(const auto& node : doc.GetArray()) {

#ifdef WITH_DEBUG

			assert(node.IsObject());
			assert(node.HasMember("id"));
			assert(node.HasMember("color_id"));
			assert(node.HasMember("created_at"));
			assert(node["created_at"].IsObject());
			assert(node["created_at"].HasMember("date"));
			assert(node.HasMember("last_updated_at"));
			assert(node.HasMember("contents"));
#endif
			result.push_back({  
				node["id"].GetInt(),
				node["color_id"].GetInt(),
				node["created_at"]["date"].GetString(),
				node["last_updated_at"].IsNull() ? "" : node["last_updated_at"]["date"].GetString(),
				node["contents"].GetString()
			});
		}

		lm::log(logger).info()<<"retrieved "<<result.size()<<" entries"<<std::endl;
		return result;
	}
	catch(std::exception& e) {

		lm::log(logger).warning()<<"failed to parse json "<<response.to_string()<<": "<<e.what()<<std::endl;
		throw server_exception("failed to parse response retrieve notes");
	}
}

void client::create_note(
	const std::string& _text
) {

}

void client::patch_note(
	int _note_id,
	const std::string& _text
) {

}

void client::delete_note(
	int _note_id
) {

}
