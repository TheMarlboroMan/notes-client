#include "notes_client/out_formatter.h"

#include <tools/string_utils.h>
#include <string>

using namespace notes_client;

void out_formatter::format_note_card(
	const note& _card,
	std::ostream& _stream
) {

	std::string contents=_card.contents.substr(0, 60);
	tools::replace(contents, "\n", " ");
	tools::replace(contents, "\r", " ");
	tools::replace(contents, "\t", " ");

	_stream<<"["<<_card.id<<"] created at "<<_card.created_at<<": "<<contents<<std::endl;
}

void out_formatter::format_note_full(
	const note& _note,
	std::ostream& _stream
) {
	_stream<<"created at: "<<_note.created_at<<std::endl
		<<"last updated at: "<<(_note.last_updated_at.size() ? _note.last_updated_at : "never")<<std::endl
		<<std::endl
		<<_note.contents<<std::endl;
}
