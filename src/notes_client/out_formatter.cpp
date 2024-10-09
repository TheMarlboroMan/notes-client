#include "notes_client/out_formatter.h"

#include <tools/string_utils.h>
#include <tools/terminal_out.h>
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

	using namespace tools;
	auto default_color=generic();
	auto card_color=yellow();

	switch(_card.color_id) {

		case 1:
		break;
		case 2:
			card_color=orange();
		break;
		case 3:
			card_color=green();
		break;
		case 4:
			card_color=blue();
		break;
		case 5:
			card_color=red();
		break;
	}

	_stream<<s::text_color(card_color.fg)
			<<s::background_color(card_color.bg)
			<<"["<<_card.id<<"]"
			<<s::text_color(default_color.fg)
			<<s::background_color(default_color.bg)
			<<" created at "<<_card.created_at<<": "<<contents<<std::endl;
}

void out_formatter::format_note_full(
	const note& _note,
	std::ostream& _stream
) {

	using namespace tools;
	auto default_color=generic();
	auto card_color=yellow();

	switch(_note.color_id) {

		case 1:
		break;
		case 2:
			card_color=orange();
		break;
		case 3:
			card_color=green();
		break;
		case 4:
			card_color=blue();
		break;
		case 5:
			card_color=red();
		break;
	}

	_stream<<s::text_color(card_color.fg)
		<<s::background_color(card_color.bg)
		<<"created at: "<<_note.created_at<<std::endl
		<<"last updated at: "<<(_note.last_updated_at.size() ? _note.last_updated_at : "never")
		<<s::text_color(default_color.fg)
		<<s::background_color(default_color.bg)
		<<std::endl<<std::endl<<_note.contents<<std::endl;
}

note_display_color notes_client::generic() {

	using namespace tools;
	return note_display_color{txt_white, bg_black};
}

note_display_color notes_client::yellow() {

	using namespace tools;
	return note_display_color{txt_black, bg_yellow};
}

note_display_color notes_client::orange() {

	using namespace tools;
	return note_display_color{txt_black, bg_white};
}

note_display_color notes_client::green() {

	using namespace tools;
	return note_display_color{txt_white, bg_green};
}

note_display_color notes_client::blue() {

	using namespace tools;
	return note_display_color{txt_white, bg_blue};
}

note_display_color notes_client::red() {

	using namespace tools;
	return note_display_color{txt_white, bg_red};
}
