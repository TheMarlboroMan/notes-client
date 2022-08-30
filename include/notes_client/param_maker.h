#pragma once
#include <rapidjson/document.h>
#include <string>

namespace notes_client {

/**
 * the only purpose of this class is to have something that can build a json
 * document painlessly so we can send it later accross the wire.
 */
class param_maker {

	public:
	                    param_maker();
	void                make_param(const std::string&, int);
	void                make_param(const std::string&, const std::string&);
	std::string         to_string() const;

	private:

	rapidjson::Document      doc;
};
}
