#pragma once

#include "note_formatter.h"

namespace notes_client {

class out_formatter:
	public note_formatter {

	public:

	void        format_note_card(const note&, std::ostream&);
	void        format_note_full(const note&, std::ostream&);
};

struct note_display_color {

	int fg,
		bg;
};

note_display_color generic();
note_display_color yellow();
note_display_color orange();
note_display_color green();
note_display_color blue();
note_display_color red();

}
