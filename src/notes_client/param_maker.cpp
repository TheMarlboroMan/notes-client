#include <notes_client/param_maker.h>

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <tools/json.h>

using namespace notes_client;

param_maker::param_maker() {

	doc.SetObject();
}

void param_maker::make_param(
	const std::string& _key,
	int _value
) {

	rapidjson::Value val(_value);

	doc.AddMember(
		tools::json_string(_key.c_str(), doc.GetAllocator()),
		val,
		doc.GetAllocator()
	);
}

void param_maker::make_param(
	const std::string& _key,
	const std::string& _value
) {

	doc.AddMember(
		tools::json_string(_key.c_str(), doc.GetAllocator()),
		tools::json_string(_value.c_str(), doc.GetAllocator()),
		doc.GetAllocator()
	);
}

std::string param_maker::to_string() const {

	rapidjson::StringBuffer sb;
	rapidjson::Writer<rapidjson::StringBuffer> wr(sb);
	doc.Accept(wr);
	return sb.GetString();
}
