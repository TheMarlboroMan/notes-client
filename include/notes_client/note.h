#pragma once
#include <string>

namespace notes_client {

/**
 * a note structure as saved from the endpoint, which will be used by the
 * librarian class to be split into index cards and text files.
 */
struct note {

	int                 id,
	                    color_id;
	std::string         created_at,
	                    last_updated_at,
	                    contents;
};

}
