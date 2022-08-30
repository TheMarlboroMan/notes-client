#pragma once
#include <string>

namespace notes_client {

struct note {

	int                 id,
	                    color_id;
	std::string         created_at,
	                    last_updated_at,
	                    contents;
};

}
