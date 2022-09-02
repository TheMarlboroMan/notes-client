#pragma once

#include <string>

namespace notes_client {

struct user_input {

	enum class commands {
		create,
		edit,
		remove,
		read,
		sync,
		exit,
		error
	};

	commands        command;
	int             id{0};
	std::string     error;
};

class cli_input {

	public:
/**
 * shows the prompt for the application
 */
	void            show_prompt() const;
/**
 * returns valid input
 */
	user_input      get_input() const;
};
}
