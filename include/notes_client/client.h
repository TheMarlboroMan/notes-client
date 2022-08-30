#pragma once
#include <string>
#include <vector>
#include <curlw/curl_request_lib.h>
#include <rapidjson/document.h>
#include <lm/logger.h>
#include "note.h"

namespace notes_client {

class client {

	public:

	                        client(const std::string&, lm::logger&);
	void                    login(const std::string&, const std::string&);
	std::vector<note>       get_notes();
	void                    create_note(const std::string&);
	void                    patch_note(int, const std::string&);
	void                    delete_note(int);

	private:

	lm::logger&             logger;
	std::string             base_uri,
	                        token;

};
}
