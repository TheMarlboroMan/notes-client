#include <notes_client/librarian.h>
#include <notes_client/exception.h>

#include <tools/file_utils.h>
#include <tools/json.h>
#include <lm/log.h>
#include <rapidjson/writer.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <fstream>

using namespace notes_client;

librarian::librarian(
	const std::string& _index_file,
	lm::logger& _logger
):
	index_file{_index_file},
	logger{_logger}
{

	read_index();
}

void librarian::make_index(
	const std::vector<note>& _notes
) {

	//build a json from these notes.
	rapidjson::Document doc;
	doc.SetArray();

	for(const auto& note : _notes) {

		rapidjson::Value node;
		node.SetObject();

		node.AddMember(
			tools::json_string("id", doc.GetAllocator()),
			rapidjson::Value(note.id),
			doc.GetAllocator()
		);

		node.AddMember(
			tools::json_string("color_id", doc.GetAllocator()),
			rapidjson::Value(note.color_id),
			doc.GetAllocator()
		);

		node.AddMember(
			tools::json_string("created_at", doc.GetAllocator()),
			tools::json_string(note.created_at, doc.GetAllocator()),
			doc.GetAllocator()
		);

		node.AddMember(
			tools::json_string("last_updated_at", doc.GetAllocator()),
			tools::json_string(note.last_updated_at, doc.GetAllocator()),
			doc.GetAllocator()
		);

		node.AddMember(
			tools::json_string("contents", doc.GetAllocator()),
			tools::json_string(note.contents, doc.GetAllocator()),
			doc.GetAllocator()
		);

		doc.GetArray().PushBack(
			node,
			doc.GetAllocator()
		);
	}

	//dump it into the index file.
	rapidjson::StringBuffer sb;
	rapidjson::Writer<rapidjson::StringBuffer> wr(sb);
	doc.Accept(wr);

	std::ofstream fs{index_file};
	fs<<sb.GetString();
}

void librarian::read_index() {

	//read the index file to fill up the index cards
	index_cards.clear();

	if(!tools::filesystem::exists(index_file)) {

		lm::log(logger).info()<<"index file not found, will not build index"<<std::endl;
		return;
	}

	const auto contents=tools::dump_file(index_file);
	if(!contents.size()) {

		lm::log(logger).info()<<"index size is empty, will not build index"<<std::endl;
		return;
	}

	//decode the json input, which will be a 1:1 of the index.
	try {

		auto doc=tools::parse_json_string(contents);
		for(const auto& note : doc.GetArray()) {

			index_cards.push_back({
				note["id"].GetInt(),
				note["color_id"].GetInt(),
				note["created_at"].GetString(),
				note["last_updated_at"].GetString(),
				note["contents"].GetString()
			});
		}

		lm::log(logger).info()<<"read "<<index_cards.size()<<" notes into the index"<<std::endl;	
	}
	catch(std::exception& e) {

		lm::log(logger).warning()<<"failed to read index file: "<<e.what()<<std::endl;
	}
}

bool librarian::has_note(
	int _note_id
) const {

	for(const auto& card : index_cards) {

		if(_note_id==card.id) {

			return true;
		}
	}
	return false;
}

const note& librarian::get_note(
	int _note_id
) const {

	for(const auto& note : index_cards) {

		if(_note_id==note.id) {

			return note;
		}
	}

	throw no_note_exception(_note_id);
}
