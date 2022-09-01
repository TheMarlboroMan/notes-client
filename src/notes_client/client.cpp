#include <notes_client/client.h>
#include <notes_client/exception.h>
#include <notes_client/param_maker.h>
#include <curlw/curl_request_lib.h>
#include <lm/log.h>
#include <tools/json.h>
#include <iostream>
#include <sstream>

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
			//dates will only consist of yyyy-mm-dd hh:mm:ss
			const std::string   created_at{node["created_at"]["date"].GetString()},
								last_updated_at{
				node["last_updated_at"].IsNull() 
					? "" 
					: node["last_updated_at"]["date"].GetString()
			};

			result.push_back({  
				node["id"].GetInt(),
				node["color_id"].GetInt(),
				created_at.substr(0, 18),
				last_updated_at.size() ? last_updated_at.substr(0, 18) : last_updated_at,
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

	const std::string uri=base_uri+"/notes";
	curlw::curl_request request{uri, curlw::curl_request::methods::POST};

	param_maker pm{};
	pm.make_param("contents", _text);
	pm.make_param("color_id", 1); //TODO: in the future we could change this

	auto response=request.add_header("notes-auth_token", token)
		.set_payload(pm.to_string())
		.send();

	//we expect either a 400 (which should not happen, since we forbid 
	//empty texts somewhere else in the program) or a 201. Let us go with
	//the 201 and anything else is a failure.
	if(201!=response.get_status_code()) {

		lm::log(logger).warning()<<"unexpected status code when creating note. Response was "<<response.to_string()<<std::endl;
		throw new_note_exception();
	}

	//TODO: we could at least return the location... but this will suffice for now.
}

void client::patch_note(
	int _note_id,
	const std::string& _text
) {

}

void client::delete_note(
	int _note_id
) {

	std::stringstream ss{};
	ss<<base_uri<<"/notes/"<<_note_id;

	std::string uri=ss.str();
	curlw::curl_request request{uri, curlw::curl_request::methods::DELETE};

	lm::log(logger).info()<<"will send a DELETE request to "<<uri<<std::endl;
	auto response=request.add_header("notes-auth_token", token)
		.send();

	//we expect a 200 or bust.
	if(200!=response.get_status_code()) {

		lm::log(logger).warning()<<"unexpected status code when deleting note. Response was "<<response.to_string()<<std::endl;
		throw delete_note_exception();
	}

}
