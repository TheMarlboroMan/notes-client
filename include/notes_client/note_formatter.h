#pragma once

#include "note.h"
#include <ostream>

namespace notes_client {

/**
 * interface for note formatters...
 */
class note_formatter {

	public:

	virtual void            format_note_card(const note&, std::ostream&)=0;
	virtual void            format_note_full(const note&, std::ostream&)=0;
};
}
