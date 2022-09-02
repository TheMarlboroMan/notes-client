#pragma once

#include "note_formatter.h"

namespace notes_client {

class out_formatter:
	public note_formatter {

	public:

	void        format_note_card(const note&, std::ostream&);
	void        format_note_full(const note&, std::ostream&);
};
}
