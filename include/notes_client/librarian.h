#pragma once

#include "note.h"
#include <lm/logger.h>
#include <string>
#include <vector>

namespace notes_client {

/**
 * the librarian maintains an index file which is used to work with the notes
 * instead of constantly downloading stuff from the remote end. It is basically
 * a note cache.
 */
class librarian {

	public:

	                        librarian(const std::string&, lm::logger&);
/**
 * remakes the index with fresh notes downloaded from the remote side.
 */
	void                    make_index(const std::vector<note>&);
	const std::vector<note>&    get_cards() const {return index_cards;}
	void                    read_index();
	bool                    has_note(int) const;
/**
 * retrieves the note with the given id, throws if cannot be found!
 */
	const note&             get_note(int) const;

	private:

	lm::logger&             logger;
	std::vector<note>       index_cards;  //not really index cards, since they contain the full text too
	const std::string       index_file; //not really an index file, more like the whole library.
};
}
